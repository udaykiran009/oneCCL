function check_impi() {
    if [[ -z "${I_MPI_ROOT}" ]] ; then
        echo "Error: \$I_MPI_ROOT is not set. Please source vars.sh"
        exit 1
    fi
}

function check_ccl() {
    if [[ -z "${CCL_ROOT}" ]]; then
        echo "Error: \$CCL_ROOT is not set. Please source vars.sh"
        exit 1
    fi
}

function check_log() {
    log_path=$1
    failed_pattern="abort|^bad$|corrupt|fail|^fault$|invalid|kill"
    failed_pattern+="|runtime_error|terminate|timed|unexpected|error|exception"
    failed_count=`grep -E -c -i "${failed_pattern}" ${log_path}`
    if [ ${failed_count} -ne 0 ]
    then
        echo "Error: found error in log ${log_path}"
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
            cmake_str="-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp_level_zero"
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
