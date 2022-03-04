#!/bin/bash

function run_cmd() {
    cmd="${1}"
    eval ${cmd}
    rc=$?
    if [ ${rc} -ne 0 ]
    then
        echo "run_cmd: ${cmd}"
        echo "Fail"
        exit 1
    fi
}

function check_impi() {
    if [[ -z "${I_MPI_ROOT}" ]]
    then
        which mpiexec.hydra >/dev/null 2>&1
        if [[ $? != 0 ]]
        then
            echo "Error: MPI not found."
            exit 1
        fi
     fi
}

function check_ccl() {
    if [[ -z "${CCL_ROOT}" ]]
    then
        echo "Error: \$CCL_ROOT is not set. Please source vars.sh"
        exit 1
    fi
}

function check_log() {
    log_path=$1

    passed_pattern="passed|# all done"
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [ ${passed_count} -eq 0 ]
    then
        echo "Error: did not find pass in log ${log_path}"
        exit 1
    fi

    failed_pattern="abort|^bad$|corrupt|fail|^fault$|[^-W]invalid"
    failed_pattern+="|kill|runtime_error|terminate|timed|unexpected"
    failed_pattern+="|[^-W]error|exception"
    failed_pattern+="|job ending due to application timeout"

    # patterns to exclude as they're printed in non-error cases
    exclude_pattern="\-\-abort\-signal|CCL_ABORT_ON_THROW"
    exclude_pattern+="|fi_strerror|MPI_Error_string|fake-path"

    failed_strings=`grep -E -i "${failed_pattern}" ${log_path} | grep -Ev "${exclude_pattern}"`
    if [ "${failed_strings}" != "" ]
    then
        echo "Error: found error in log ${log_path}"
        echo ""
        echo "${failed_strings}"
        echo ""
        exit 1
    fi
}

function check_command_exit_code() {
    if [ ${1} -ne 0 ]
    then
        echo "ERROR: ${2}"
        exit ${1}
    fi
}

check_ret() {
    rc=$1
    if [[ $rc -ne 0 ]]
    then
        echo "Fail"
        exit 1
    fi
}

function get_bench() {
    dst_dir=$1
    log_path=$2
    backend=$3

    build_dir="build"
    cmake_str=""

    if [ ! -f ${CCL_ROOT}/examples/benchmark/benchmark ]
    then
        cd ${CCL_ROOT}/examples
        if [ "${backend}" == "sycl" ]
        then
            build_dir="build_sycl"
            cmake_str="-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp"
        fi
        mkdir -p ${build_dir}
        cd ${build_dir}

        cmake .. $cmake_str &>> ${log_path}
        make benchmark &>> ${log_path}

        check_command_exit_code $? "Benchmark build failed"
        cp ${CCL_ROOT}/examples/${build_dir}/benchmark/benchmark ${dst_dir}
    else
        cp ${CCL_ROOT}/examples/benchmark/benchmark ${dst_dir}
    fi
}

function get_default_bench_dtype() {
    echo "-d int32"
}

function get_default_prov() {
    if [[ ${TRANSPORT} == "mpich" ]]
    then
        echo "sockets"
    else
        echo "tcp"
    fi
}

function get_default_and_native_provs() {
    if [[ ${TRANSPORT} == "mpich" ]]
    then
        echo "$(get_default_prov)"
    elif [[ ${PLATFORM_HW_DISCRETE_GPU} == "ats" ]]
    then
        echo "$(get_default_prov) psm3"
    else
        echo "$(get_default_prov)"
    fi
}

function check_hmem_log() {
    log_path=$1
    hmem_mode=$2
    passed_pattern="in: { .* hmem: ${hmem_mode}"
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [[ ${passed_count} -eq 0 ]]
    then
        echo "Error: did not find input hmem enable in log ${log_path}"
        exit 1
    fi
    passed_pattern="out: { .* hmem: ${hmem_mode}"
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [[ ${passed_count} -eq 0 ]]
    then
        echo "Error: did not find output hmem enable in log ${log_path}"
        exit 1
    fi
}

make_common_actions() {
    work_dir=$1
    log_path=$2
    bench_backend=${3:-}

    check_impi
    check_ccl
    get_bench ${work_dir} ${log_path} ${bench_backend}

    cd ${work_dir}
}

create_ze_affinity_env() {
    device_list=$1

    result_env=""
    counter=0

    for device in ${device_list}
    do
        result_env+="env ZE_AFFINITY_MASK=${device}:${counter};"
        counter=$((counter + 1))
    done

    echo "I_MPI_GTOOL=\"$result_env"\"
}
