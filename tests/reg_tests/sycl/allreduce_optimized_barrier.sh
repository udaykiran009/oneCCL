#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG} "sycl"

cd ${SCRIPT_DIR}

export CCL_ATL_SYNC_COLL=1
export CCL_ATL_TRANSPORT=mpi

export CCL_ALLREDUCE=topo
export CCL_BARRIER=direct

export I_MPI_FABRICS=shm

# TODO: remove after fix IMPI-3127
export I_MPI_THREAD_LOCK_LEVEL=global

bench_options="-b sycl -w 4 -i 8 -c all -l allreduce -y 1000"

mpiexec -l -n 2 -ppn 2 ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
ret_val=$?
if [ $ret_val -ne 0 ]
then
    echo "Fail"
    exit -1
fi

check_log ${TEST_LOG}

rm ${TEST_LOG}
echo "Pass"
