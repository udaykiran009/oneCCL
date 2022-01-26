#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

mode=""

case $1 in
    "test"|"") mode="test";;
    "perf") mode="perf";;
    *) echo "unknown mode: $1";;
esac

function common_env() {
    export CCL_SYCL_OUTPUT_EVENT=1
    export CCL_ZE_CLOSE_IPC_WA=1
    export EnableDirectSubmission=1
}

function test_run() {
    common_env

    mpiexec -n 2 -ppn 2 ${SCRIPT_DIR}/${BINFILE} > ${TEST_LOG} 2>&1
    rc1=$?

    echo "\n\n\n....Running more iteration....\n\n\n" >> ${TEST_LOG}

    # run the same test with more iterations
    mpiexec -n 2 -ppn 2 ${SCRIPT_DIR}/${BINFILE} 15 2097152 1000 > ${TEST_LOG} 2>&1
    rc2=$?

    if [[ ${rc1} -ne 0 || ${rc2} -ne 0 ]]
    then
        echo "Fail"
        exit 1
    fi

    check_log ${TEST_LOG}

    rm -f ${BINFILE} ${TEST_LOG}
    echo "Pass"
}

function perf_run() {
    common_env

    mpiexec -n 2 -ppn 2 ${SCRIPT_DIR}/${BINFILE}
    rc=$?
    if [[ ${rc} -ne 0 ]]
    then
        echo "Fail"
        exit 1
    fi

    echo "Pass"
}

echo "Running in $mode mode"
if [[ $mode == "test" ]]
then
    test_run
else
    perf_run
fi

