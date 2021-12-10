#!/bin/bash

# https://jira.devtools.intel.com/browse/MLSL-1160

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh
export CCL_WORKER_OFFLOAD=0

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG}

cd ${SCRIPT_DIR}

proc_counts="24"
transports="ofi mpi"
ofi_provs="tcp"

bench_options="-w 1 -i 4 -d all -c all -b host -y 17,1024,65536,4194304 -l allreduce"

for proc_count in ${proc_counts}
do
    for transport in ${transports}
    do
        for ofi_prov in ${ofi_provs}
        do
            export FI_PROVIDER=${ofi_prov}
            export CCL_ATL_TRANSPORT=${transport}
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

echo "Pass"
