#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

inplace_modes="0 1"
ppns="2 4"

bench_options="-b sycl -w 1 -i 4 -c all -l allreduce -y 1,7,8,16,17,64,133,1077,16384,65539,131072,133073"
bench_dtypes="-d int8,int32,float16,float32"
if [[ ${PLATFORM_HW_DISCRETE_GPU} != "gen9" ]]
then
    bench_dtypes+=",bfloat16"
fi
bench_options+=" ${bench_dtypes}"

for inplace_mode in ${inplace_modes}
do
    for ppn in ${ppns}
    do
        cmd="CCL_ZE_BIDIR_ALGO=1"
        cmd+=" mpiexec -l -n $((2*ppn)) -ppn ${ppn} ${SCRIPT_DIR}/benchmark"
        cmd+=" ${bench_options} -q ${inplace_mode} > ${TEST_LOG} 2>&1"
        run_cmd "${cmd}"
        check_log ${TEST_LOG}
    done
done

rm ${TEST_LOG}
echo "Pass"
