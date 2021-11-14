#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

proc_counts="1 2 4"
inplace_modes="0 1"
transports="ofi mpi"
# TODO: add topo algo
algos="multi_bcast flat"

export CCL_LOG_LEVEL=info

for proc_count in ${proc_counts}
do
    for inplace_mode in ${inplace_modes}
    do
        for transport in ${transports}
        do
            for algo in ${algos}
            do
                export CCL_ATL_TRANSPORT=${transport}
                export CCL_ALLGATHERV=${algo}
                mpiexec -l -n ${proc_count} -ppn 4 ${SCRIPT_DIR}/${BINFILE} ${inplace_mode} > ${TEST_LOG} 2>&1
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
done

rm ${BINFILE} ${TEST_LOG}

echo "Pass"
