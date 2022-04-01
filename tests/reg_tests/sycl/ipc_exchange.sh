#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

ipc_exchange_modes="sockets drmfd"
transports="ofi mpi"
proc_counts="2 4"

bench_options="-w 0 -i 4 -j off -c all -b sycl -y 131072 -l all $(get_default_bench_dtype)"

for ipc_exchange_mode in ${ipc_exchange_modes}
do
    for transport in ${transports}
    do
        for proc_count in ${proc_counts}
        do
            cmd="CCL_ALLREDUCE=topo"
            cmd+=" CCL_ATL_TRANSPORT=${transport}"
            cmd+=" CCL_ZE_IPC_EXCHANGE=${ipc_exchange_mode}"
            cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
            cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
            run_cmd "${cmd}"
            check_log ${TEST_LOG}
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
