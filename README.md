# CEMU Docker
## 架构说明
此项目中的源码clone时间为2026.04.11, 源码目录未进行任何编译, 所有编译都在dockerfile中进行
CEMU目录中是基于FEMU(基于QEMU)修改后的CEMU源码
linux-cemu目录中是基于linux-6.8 kernel, 为cemu增加了适配接口的修改版kernel
ubpf-cemu目录中是为cemu增加ubpf能力的插件
dqib_amd64-pc目录中是qemu启动时所需的引导盘
Dockerfile中包括两阶段, 编译阶段和运行阶段, 后者删除了编译后所有不再使用的文件, 将docker镜像压缩为6.28GB, 其中包括:
	- ubuntu:22.04 + 编译工具链, 约为1GB
	- CEMU目录: 编译后的build目录仍然依赖于CEMU根目录, 因此暂未删减, 约为2.6GB
	- linux-cemu目录: 只保留编译后的vmlinux, 约为0.5GB
	- dqib_amd64-pc目录: 引导盘, 无法删减, 约为2.7GB

## 使用说明
镜像启动后直接运行默认配置的cemu, 即8核CPU+8G内存+1块48GB SSD
可直接从论文Appendix的A.5中的连接ssh步骤开始实验, 具体为

```
# 运行镜像
Host:~/cemu_docker& docker run -d --name cemu cemu_vm
# 连接进入容器
Host:~/cemu_docker& docker exec -it cemu /bin/bash
# 容器内部进入CEMU虚拟机
inside_Docker:/cemu/CEMU/build# ssh root@localhost -p 2222
# 开始后续实验
inside_VM:~# ./cemu-mount.sh 4
...
```

在run.sh中运行docker run时，可以通过添加CSD_COUNT指定CSD数量；进入虚拟机后，需要运行cemu-mount.sh挂载CSD，现在运行该脚本需要指定一个整型参数，用于指定初始化的数据文件大小(GB)，防止占用过多主机内存

## 后续开发
docker镜像中保留了编译工具链, 后续开发可在docker内部直接修改CEMU源码重新编译, 版本稳定后再使用docker cp将修改后的源码覆盖到主机文件并进行git管理
