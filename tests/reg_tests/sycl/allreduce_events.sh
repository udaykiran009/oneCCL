#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

export CCL_SYCL_OUTPUT_EVENT=1
export CCL_ZE_CLOSE_IPC_WA=1
export EnableDirectSubmission=1

mpiexec -n 2 -ppn 2 ${SCRIPT_DIR}/${BINFILE} > ${TEST_LOG} 2>&1
rc=$?
if [[ ${rc} -ne 0 ]]
then
    echo "Fail"
    exit 1
fi

check_log ${TEST_LOG}

rm -f ${BINFILE} ${TEST_LOG}
echo "Pass"
