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

export SYCL_DEVICE_FILTER=opencl

transports="mpi ofi"
proc_counts="2 4"
worker_counts="1 2"
cache_modes="1"
bench_options="-n 2 -w 2 -i 8 -c all -l all -d int32,float32 -b sycl -t 2097152"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for worker_count in ${worker_counts}
        do
            for cache_mode in ${cache_modes}
            do
                cmd="CCL_ATL_TRANSPORT=${transport} CCL_WORKER_COUNT=${worker_count}"
                cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
                cmd+=" ${bench_options} -p ${cache_mode} > ${TEST_LOG} 2>&1"
                run_cmd "${cmd}"
                check_log ${TEST_LOG}
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
