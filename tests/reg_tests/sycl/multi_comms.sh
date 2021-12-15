#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

#TODO: uncoment after fix MLSL-1193 (OFI) and MLSL-1241 (MPI)
#comm_size_modes="direct reverse"
#algos="topo ring"
comm_size_modes="reverse"
transports="ofi mpi"

algos="ring"

for comm_size_mode in ${comm_size_modes}
do
    for transport in ${transports}
    do
        for algo in ${algos}
        do
            export CCL_ALLREDUCE=${algo}
            export CCL_BCAST=${algo}
            export CCL_ATL_TRANSPORT=${transport}
            mpiexec -l -n 6 -ppn 6 ${SCRIPT_DIR}/${BINFILE} ${comm_size_mode} > ${TEST_LOG} 2>&1
            rc=$?
            if [ ${rc} -ne 0 ]
            then
                echo "Fail"
                exit 1
            fi
            check_log ${TEST_LOG}
        done
    done
done

rm ${BINFILE} ${TEST_LOG}
echo "Pass"
