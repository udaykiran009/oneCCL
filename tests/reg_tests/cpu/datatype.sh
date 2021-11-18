#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE=${BASENAME}

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

cd ${SCRIPT_DIR}

proc_counts="2 4"

for proc_count in ${proc_counts}
do
    cmd="CCL_WORKER_COUNT=2"
    cmd+=" CCL_ATL_TRANSPORT=ofi"
    cmd+=" FI_PROVIDER=tcp"
    cmd+=" mpiexec -l -n ${proc_count} -ppn 2"
    cmd+=" ${SCRIPT_DIR}/${BASENAME} > ${TEST_LOG} 2>&1"
    run_cmd "${cmd}"
    check_log ${TEST_LOG}
done

rm ${BINFILE} ${TEST_LOG}
echo "Pass"
