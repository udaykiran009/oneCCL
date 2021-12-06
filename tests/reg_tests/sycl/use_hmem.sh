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
export CCL_ALLREDUCE=topo
export CCL_ATL_HMEM=1

# TODO: uncoment after fix MLSL-1155
#transports="ofi mpi"
transports="mpi"

proc_counts="4 8"

# MLSL-1225
# colls="allreduce reduce"
colls="allreduce"
single_list_modes="0 1"
use_hmem_modes="0 1"

bench_options="-w 4 -i 8 -l allreduce,reduce -c all -b sycl -t 10000000"

for transport in ${transports}
do
    for proc_count in ${proc_counts}
    do
        for coll in ${colls}
        do
            for single_list_mode in ${single_list_modes}
            do
                for use_hmem_mode in ${use_hmem_modes}
                do
                    export CCL_ATL_TRANSPORT=${transport}
                    export CCL_ZE_SINGLE_LIST=${single_list_mode}
                    export CCL_USE_HMEM=${use_hmem_mode}
                    mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark ${bench_options} -l ${coll} > ${TEST_LOG} 2>&1
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
done

rm ${TEST_LOG}
echo "Pass"
