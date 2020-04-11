#!/bin/bash

RELEASE="ccl_public"

CheckCommandExitCode() {
    if [ ${1} -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit ${1}
    fi
}

if [ ! -z "${1}" ]
then
    if [ "${1}" = "ccl_oneapi" ] || [ "${1}" = "ccl_public" ]
    then
        RELEASE=${1}
    else
        echo "Incorrect argument (expected 'ccl_oneapi' or 'ccl_public')"
        exit 1 
    fi
fi

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
WORKSPACE=`cd ${SCRIPT_DIR}/../ && pwd -P`

files=(
    "ccl ${WORKSPACE}/cmake"
    "vars.sh.in ${WORKSPACE}/cmake"
    "third-party-programs.txt ${WORKSPACE}"
)

for item in "${files[@]}"
do
    FILE=`echo "${item}" | cut -f 1 -d " "`
    MOVE_PATH=`echo "${item}" | cut -f 2 -d " "`
    echo "Copy ${RELEASE}/${FILE} to ${MOVE_PATH}"
    cd ${MOVE_PATH} && rm -r ${FILE} && cp ${WORKSPACE}/${RELEASE}/${FILE} .
    CheckCommandExitCode $? "ERROR: copy failed"
done
