*** Settings ***
Resource    ../../resources/common.robot
Resource    ../../resources/envoy.robot
Resource    ../../resources/sipp.robot

Suite Setup    Network Preparation

Test Teardown    Terminate All Processes    kill=True

Test Timeout    20 sec

*** Test Cases ***
Basic Call
    Start Tra
    Update Tra    lskpmcs    I3F2=12.0.0.1 S3F2=13.0.0.1 

    ${egress_envoy} =       Start Envoy    egress       static-sip-egress-cfg.yaml       0    trace    envoy-egress.log
    Wait For Socket Listened    11.0.0.1:5060
    ${ingress_envoy_1} =    Start Envoy    ingress_1    static-sip-ingress-cfg-1.yaml    1    trace    envoy-ingress-1.log
    Wait For Socket Listened    12.0.0.1:5060
    ${ingress_envoy_2} =    Start Envoy    ingress_2    static-sip-ingress-cfg-2.yaml    2    trace    envoy-ingress-2.log
    Wait For Socket Listened    13.0.0.1:5060

    Log   Start Sip Server    console=yes
    ${sips1} =    Start Sipp Server 

    Log   Start Sip Client    console=yes
    Start Sipp Client 
    Stop Sipp Server    ${sips1} 
