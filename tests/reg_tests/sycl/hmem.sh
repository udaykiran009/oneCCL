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

export CCL_USE_HMEM=1
export FI_PROVIDER=tcp
export I_MPI_JOB_TIMEOUT=120

worker_counts="1"

# TODO: enable ofi transport, MLSL-1155 + find dmabuf-enabled nodes
# transports="ofi mpi"
transports="mpi"

send_proxy_modes="none regular"
hmem_modes="0 1"
single_list_modes="0 1"
algos="topo rabenseifner direct"

# TODO: enable reduce coll, MLSL-1225
# colls="allreduce reduce"
colls="allreduce"

proc_counts="2 4"
if [[ ${PLATFORM_HW_DISCRETE_GPU} = "ats" ]]
then
    export FI_PROVIDER=psm3
    # TODO: enable other algos after IMPI-3210
    algos="topo"
fi

bench_options="-w 4 -i 8 -j off -c all -b sycl -t 2097152"

for worker_count in ${worker_counts}
do
    for transport in ${transports}
    do
        for send_proxy_mode in ${send_proxy_modes}
        do
            for hmem_mode in ${hmem_modes}
            do
                for single_list_mode in ${single_list_modes}
                do
                    for algo in ${algos}
                    do
                        for coll in ${colls}
                        do
                            for proc_count in ${proc_counts}
                            do
                                # TODO: MLSL-1157
                                if [[ ${single_list_mode} == "1" ]] && [[ ${hmem_mode} == "1" ]];
                                then
                                    continue
                                fi
                                cmd="CCL_WORKER_COUNT=${worker_count}"
                                cmd+=" CCL_ATL_TRANSPORT=${transport}"
                                cmd+=" CCL_ATL_SEND_PROXY=${send_proxy_mode}"
                                cmd+=" CCL_ATL_HMEM=${hmem_mode}"
                                cmd+=" CCL_ZE_SINGLE_LIST=${single_list_mode}"
                                cmd+=" CCL_ALLREDUCE=${algo}"
                                cmd+=" CCL_REDUCE=${algo}"
                                cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
                                cmd+=" ${bench_options} -l ${coll} > ${TEST_LOG} 2>&1"
                                run_cmd "${cmd}"
                                check_log ${TEST_LOG}
                                check_hmem_log ${TEST_LOG} ${hmem_mode}
                            done
                        done
                    done
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
