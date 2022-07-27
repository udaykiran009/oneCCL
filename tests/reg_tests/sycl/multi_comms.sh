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
                if [ ${transport} == "ofi" ];
                then
                    if [ ${comm_size_mode} == "direct" ] || [ ${parallel_comm_mode} == "1" ]; then
                        echo "WARN: multi_comms.sh: Need to verify that the MLSL-1193 fix works for following parameters:"
                        echo "  comm_size_mode: ${comm_size_mode}  rank_order_mode: ${rank_order_mode}  parallel_comm_mode: ${parallel_comm_mode}  transport: ${transport}"
                        continue
                    fi
                fi
                if [[ $transport == "mpi" && $algo == "topo" && $rank_order_mode == "reorder" && $comm_size_mode == "direct" ]]
                then
                    #TODO: MLSL-1514
                    echo "WARN: transport=$transport, algo=$algo, rank_order_mode=$rank_order_mode, comm_size_mode=$comm_size_mode SKIPPED. Needs to be tested"
                    continue
                fi
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
