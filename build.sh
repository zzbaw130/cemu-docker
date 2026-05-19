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
if docker images | grep -q cemu_vm; then
    read -p "docker image cemu_vm exists, continue to build?(y/n): " confirm
    if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
        echo "build canceled"
        exit 0
    fi
    docker ps -aqf "ancestor=cemu_vm" | xargs docker rm -f
    docker rmi cemu_vm
fi
echo "building docker image..."
docker build -t cemu_vm .
