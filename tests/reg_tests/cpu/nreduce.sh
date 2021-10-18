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

export CCL_ATL_TRANSPORT=ofi
export CCL_ALLREDUCE=nreduce

proc_counts="2 4 7 8"
inplace_modes="0 1"
bench_options="-c all -b host -t 10000000"

for proc_count in ${proc_counts}
do
    for inplace_mode in ${inplace_modes}
    do
        mpiexec.hydra -l -n ${proc_count} -ppn 1 ${SCRIPT_DIR}/benchmark ${bench_options} -q ${inplace_mode} > ${TEST_LOG} 2>&1
        rc=$?
        if [ ${rc} -ne 0 ]
        then
            echo "Fail"
            exit 1
        fi
    done
done

echo "Pass"
