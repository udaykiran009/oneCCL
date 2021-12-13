#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG}

cd ${SCRIPT_DIR}

worker_counts="1 4 8"
proc_counts="2"
transports="ofi mpi"
ofi_provs="tcp verbs"

bench_options="-c all -b host -t 2097152"

export FI_VERBS_DEVICE_NAME=hfi
export I_MPI_PIN_PROCESSOR_LIST=1
export CCL_WORKER_AFFINITY=2-9

for worker_count in ${worker_counts}
do
    for proc_count in ${proc_counts}
    do
        for transport in ${transports}
        do
            for ofi_prov in ${ofi_provs}
            do
                export FI_PROVIDER=${ofi_prov}
                export CCL_ATL_TRANSPORT=${transport}
                export CCL_WORKER_COUNT=${worker_count}
                mpiexec -l -n ${proc_count} -ppn 1 ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
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

echo "Pass"