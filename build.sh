PORT=${1:-2222}

# 克隆三个github项目
git clone https://github.com/cs-qyzhang/CEMU.git
git clone https://github.com/cs-qyzhang/linux-cemu.git
git clone https://github.com/cs-qyzhang/ubpf-cemu.git --recurse-submodules

# 克隆预制引导盘
if [ ! -d dqib_amd64-pc ]; then
    wget https://bit.ly/cemu-dqib -O dqib.tar.xz
    tar -xJf dqib.tar.xz
fi

# 编译docker镜像
docker build -t cemu_vm .

# -p指定主机以哪个端口连接虚拟机，不同用户请勿重复
# -v将主机的test-cemu目录覆盖容器的tests/cemu目录，
# 而容器的tests/cemu目录又会覆盖虚拟机的tests/cemu目录，
# 从而实现编辑主机自动覆盖虚拟机
docker run -d --name cemu_$USER -p $PORT:2222 -v $PWD/test-cemu:/cemu/CEMU/tests/cemu cemu_vm

# 等待虚拟机启动
sleep 20

# 启动后拷贝ssh公钥，方便后续ssh直连虚拟机
ssh-copy-id -f -i ./id_rsa.pub -p $PORT root@localhost

