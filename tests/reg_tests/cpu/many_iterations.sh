#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
BINFILE="${BASENAME}"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG}

ppns="4"
transports="mpi ofi"

for ppn in ${ppns}
do
    for transport in ${transports}
    do
        cmd="CCL_ATL_TRANSPORT=${transport}"
        cmd+=" CCL_WORKER_COUNT=2"
        cmd+=" CCL_WORKER_AFFINITY=5-12"
        cmd+=" I_MPI_PIN_PROCESSOR_LIST=1,2,3,4"
        cmd+=" mpiexec -l -n $((${ppn}*2)) -ppn ${ppn}"
        cmd+=" ${SCRIPT_DIR}/${BINFILE} > ${TEST_LOG} 2>&1"
        run_cmd "${cmd}"
        check_log ${TEST_LOG}
    done
done

rm ${BINFILE} ${TEST_LOG}

echo "Pass"
