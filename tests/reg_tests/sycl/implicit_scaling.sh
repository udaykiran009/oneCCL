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

export SYCL_DEVICE_FILTER=level_zero

transports="mpi"
proc_counts="1 2"
root_device_modes="0 1"

# TODO: MLSL-1280
# algos="ring topo"
algos="ring"

bench_options="-w 1 -i 4 -l allreduce,bcast -c all -b sycl -t 1048576"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for root_device_mode in ${root_device_modes}
        do
            for algo in ${algos}
            do
                cmd="CCL_ATL_TRANSPORT=${transport}"
                cmd+=" CCL_LOG_LEVEL=info"
                cmd+=" CCL_ALLREDUCE=${algo}"
                cmd+=" CCL_BCAST=${algo}"
                cmd+=" EnableImplicitScaling=1"
                cmd+=" mpiexec -l -n ${proc_count} -ppn 4 ${SCRIPT_DIR}/benchmark"
                cmd+=" ${bench_options} -g ${root_device_mode} > ${TEST_LOG} 2>&1"
                run_cmd "${cmd}"
                check_log ${TEST_LOG}
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
