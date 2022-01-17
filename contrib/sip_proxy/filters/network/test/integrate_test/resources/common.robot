*** Keywords ***
Network Preparation
    Run Process    ip address add 11.0.0.1/24 dev eth0    shell=True
    Run Process    ip address add 12.0.0.1/24 dev eth0    shell=True
    Run Process    ip address add 13.0.0.1/24 dev eth0    shell=True
