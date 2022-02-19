#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

algos="ring topo"
topo_color_modes="ze fixed"
proc_counts="2 4"
ppns="1 2"

bench_options="-b sycl -w 1 -i 4 -c all -l allreduce,bcast -t 131072 $(get_default_bench_dtype)"

for algo in ${algos}
do
    for proc_count in ${proc_counts}
    do
        for ppn in ${ppns}
        do
            for topo_color_mode in ${topo_color_modes}
            do
                export CCL_ALLREDUCE=${algo}
                export CCL_BCAST=${algo}
                export CCL_TOPO_COLOR=${topo_color_mode}
                mpiexec -l -n ${proc_count} -ppn ${ppn} ${SCRIPT_DIR}/benchmark ${bench_options} > ${TEST_LOG} 2>&1
                ret_val=$?
                if [ $ret_val -ne 0 ]
                then
                    echo "Fail"
                    exit -1
                fi
            done
        done
    done
done

check_log ${TEST_LOG}

rm ${TEST_LOG}
echo "Pass"
