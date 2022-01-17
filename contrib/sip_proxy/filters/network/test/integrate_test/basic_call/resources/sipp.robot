*** Settings ***
Library           Process

*** Variables ***
${SIPP_BIN}    /usr/local/bin/sipp
${CSVF_PATH}   scenario/ue.csv

${SIPC_XML_SF_PATH}    scenario/uac.xml
${SIPC_OPTIONS}    -t t1 -max_socket 2000 -l 10000 -m 10 -r 10 -rp 1s -fd 500  -default_behaviors all -trace_err -error_file error-uac.log -trace_stat -stf stats-uac.csv -watchdog_minor_threshold 2000 -watchdog_major_threshold 6000 -trace_screen $*
${SIPS_XML_SF_PATH}    scenario/uas.xml
${SIPS_OPTIONS}    -t t1 -m 10 -max_socket 10 -trace_err -error_file error-uac.log -trace_stat -stf stats-uac.csv -watchdog_minor_threshold 2000 -watchdog_major_threshold 6000 -trace_screen $*

*** Keywords ***
Start Sipp Client
    [Arguments]    ${name}=sipc
    
    ${result} =    Run Process    ${SIPP_BIN} -sf ${SIPC_XML_SF_PATH} -inf ${CSVF_PATH} ${SIPC_OPTIONS} 11.0.0.1:5060    shell=True
    Log    ${result.stderr}
    Log    ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0

Stop Sipp Client
    [Arguments]    ${name}=sipc
    stopSippSimulator    client    ${name}

Start Sipp Server
    [Arguments]    ${name}=sips
    ${simulator} =    Start Process    ${SIPP_BIN} -sf ${SIPS_XML_SF_PATH} -inf ${CSVF_PATH} -p 5060 ${SIPS_OPTIONS} 12.0.0.1:15060    shell=True
    [Return]    ${simulator}

Stop Sipp Server
    [Arguments]    ${simulator}
    #Send Signal To Process    SIGUSR1    ${simulator}
    ${result} =    Wait For Process    ${simulator}    42 secs
    Log    ${result.stderr}
    Log    ${result.stdout}
