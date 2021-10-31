#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${SCRIPT_DIR}/${TEST_LOG} "sycl"

cd ${SCRIPT_DIR}

export CCL_ATL_TRANSPORT=mpi
export CCL_LOG_LEVEL=info

mpiexec -l -n 2 -ppn 1 ${SCRIPT_DIR}/benchmark > ${SCRIPT_DIR}/${TEST_LOG} 2>&1
rc=$?
if [ ${rc} -ne 0 ]
then
    echo "Fail"
    exit 1
fi

lib_type=$(cat ${SCRIPT_DIR}/${TEST_LOG} | grep "mpi_lib_attr.type" | awk '{print $NF}')
if [[ ${lib_type} != "mpich" ]]
then
    echo "Fail: ${lib_type} != mpich"
    exit 1
fi

echo "Pass"
