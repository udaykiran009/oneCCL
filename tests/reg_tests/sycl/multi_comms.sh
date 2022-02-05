#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

#TODO: uncoment after fix MLSL-1193 (OFI)
#comm_size_modes="direct reverse"
#parallel_comm_modes="0 1"

comm_size_modes="reverse"
rank_order_modes="direct reorder"
parallel_comm_modes="0"
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
                    cmd="CCL_ALLREDUCE=${algo} CCL_BCAST=${algo}"
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

rm ${BINFILE} ${TEST_LOG}
echo "Pass"
