*** Settings ***
Library           Process

*** Variables ***
${ENVOY_BIN}    /source/build/envoy/source/exe/envoy-contrib/envoy

*** Keywords ***
Start Envoy
    [Arguments]    ${name}    ${config_file}    ${base_id}    ${log_level}    ${log_path}
    ${result} =    Run Process    ss -lntp   shell=True
    Log    ${result.stdout}
    ${envoy} =    Start Process    ${ENVOY_BIN} -c ${config_file} --base-id ${base_id} --log-level ${log_level} --log-path ${log_path}    shell=True

Stop Envoy
    [Arguments]    ${envoy}
    ${result} =    Terminate Process    ${envoy}
