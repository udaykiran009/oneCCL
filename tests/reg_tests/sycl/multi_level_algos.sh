#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG} "sycl"

cd ${SCRIPT_DIR}

export CCL_LOG_LEVEL=info

# TODO: uncoment after fix MLSL-1156
# proc_counts="2 4 8"
proc_counts="2 4"
collectives="allreduce allgatherv reduce"
transports="ofi mpi"
bench_options="-w 4 -i 8 -c all -b sycl -t 10000000"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for coll in ${collectives}
        do
            # TODO: uncoment after fix MLSL-1155
            # export CCL_ATL_TRANSPORT=${transport}
            export CCL_ATL_TRANSPORT=mpi
            mpiexec.hydra -l -n ${proc_count} -ppn 4 ${SCRIPT_DIR}/benchmark ${bench_options} -l ${coll} >> ${TEST_LOG} 2>&1
            rc=$?
            if [ ${rc} -ne 0 ]
            then
                echo "Fail"
                exit 1
            fi
        done
    done
done

rm ${TEST_LOG}

echo "Pass"
