#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

export CCL_YIELD=sched_yield

export CCL_ALLGATHERV=topo
export CCL_ALLREDUCE=topo
export CCL_BCAST=topo
export CCL_REDUCE=topo

transports="ofi mpi"
proc_counts="2 4 8"
if [[ ${PLATFORM_HW_GPU} = "ats" ]]
then
    proc_counts="2 4"
fi

colls="allgatherv,alltoallv,allreduce,reduce,bcast"
single_list_modes="0 1"
#TODO: merge with allreduce_bidir_algo.sh
bidir_modes="0 1"
cache_modes="0 1"

bench_options="-w 2 -i 4 -j off -c all -b sycl -y 17,1024,131072 $(get_default_bench_dtype)"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for bidir_mode in ${bidir_modes}
        do
            for single_list_mode in ${single_list_modes}
            do
                for cache_mode in ${cache_modes}
                do
                    cmd="CCL_ZE_SINGLE_LIST=${single_list_mode}"
                    cmd+=" CCL_ZE_BIDIR_ALGO=${bidir_mode}"
                    cmd+=" CCL_ATL_TRANSPORT=${transport}"
                    cmd+=" mpiexec -l -n ${proc_count} -ppn 4 ${SCRIPT_DIR}/benchmark"
                    cmd+=" ${bench_options} -l ${colls} -p ${cache_mode}"
                    cmd+=" > ${TEST_LOG} 2>&1"
                    run_cmd "${cmd}"
                    check_log ${TEST_LOG}
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
