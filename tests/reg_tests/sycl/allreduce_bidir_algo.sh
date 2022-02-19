#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

inplace_modes="0 1"

bench_options="-d all -b sycl -w 4 -i 8 -c all -l allreduce -t 131072"

for inplace_mode in ${inplace_modes}
do
    cmd="CCL_ZE_BIDIR_ALGO=1"
    cmd+=" mpiexec -l -n 2 -ppn 2 ${SCRIPT_DIR}/benchmark"
    cmd+=" ${bench_options} -q ${inplace_mode} > ${TEST_LOG} 2>&1"
    run_cmd "${cmd}"
    check_log ${TEST_LOG}
done

rm ${TEST_LOG}
echo "Pass"
