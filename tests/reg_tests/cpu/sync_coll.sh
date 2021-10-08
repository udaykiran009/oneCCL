#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

if [ ! -f ${CCL_ROOT}/examples/benchmark/benchmark ]
then
    cd ${CCL_ROOT}/examples
    mkdir build
    cd build
    cmake .. &>> ${TEST_LOG}
    make benchmark &>> ${TEST_LOG}
    CheckCommandExitCode $? "Benchmark build failed"
    cp ${CCL_ROOT}/examples/build/benchmark/benchmark ${SCRIPT_DIR}
else
    cp ${CCL_ROOT}/examples/benchmark/benchmark ${SCRIPT_DIR}
fi

cd ${SCRIPT_DIR}

export CCL_ATL_TRANSPORT=mpi
export CCL_ALLREDUCE=direct
export CCL_ATL_SYNC_COLL=1

mpiexec.hydra -l -n 4 -ppn 1 ${SCRIPT_DIR}/benchmark -b host > ${TEST_LOG} 2>&1
rc=$?

if [ ${rc} -ne 0 ] ; then
    echo "fail"
    exit 1
else
    echo "Pass"
fi
