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

export SYCL_DEVICE_FILTER=level_zero
export CCL_ALLREDUCE="ring:0-262143;topo:262144-max"
export CCL_REDUCE="rabenseifner:0-262143;topo:262144-max"

proc_counts="2"
ze_bin_modes="0 1"
detection_modes="0 1"

bench_options="-w 1 -i 4 -c last -l all -b sycl -t 524288"

for proc_count in ${proc_counts}
do
    for ze_bin_mode in ${ze_bin_modes}
    do
        for detection_mode in ${detection_modes}
        do
            if [[ ${ze_bin_mode} == "0" ]] && [[ ${detection_mode} == "1" ]];
            then
                continue
            fi

            cmd="IGC_EnableZEBinary=${ze_bin_mode}"
            cmd+=" IGC_DumpHasNonKernelArgLdSt=${detection_mode}"
            cmd+=" mpiexec -l -n ${proc_count} -ppn 2 ${SCRIPT_DIR}/benchmark"
            cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
            run_cmd "${cmd}"
            check_log ${TEST_LOG}
        done
    done
done

rm ${TEST_LOG}
echo "Pass"
