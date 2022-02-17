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

export CCL_USE_HMEM=1
export I_MPI_JOB_TIMEOUT=120

worker_counts="1"
send_proxy_modes="none regular"
hmem_modes="0 1"
single_list_modes="0 1"
algos="topo rabenseifner"
proc_counts="2 4"
libfabric_dmabuf_peer_mem_path="/home/sys_ctlab/prog/libfabric_dmabuf_peer_mem/lib"
libfabrics="default dmabuf_peer_mem"
provs="verbs"
bench_options="-l allreduce,reduce -w 1 -i 4 -j off -c all -b sycl -t 131072"

for worker_count in ${worker_counts}
do
    for send_proxy_mode in ${send_proxy_modes}
    do
        for hmem_mode in ${hmem_modes}
        do        
            for single_list_mode in ${single_list_modes}
            do
                for algo in ${algos}
                do   
                    for proc_count in ${proc_counts}
                    do
                        for libfabric in ${libfabrics}
                        do
                            for prov in ${provs}
                            do
                                if [[ ${libfabric} == "dmabuf_peer_mem" ]]
                                then
                                    fabric_env="FI_HOOK=dmabuf_peer_mem"
                                    fabric_env+=" FI_VERBS_INLINE_SIZE=0"
                                    fabric_env+=" MLX5_SCATTER_TO_CQE=0"
                                    fabric_env+=" LD_LIBRARY_PATH=${libfabric_dmabuf_peer_mem_path}:${libfabric_dmabuf_peer_mem_path}/libfabric:${LD_LIBRARY_PATH}"
                                    fabric_env+=" FI_PROVIDER_PATH=${libfabric_dmabuf_peer_mem_path}/libfabric"
                                fi 
                                cmd=${fabric_env}
                                cmd+=" CCL_WORKER_COUNT=${worker_count}"
                                cmd+=" CCL_ATL_TRANSPORT=ofi"
                                cmd+=" CCL_ATL_SEND_PROXY=${send_proxy_mode}"
                                cmd+=" CCL_ATL_HMEM=${hmem_mode}"
                                cmd+=" CCL_ZE_SINGLE_LIST=${single_list_mode}"
                                cmd+=" CCL_ALLREDUCE=${algo}"
                                cmd+=" FI_PROVIDER=${prov}"
                                cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
                                cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
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