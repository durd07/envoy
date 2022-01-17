*** Settings ***
Library           Process
Library           RequestsLibrary
Resource          common.robot

*** Variables ***
${ENVOY_BIN}    /source/build/envoy/source/exe/envoy-contrib/envoy
${TRA_BIN}    /usr/local/bin/tra

*** Keywords ***
Start Envoy
    [Arguments]    ${name}    ${config_file}    ${base_id}    ${log_level}    ${log_path}
    ${result} =    Run Process    ss -lntp   shell=True
    Log    ${result.stdout}
    ${envoy} =    Start Process    ${ENVOY_BIN} -c ${config_file} --base-id ${base_id} --log-level ${log_level} --log-path ${OUTPUT DIR}/${log_path}    shell=True

Stop Envoy
    [Arguments]    ${envoy}
    ${result} =    Terminate Process    ${envoy}

Start Tra
    Start Process    /usr/local/bin/tra    shell=True
    Wait For Socket Listened    50052
    &{lskpmcs} =    Create Dictionary    S3F2=12.0.0.1
    Log    ${lskpmcs}
    ${resp} =    POST    http://localhost:50052/lskpmcs    json=${lskpmcs}

    ${resp}=    GET  http://localhost:50052/lskpmcs    expected_status=200
    Log    ${resp}
