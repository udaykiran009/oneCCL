#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

comm_size_modes="reverse direct"
rank_order_modes="direct reorder"
parallel_comm_modes="0 1"
transports="ofi mpi"
algos="ring topo"

for comm_size_mode in ${comm_size_modes}
do
    for rank_order_mode in ${rank_order_modes}
    do
        for parallel_comm_mode in ${parallel_comm_modes}
        do
            for transport in ${transports}
            do
                for algo in ${algos}
                do
                    a2a_algo=${algo}
                    if [ ${algo} == "ring" ]
                    then
                        a2a_algo="scatter"
                    fi

                    cmd="CCL_ALLGATHERV=${algo}"
                    cmd+=" CCL_ALLREDUCE=${algo}"
                    cmd+=" CCL_ALLTOALLV=${a2a_algo}"
                    cmd+=" CCL_BCAST=${algo}"
                    cmd+=" CCL_ATL_TRANSPORT=${transport}"
                    cmd+=" mpiexec -l -n 6 -ppn 6 ${SCRIPT_DIR}/${BINFILE}"
                    cmd+=" -c ${comm_size_mode}"
                    cmd+=" -o ${rank_order_mode}"
                    cmd+=" -p ${parallel_comm_mode} > ${TEST_LOG} 2>&1"

                    run_cmd "${cmd}"
                    check_log ${TEST_LOG}
                done
            done
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
