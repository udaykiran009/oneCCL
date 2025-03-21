#!/bin/bash

REG_TESTS_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

function run_cmd() {
    cmd="${1}"
    # echo "run_cmd: ${cmd}"
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
    extra_passed_pattern=${2:-}

    passed_pattern="passed|# all done"
    if [ "${extra_passed_pattern}" != "" ]
    then
        passed_pattern+="|${extra_passed_pattern}"
    fi
    passed_count=`grep -E -c -i "${passed_pattern}" ${log_path}`
    if [ ${passed_count} -eq 0 ]
    then
        echo "Error: did not find pass in log ${log_path}"
        exit 1
    fi

    failed_pattern="abort|^bad$|corrupt|fail|^fault$|[^-W]invalid"
    failed_pattern+="|kill|runtime_error|terminate|timed|unexpected"
    failed_pattern+="|[^-W]error|exception|connection refused"
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

    src_root_dir=$(realpath "${REG_TESTS_DIR}/../../")

    build_dir="build"
    cmake_str=""

    if [[ -z ${PV_ENVIRONMENT} ]]
    then
        examples_dir="${src_root_dir}/examples"
        benchmark_dir="${examples_dir}/benchmark"
    else
        examples_dir="${REG_TESTS_DIR}/../examples"
        benchmark_dir="${examples_dir}/build/benchmark"
    fi

    if [ ! -f ${benchmark_dir}/benchmark ]
    then
        cd ${examples_dir}

        if [ "${backend}" == "sycl" ]
        then
            build_dir="build_sycl"
            cmake_str="-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp"
        fi
        mkdir -p ${build_dir}
        cd ${build_dir}

        cmake .. $cmake_str &>> ${log_path}
        make benchmark &>> ${log_path}

        check_command_exit_code $? "Benchmark build failed"
        cp ${examples_dir}/${build_dir}/benchmark/benchmark ${dst_dir}
    else
        cp ${benchmark_dir}/benchmark ${dst_dir}
    fi
}

function get_default_bench_dtype() {
    echo "-d int32"
}

function get_default_prov() {
    if [[ ${PV_ENVIRONMENT} == "yes" ]]
    then
        echo "tcp"
    elif [[ ${TRANSPORT} == "mpich" ]]
    then
        echo "sockets"
    else
        echo "tcp"
    fi
}

function get_default_and_native_provs() {
    if [[ ${PV_ENVIRONMENT} == "yes" ]] || [[ ${TRANSPORT} == "mpich" ]]
    then
        echo "$(get_default_prov)"
    elif [[ ${PLATFORM_HW_GPU} == "ats" ]] || [[ -z ${PLATFORM_HW_GPU} ]]
    then
        echo "$(get_default_prov) psm3"
    else
        echo "$(get_default_prov)"
    fi
}

function get_default_and_ext_native_provs() {
    if [[ -z ${PLATFORM_HW_GPU} ]]
    then
        echo "$(get_default_and_native_provs) verbs"
    else
        echo "$(get_default_and_native_provs)"
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

get_ofi_path() {
    if [[ ${TRANSPORT} == "mpich" ]]
    then
        # taken from ats_* and pvc_helper.sh
        echo "${MPICH_LIBFABRIC_PATH}/libfabric.so.1"
    else
        echo "${I_MPI_ROOT}/libfabric/lib/libfabric.so.1"
    fi
}

get_mpi_path() {
    if [[ ${TRANSPORT} == "mpich" ]]
    then
        # get first libmpi.so.12 from LD_LIBRARY_PATH
        for dir in $(echo ${LD_LIBRARY_PATH}} | tr ":" "\n")
        do
            if [[ -f "${dir}/libmpi.so.12" ]]
            then
                echo "${dir}/libmpi.so.12"
                break
            fi
        done
    else
        echo "${I_MPI_ROOT}/lib/release/libmpi.so.12"
    fi
}
