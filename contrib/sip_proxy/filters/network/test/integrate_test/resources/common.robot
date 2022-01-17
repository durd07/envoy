*** Keywords ***
Network Preparation
    Run Process    ip address add 11.0.0.1/24 dev eth0    shell=True
    Run Process    ip address add 12.0.0.1/24 dev eth0    shell=True
    Run Process    ip address add 13.0.0.1/24 dev eth0    shell=True

Socket Should Listen
    [Arguments]    ${port}
    ${result} =    Run Process    ss -lntp | grep ${port}    shell=True
    Should Be Equal As Integers    ${result.rc}    0

Wait For Socket Listened
    [Arguments]    ${port}
    Wait Until Keyword Succeeds    30 secs    1 secs    Socket Should Listen    ${port}
