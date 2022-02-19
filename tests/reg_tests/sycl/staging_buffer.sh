#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

export SYCL_DEVICE_FILTER=level_zero
export CCL_ALLTOALL=scatter

transports="mpi ofi"
provs="$(get_default_and_native_provs)"
proc_counts="2"
worker_counts="2"
staging_buffers="regular usm"

base_pattern="1,2,4,7,8,16,17,32,64,128,133,256,1077,16384,16387"
full_pattern="${base_pattern},${base_pattern},${base_pattern}"
bench_options="-n 16 -w 2 -i 8 -c all -l alltoall -b sycl -y ${full_pattern} $(get_default_bench_dtype)"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for worker_count in ${worker_counts}
        do
            for staging_buffer in ${staging_buffers}
            do
                for prov in ${provs}
                do
                    if [[ ${staging_buffer} == "usm" && ${prov} == "psm3" ]]
                    then
                        continue
                    fi
                    cmd="CCL_ATL_TRANSPORT=${transport} CCL_WORKER_COUNT=${worker_count}"
                    cmd+=" CCL_STAGING_BUFFER=${staging_buffer}"
                    cmd+=" FI_PROVIDER=${prov}"
                    cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
                    cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
                    run_cmd "${cmd}"
                    check_log ${TEST_LOG}
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
