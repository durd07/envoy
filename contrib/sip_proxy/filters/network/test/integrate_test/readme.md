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

## Install RobotFramework
```
pip3 install robotframework
```
