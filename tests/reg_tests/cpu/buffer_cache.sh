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

export CCL_LOG_LEVEL=info
export CCL_ATL_TRANSPORT=ofi

proc_counts="2 4"
buffer_cache_modes="0 1"
bench_options="-w 4 -i 8 -l all -c all -b host -t 1000000"

for proc_count in ${proc_counts}
do
    for buffer_cache_mode in ${buffer_cache_modes}
    do
        export CCL_BUFFER_CACHE=${buffer_cache_mode}
        mpiexec -l -n ${proc_count} -ppn 1 ${SCRIPT_DIR}/benchmark ${bench_options} >> ${TEST_LOG} 2>&1
        rc=$?
        if [ ${rc} -ne 0 ]
        then
            echo "Fail"
            exit 1
        fi
        check_log ${TEST_LOG}
    done
done

rm ${TEST_LOG}
echo "Pass"
