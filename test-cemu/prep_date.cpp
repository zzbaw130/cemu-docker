int main(){
    // create input data
    int *buf_in = static_cast<int *>(aligned_alloc(4096, 4096 * 2));
    int *buf_out = static_cast<int *>(aligned_alloc(4096, 4096));
    for (int i = 0; i < 1024; i++) {
        buf_in[i] = i;
        buf_in[i*2] = i*2;
        buf_out[i] = 0;
    }

    if (src_in_file) {
        // write data to file
        int nvm_in_fd = open("/mnt/nvme0/in", O_RDWR | O_CREAT | O_DIRECT, 0666);
        if (nvm_in_fd == -1) {
            perror("open nvm in file");
            exit(1);
        }
        ret = pwrite(nvm_in_fd, buf_in, 8192, 0);
        assert(ret == 8192);

        // copy data from nvm to fdm
        long off_in = 0;
        long off_out = 0;
        ret = copy_file_range(nvm_in_fd, &off_in, mrs.fd[1], &off_out, 8192, 0);
        assert(ret == 8192);

        close(nvm_in_fd);
    } else {
        // directly write data to memory range set
        ret = pwrite(mrs.fd[1], buf_in, 8192, 0);
        assert(ret == 8192);
    }
    // clear output buffer
    ret = pwrite(mrs.fd[0], buf_out, 4096, 0);
    assert(ret == 4096);
}
