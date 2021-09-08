#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

CCL_KERNEL_OUTPUT_EVENT=1 EnableDirectSubmission=1 CCL_ZE_CLOSE_IPC_WA=1 mpiexec.hydra -n 2 -ppn 1 ${SCRIPT_DIR}/${BINFILE} > ${TEST_LOG} 2>&1
rc=$?

if [[ ${rc} -ne 0 ]] ; then
    echo "fail"
    exit 1
else
    rm -f ${BINFILE} ${TEST_LOG}
    echo "Pass"
fi
