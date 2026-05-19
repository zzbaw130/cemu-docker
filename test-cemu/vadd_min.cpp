#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <chrono>
#include <iostream>

#include "iouring.h"
#include "util.h"
#include "kernel/cemu_def.h"

void prep_data(int *buf_in) {
    // create input data
    for (int i = 0; i < 1024; i++) {
        buf_in[i * 2] = i;
        buf_in[i * 2 + 1] = i * 2;
    }

    // write data to file
    int nvm_in_fd = open("/mnt/nvme0/vadd_in", O_RDWR | O_CREAT | O_DIRECT, 0666);
    if (nvm_in_fd == -1) {
        perror("open nvm in file");
        exit(1);
    }
    int ret = pwrite(nvm_in_fd, buf_in, 8192l, 0);
    assert(ret == 8192);

    close(nvm_in_fd);
}

int cemu_test(int *buf_out, bool dst_in_file = false) {
    struct ioctl_download download{};
    download.name = "vadd";
    int ret;

    prep_shared_library("./build/vadd.so", "vadd", &download);

    int ctl_fd = open("/dev/nvme0c3", O_RDWR);
    if (ctl_fd == -1) {
        perror("open dev file");
        exit(1);
    }

    // load program
    ret = ioctl(ctl_fd, IOCTL_CEMU_DOWNLOAD, &download);
    if (ret <= 0)
        perror("ioctl");
    printf("download ret: %d, pind : %d\n", ret, download.pind);

    // activate program
    ret = ioctl(ctl_fd, IOCTL_CEMU_ACTIVATE, &download);
    if (ret)
        perror("ioctl");
    printf("activation ret: %d, pind : %d\n", ret, download.pind);

    // create memory range set
    struct ioctl_create_mrs mrs{};
    prep_mrs(&mrs, 2);
    for (int i = 0; i < mrs.nr_fd; i++) {
        char buf[64];
        sprintf(buf, "/mnt/fdm0/%d", i);
        mrs.fd[i] = open(buf, O_RDWR);
        if (mrs.fd[i] == -1) {
            perror("open data file");
            exit(1);
        }
    }
    ret = ioctl(ctl_fd, IOCTL_CEMU_CREATE_MRS, &mrs);
    if (ret)
        perror("ioctl CREATE_MRS");
    printf("create_mrs ret: %d, rsid : %d\n", ret, mrs.rsid);

    auto start = std::chrono::high_resolution_clock::now();

    // copy data from nvm to fdm
    int nvm_in_fd = open("/mnt/nvme0/vadd_in", O_RDWR | O_CREAT | O_DIRECT, 0666);
    if (nvm_in_fd == -1) {
        perror("open nvm in file");
        exit(1);
    }
    long off_in = 0;
    long off_out = 0;
    ret = copy_file_range(nvm_in_fd, &off_in, mrs.fd[1], &off_out, 8192, 0);
    assert(ret == 8192);
    close(nvm_in_fd);

    // execute program
    int ng_fd = open("/dev/ng0n3", O_RDWR);
    if (ng_fd == -1) {
        perror("open dev file");
        exit(1);
    }

    // passthru program execute nvme command

    struct nvme_passthru_cmd ioctl_cmd{};
    prep_nvme_passthru_program_execute(&ioctl_cmd,
                                       /*cparam1=*/1024,
                                       /*cparam2=*/0,
                                       /*pind=*/download.pind,
                                       /*rsid=*/mrs.rsid,
                                       /*numr=*/0,
                                       /*group=*/0,
                                       /*chunk_nlb=*/0,
                                       /*user_runtime=*/0,
                                       /*data=*/nullptr,
                                       /*data_len=*/0);
    // use ioctl nvme passthru
    ret = ioctl(ng_fd, NVME_IOCTL_IO_CMD, &ioctl_cmd);
    printf("ioctl nvme passthru ret: %d\n", ret);
    if (ret < 0) {
        perror("ioctl");
        return 1;
    }

    // confirm data destination
    if (dst_in_file) {
        // copy data from fdm to nvm
        int nvm_out_fd = open("/mnt/nvme0/vadd_out", O_RDWR | O_CREAT | O_DIRECT, 0666);
        if (nvm_out_fd == -1) {
            perror("open nvm out file");
            exit(1);
        }
        off_in = 0;
        off_out = 0;
        ret = copy_file_range(mrs.fd[0], &off_in, nvm_out_fd, &off_out, 4096, 0);
        assert(ret == 4096);

        // read output from nvm
        ret = pread(nvm_out_fd, buf_out, 4096, 0);
        assert(ret == 4096);
        close(nvm_out_fd);
    } else {
        // read directly from fdm
        ret = pread(mrs.fd[0], buf_out, 4096, 0);
        assert(ret == 4096);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "CSD Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" <<
            std::endl;

    // deactivate program
    ret = ioctl(ctl_fd, IOCTL_CEMU_DEACTIVATE, &download);
    if (ret)
        perror("ioctl");
    printf("deactivation ret: %d, pind : %d\n", ret, download.pind);

    // unload program
    ret = ioctl(ctl_fd, IOCTL_CEMU_UNLOAD, &download);
    if (ret)
        perror("ioctl");
    printf("remove ret: %d, pind : %d\n", ret, download.pind);

    close(ctl_fd);
    close(ng_fd);
    for (int i = 0; i < mrs.nr_fd; i++) {
        close(mrs.fd[i]);
    }

    return 0;
}

extern "C" {
long long vadd(struct cemu_args *args);
}

int cemu_common(int *buf_in, int *buf_out) {
    // copy data from nvm to memory
    int nvm_in_fd = open("/mnt/nvme0/vadd_in", O_RDWR | O_CREAT | O_DIRECT, 0666);
    if (nvm_in_fd == -1) {
        perror("open nvm in file");
        exit(1);
    }
    pread(nvm_in_fd, buf_in, 4096 * 2, 0);

    void *addr_array[2] = {buf_out, buf_in};
    long long len_array[2] = {4096, 4096 * 2};
    struct cemu_args args{};
    args.numr = 2;
    args.mr_addr = addr_array;
    args.mr_len = len_array;
    args.cparam1 = 1024;

    long long result = vadd(&args);
    assert(result == 1024);

    return 0;
}

void validate_data(const int *buf_in, const int *buf_out) {
    // validate data
    for (int i = 0; i < 1024; i++) {
        assert(buf_in[i * 2] + buf_in[i * 2 + 1] == buf_out[i]);
        // printf("%d + %d = %d\n", buf_in[i * 2], buf_in[i * 2 + 1], buf_out[i]);
    }
    printf("data validation passed\n");
}

int main() {
    int *buf_in = static_cast<int *>(aligned_alloc(4096, 4096 * 2));
    int *buf_out = static_cast<int *>(aligned_alloc(4096, 4096));
    // prep_data(buf_in);

    // ===========================cal in host================================
    auto start = std::chrono::high_resolution_clock::now();

    cemu_common(buf_in, buf_out);

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Host Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" <<
            std::endl;

    validate_data(buf_in, buf_out);

    // ===========================cal in csd================================

    cemu_test(buf_out);

    validate_data(buf_in, buf_out);

    return 0;
}
