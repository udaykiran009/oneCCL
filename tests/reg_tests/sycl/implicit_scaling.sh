#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

transports="mpi"
proc_counts="1 2"
root_device_modes="0 1"
implicit_scaling_modes="0 1"
algos="ring topo"

bench_options="-w 1 -i 4 -l allreduce,bcast -c all -b sycl -t 1048576 $(get_default_bench_dtype)"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for root_device_mode in ${root_device_modes}
        do
            for implicit_scaling_mode in ${implicit_scaling_modes}
            do
                for algo in ${algos}
                do
                    cmd="CCL_ATL_TRANSPORT=${transport}"
                    cmd+=" CCL_LOG_LEVEL=info"
                    cmd+=" CCL_ALLREDUCE=${algo}"
                    cmd+=" CCL_BCAST=${algo}"
                    cmd+=" SYCL_DEVICE_FILTER=level_zero"
                    cmd+=" EnableImplicitScaling=${implicit_scaling_mode}"
                    cmd+=" mpiexec -l -n ${proc_count} -ppn 4 ${SCRIPT_DIR}/benchmark"
                    cmd+=" ${bench_options} -g ${root_device_mode} > ${TEST_LOG} 2>&1"
                    run_cmd "${cmd}"
                    check_log ${TEST_LOG}
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
