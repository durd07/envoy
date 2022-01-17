# Sip Proxy Integrate Test

export http_proxy=http://10.158.100.9:8080
export https_proxy=http://10.158.100.9:8080
apt update
apt install vim

## Install Sipp
```
apt install -y pkg-config dh-autoreconf ncurses-dev build-essential libssl-dev libpcap-dev libncurses5-dev libsctp-dev lksctp-tools cmake
git clone https://github.com/SIPp/sipp.git
cd sipp
cmake . -DUSE_SSL=1 -DUSE_SCTP=1 -DUSE_PCAP=1 -DUSE_GSL=1
make -j$(nproc) all
make install
```

## Install Tra
```
apt install -y protobuf-compiler

# Install Golang
wget -O /tmp/go.tar.gz https://go.dev/dl/go1.17.6.linux-amd64.tar.gz
rm -rf /usr/local/go && tar -C /usr/local -xzf /tmp/go1.17.6.linux-amd64.tar.gz

go get -u google.golang.org/protobuf/cmd/protoc-gen-go
go install google.golang.org/protobuf/cmd/protoc-gen-go

go get -u google.golang.org/grpc/cmd/protoc-gen-go-grpc
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc

export PATH=/root/go/bin:/usr/local/go/bin:$PATH

git clone https://github.com/durd07/tra.git
cd tra
make server
```

or you can download the binary directly
```
wget -O /usr/local/bin/tra https://github.com/durd07/tra/releases/download/v0.0.1-v3alpha/tra_server-ubuntu1804
chmod +x /usr/local/bin/tra
```

## Install RobotFramework
```
pip3 install robotframework robotframework-requests
```
