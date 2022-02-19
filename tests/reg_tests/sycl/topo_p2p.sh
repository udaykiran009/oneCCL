#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

find_expected_algo() {
    found_algo_str=$1
    expected_strs=$2

    match_count=0
    for expected_str in ${expected_strs}; do
        if [[ "$found_algo_str" == "$expected_str" ]]; then
            match_count=$((match_count+1))
        fi
    done

    if [[ "$count_matches" == "0" ]]; then
        echo "Fail: couldn't find expected algo" >> ${TEST_LOG} 2>&1
    else
        echo "Found matches: ${count_matches}" >> ${TEST_LOG} 2>&1
    fi
}

parse_algo() {
    filename=$1
    expected_strs=$2

    found_algo_str=""
    while IFS='' read -r line; do
        if [[ $line =~ "selected algo: coll allreduce, "(.*) ]]; then
            found_sub_str=${BASH_REMATCH[1]}
            if [[ $found_sub_str =~ algo\ ([a-zA-Z0-9_]*) ]]; then
                found_algo_str="${BASH_REMATCH[1]}"
            fi
        fi
    done < $filename

    find_expected_algo "${found_algo_str}" "${expected_strs}"
}

export SYCL_DEVICE_FILTER=level_zero

transports="mpi ofi"

expected_non_topo_algos="direct ring recursive_doubling"
bench_options="-w 1 -i 2 -c all -l allreduce -b sycl -t 131072 $(get_default_bench_dtype)"

run_case() {
    expected_str=$1
    extra_env_str=$2
    extra_bench_ops=$3

    for transport in ${transports}
    do
        cmd="CCL_ATL_TRANSPORT=${transport}"
        cmd+=" CCL_LOG_LEVEL=debug"
        cmd+=" CCL_TOPO_COLOR=ze"
        cmd+=" ${extra_env_str}"
        cmd+=" mpiexec -l -n 2 ${SCRIPT_DIR}/benchmark"
        cmd+=" ${bench_options} ${extra_bench_ops} > ${TEST_LOG} 2>&1"
        run_cmd "${cmd}"

        parse_algo ${TEST_LOG} "${expected_str}"
        check_log ${TEST_LOG}
        rm ${TEST_LOG}
    done
}

run_case "topo"
run_case "${expected_non_topo_algos}" "EnableCrossDeviceAccess=0"
if [[ ${PLATFORM_HW_DISCRETE_GPU} = "ats" ]]; then
    run_case "${expected_non_topo_algos}" "" "-g 1"
fi

echo "Pass"
