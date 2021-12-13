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
export FI_PROVIDER=tcp

transports="mpi"
proc_counts="2"
affinity_masks="0.0 0.0,0.1 0.0,1.0"

bench_options="-w 1 -i 8 -c all -l all -b sycl -y 1024"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for affinity_mask in ${affinity_masks}
        do
            cmd="CCL_ATL_TRANSPORT=${transport}"
            cmd+=" ZE_AFFINITY_MASK=${affinity_mask}"
            cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
            cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
            run_cmd "${cmd}"
            check_log ${TEST_LOG}
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
