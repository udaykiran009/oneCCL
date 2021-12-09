#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

copy_engines="auto main link"
single_list_modes="1 0"

export CCL_LOG_LEVEL=info

for copy_engine in ${copy_engines}
do
    for single_list_mode in ${single_list_modes}
    do
        export CCL_ZE_COPY_ENGINE=${copy_engine}
        export CCL_ZE_SINGLE_LIST=${single_list_mode}
        mpiexec -l -n 4 ${SCRIPT_DIR}/benchmark -l allreduce -y 2097152 > ${TEST_LOG} 2>&1
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
