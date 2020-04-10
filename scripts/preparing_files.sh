#!/bin/bash

CheckCommandExitCode() {
    if [ ${1} -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit ${1}
    fi
}

if [ ${1} != "ccl_oneapi" ] && [ ${1} != "ccl_public" ]
then
    echo "Incorrect argument (expected 'inteloneapi' or 'ccl_public')"
    exit 1
fi

RELEASE=${1}
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
    echo "copy $FILE to $MOVE_PATH"
    cp ${WORKSPACE}/${RELEASE}/${FILE} ${MOVE_PATH}
    CheckCommandExitCode $? "ERROR: copying failed"
done
