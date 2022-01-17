*** Settings ***
Resource    ../../resources/common.robot
Resource    ../../resources/envoy.robot
Resource    ../../resources/sipp.robot

Suite Setup    Network Preparation

Test Teardown    Terminate All Processes    kill=True

Test Timeout    20 sec

*** Test Cases ***
Basic Call
    ${egress_envoy} =    Start Envoy    egress    envoy/static-sip-egress-cfg.yaml    0    trace    envoy-egress.log
    ${ingress_envoy_1} =    Start Envoy    ingress_1    envoy/static-sip-ingress-cfg-1.yaml    1    trace    envoy-ingress-1.log
    ${ingress_envoy_2} =    Start Envoy    ingress_2    envoy/static-sip-ingress-cfg-2.yaml    2    trace    envoy-ingress-2.log

    Sleep    3 secs

    Log   Start Sip Server    console=yes
    ${sips1} =    Start Sipp Server 

    Log   Start Sip Client    console=yes
    Start Sipp Client 
    Stop Sipp Server    ${sips1} 
