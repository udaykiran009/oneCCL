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

check_hmem_log() {
    log_path=$1
    hmem_mode=$2
    passed_pattern="in: { .* hmem: ${hmem_mode}"
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [ ${passed_count} -eq 0 ]
    then
        echo "Error: did not find input hmem enable in log ${log_path}"
        exit 1
    fi
    passed_pattern="out: { .* hmem: ${hmem_mode}"
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [ ${passed_count} -eq 0 ]
    then
        echo "Error: did not find output hmem enable in log ${log_path}"
        exit 1
    fi
}

export CCL_ATL_TRANSPORT=mpi
export CCL_LOG_LEVEL=info
export FI_PROVIDER=tcp

allreduce_algorithms="ring direct"
worker_counts="1"
# TODO: enable proc_count=4
# https://jira.devtools.intel.com/browse/IMPI-3210
proc_count="2"
hmem_modes="0 1"

bench_options="-b sycl -l allreduce -c all -w 1 -i 4 -j off -t 2097152"

for worker_count in ${worker_counts}
do
    for allreduce_algorithm in ${allreduce_algorithms}
    do
        for hmem_mode in ${hmem_modes}
        do
            export CCL_ALLREDUCE=${allreduce_algorithm}
            export CCL_WORKER_COUNT=${worker_count}
            export CCL_ATL_HMEM=${hmem_mode}
            mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
            rc=$?
            if [ ${rc} -ne 0 ]
            then
                echo "Fail"
                exit 1
            fi
            check_log ${TEST_LOG}
            check_hmem_log ${TEST_LOG} ${hmem_mode}
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
