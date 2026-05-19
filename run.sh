PORT=${1:-2222}

if docker ps -aqf "publish=$PORT" | grep -q .; then
    echo "端口 $PORT 已被占用，请运行 $0 <PORT> 指定其他端口。"
    exit 1
fi

# -p指定主机以哪个端口连接虚拟机，不同用户请勿重复
# -v将主机的test-cemu目录覆盖容器的tests/cemu目录，
# 而容器的tests/cemu目录又会覆盖虚拟机的tests/cemu目录，
# 从而实现编辑主机自动覆盖虚拟机
docker run -d \
    --name cemu_$USER \
    -p $PORT:2222 \
    -e CSD_COUNT=1 \
    -v $PWD/test-cemu:/cemu/CEMU/tests/cemu \
    cemu_vm

# 等待虚拟机启动
#sleep 40

# 启动后拷贝ssh公钥，方便后续ssh直连虚拟机
#ssh-copy-id -f -i ./id_rsa.pub -p $PORT root@localhost

