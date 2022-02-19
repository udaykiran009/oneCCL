#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

export CCL_ALLREDUCE=topo
export CCL_REDUCE=topo
export CCL_YIELD=sched_yield
export CCL_USE_HMEM=1

export SYCL_DEVICE_FILTER=level_zero

# TODO: MLSL-to_be_update
#hmems="0 1"
hmems="0"
transports="mpi ofi"
worker_counts="1 2"
multi_workers="0 1"
ppns="2 4"
cache_modes="0 1"

bench_options="-w 1 -i 2 -c all -l allreduce,reduce -b sycl -y 1024,131072 $(get_default_bench_dtype)"

for transport in ${transports}
do
    for hmem in ${hmems}
    do
        if [[ "${transport}" == "ofi" ]] && [[ "${hmem}" == "1" ]]
        then
            continue
        fi
        for ppn in ${ppns}
        do
            #TODO: unstable fail MLSL-to_be_update
            if [[ "${transport}" == "ofi" ]] && [[ "${ppn}" == "4" ]]
            then
                continue
            fi
            for worker_count in ${worker_counts}
            do
                #TODO: unstable fail MLSL-to_be_update
                if [[ "${transport}" == "ofi" ]] && [[ "${worker_count}" == "1" ]]
                then
                    continue
                fi
                for multi_worker in ${multi_workers}
                do
                    for cache_mode in ${cache_modes}
                    do
                        cmd="CCL_ATL_TRANSPORT=${transport}"
                        cmd+=" CCL_WORKER_COUNT=${worker_count}"
                        cmd+=" CCL_ZE_MULTI_WORKERS=${multi_worker}"
                        cmd+=" CCL_ATL_HMEM=${hmem}"
                        cmd+=" CCL_LOG_LEVEL=debug"
                        cmd+=" mpiexec -l -n $((${ppn}*2)) -ppn ${ppn} ${SCRIPT_DIR}/benchmark"
                        cmd+=" ${bench_options} -p ${cache_mode} > ${TEST_LOG} 2>&1"
                        run_cmd "${cmd}"
                        check_log ${TEST_LOG}
                    done
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
