git clone https://github.com/cs-qyzhang/CEMU.git
git clone https://github.com/cs-qyzhang/linux-cemu.git
git clone https://github.com/cs-qyzhang/ubpf-cemu.git --recurse-submodules

wget https://bit.ly/cemu-dqib -O dqib.tar.xz
tar -xJf dqib.tar.xz

docker build -t cemu_vm .

docker run -d --name cemu -p 2323:2222 cemu_vm

sleep 20

ssh-copy-id -i ./id_rsa.pub -p 2323 root@localhost