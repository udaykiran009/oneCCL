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

algos="ring topo"
topo_color_modes="ze fixed"
proc_counts="2 4"

bench_options="-b sycl -w 4 -i 8 -c all -l allreduce -t 2097152"

for algo in ${algos}
do
    for proc_count in ${proc_counts}
    do
        for topo_color_mode in ${topo_color_modes}
        do
            export CCL_ALLREDUCE=${algo}
            export CCL_TOPO_COLOR=${topo_color_mode}
            mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
            ret_val=$?
            if [ $ret_val -ne 0 ]
            then
                echo "Fail"
                exit -1
            fi
        done
    done
done

check_log ${TEST_LOG}

rm ${TEST_LOG}
echo "Pass"
