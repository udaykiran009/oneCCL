#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG}

cd ${SCRIPT_DIR}

export CCL_ATL_TRANSPORT=mpi
export CCL_ALLREDUCE=direct
export CCL_ATL_SYNC_COLL=1

mpiexec.hydra -l -n 4 -ppn 1 ${SCRIPT_DIR}/benchmark -b host > ${TEST_LOG} 2>&1
rc=$?

if [ ${rc} -ne 0 ] ; then
    echo "Fail"
    exit 1
else
    echo "Pass"
fi
