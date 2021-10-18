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

proc_counts="4 8"
transports="ofi mpi"
# ofi_provs="tcp psm3" # TODO: uncomment after fix MLSL-1112
ofi_provs="tcp"

bench_options="-c all -b host -t 10000000"

for proc_count in ${proc_counts}
do
    for transport in ${transports}
    do
        for ofi_prov in ${ofi_provs}
        do
            export FI_PROVIDER=${ofi_prov}
            export CCL_ATL_TRANSPORT=${transport}
            mpiexec.hydra -l -n ${proc_count} -ppn 1 ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
            rc=$?
            if [ ${rc} -ne 0 ]
            then
                echo "Fail"
                exit 1
            fi
        done
    done
done

echo "Pass"