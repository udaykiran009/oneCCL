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
export CCL_ALLTOALL=scatter
export FI_PROVIDER=tcp

transports="mpi ofi"
proc_counts="2"
worker_counts="2"

staging_buffers="regular usm"
if [[ ${PLATFORM_HW_DISCRETE_GPU} = "ats" ]]
then
    # TODO: debug on ATS and fill ticket
    staging_buffers="usm"
fi

base_pattern="1,2,4,7,8,16,17,32,64,128,133,256,1077,16384,16387"
full_pattern="${base_pattern},${base_pattern},${base_pattern}"
bench_options="-n 16 -w 2 -i 8 -c all -l alltoall -b sycl -y ${full_pattern}"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for worker_count in ${worker_counts}
        do
            for staging_buffer in ${staging_buffers}
            do
                cmd="CCL_ATL_TRANSPORT=${transport} CCL_WORKER_COUNT=${worker_count}"
                cmd+=" CCL_STAGING_BUFFER=${staging_buffer}"
                cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
                cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
                run_cmd "${cmd}"
                check_log ${TEST_LOG}
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
