#!/bin/bash

readonly SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

#For RC2, we use release only build.
#But since RC3 release we can use ${build_type} in ABI_OLD_LIB_PATH and ABI_NEW_LIB_PATH.
readonly ABI_OLD_LIB_PATH=/p/pdsd/scratch/Uploads/RegularTesting/CCL/l_ccl_release__2021.1.0_RC_ww45.20201105.212942

if [[ -z "$ABI_NEW_LIB_PATH" ]]; then
    ABI_NEW_LIB_PATH=/p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/l_ccl_release*
    echo "WARNING: ABI_NEW_LIB_PATH is not defined, will be used default: $ABI_NEW_LIB_PATH"
fi

if [[ -z "$ABI_EXAMPLES_BUILDSPACE" ]]; then
    ABI_EXAMPLES_BUILDSPACE=$SCRIPT_DIR/../../buildspace
    echo "WARNING: ABI_EXAMPLES_BUILDSPACE is not defined, will be used default: $ABI_EXAMPLES_BUILDSPACE"
fi

readonly EXIT_FAILURE=-1

check_command_exit_code() {
    if [ ${1} -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $EXIT_FAILURE
    fi
}

prepare_files()
{
    rm -rf $ABI_EXAMPLES_BUILDSPACE
    mkdir -p $ABI_EXAMPLES_BUILDSPACE

    if [ ! -d "$ABI_EXAMPLES_BUILDSPACE" ]; then
        echo "$ABI_EXAMPLES_BUILDSPACE doesn't exist"
        exit $EXIT_FAILURE
    fi

    cd $ABI_EXAMPLES_BUILDSPACE
    cp -r $ABI_OLD_LIB_PATH/examples/* $ABI_EXAMPLES_BUILDSPACE/

    #For RC2, we copy the run.sh script into the examples directory,
    #which is not entirely correct. Since RC3 release we can use the run.sh script from it.
    cp ../examples/run.sh $ABI_EXAMPLES_BUILDSPACE/
}

set_common_env()
{
    local config=$1

    if [[ $config == "cpu_icc" ]]; then
        source /nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/bin/compilervars.sh intel64
        BUILD_COMPILER=/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/bin/intel64/
        C_COMPILER=${BUILD_COMPILER}/icc
        CXX_COMPILER=${BUILD_COMPILER}/icpc
    else
        if [ -z "${SYCL_BUNDLE_ROOT}" ]; then
            SYCL_BUNDLE_ROOT="/p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/linux/"
            echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: $SYCL_BUNDLE_ROOT"
        fi
        source ${SYCL_BUNDLE_ROOT}/../../../setvars.sh --force
        BUILD_COMPILER=${SYCL_BUNDLE_ROOT}/compiler/latest/linux/bin
        C_COMPILER=${BUILD_COMPILER}/clang
        CXX_COMPILER=${BUILD_COMPILER}/clang++
    fi

    source /p/pdsd/scratch/Uploads/CCL_oneAPI/mpi_oneapi/last/mpi/latest/env/vars.sh -i_mpi_library_kind=release_mt
}

reset_previous_env()
{
    LD_LIBRARY_PATH=""
}

build_examples()
{
    local config=$1
    echo "Building examples ($config)..."

    reset_previous_env
    set_common_env $config
    source $ABI_OLD_LIB_PATH/env/vars.sh --ccl-configuration=$config

    local run_mode=gpu
    [[ $config == "cpu_icc" ]] && run_mode=cpu
    $ABI_EXAMPLES_BUILDSPACE/run.sh --mode $run_mode --build-only
    check_command_exit_code $? "examples build failed"
}

run_examples()
{
    local config=$1
    LOG_FILE_PATH="$ABI_EXAMPLES_BUILDSPACE/abi_$config.log"
    echo "Running examples ($config)..."

    reset_previous_env
    set_common_env $config
    source $ABI_NEW_LIB_PATH/env/vars.sh --ccl-configuration=$config

    local run_mode=gpu
    [[ $config == "cpu_icc" ]] && run_mode=cpu
    log $ABI_EXAMPLES_BUILDSPACE/run.sh --mode $run_mode --scope abi --run-only
}

log()
{
    eval $* 2>&1 | tee -a $LOG_FILE_PATH
}

check_result()
{
    [[ ! -f $LOG_FILE_PATH ]] && echo "File $LOG_FILE_PATH not found"

    UNDEFINED=`cat $LOG_FILE_PATH | grep "undefined symbol"`
    if [[ ! -z "$UNDEFINED" ]]; then
        log echo
        log echo "There are undefined symbols:"
        log echo "$UNDEFINED"
        log echo "ABI TEST FAILED"
        log echo
        exit $EXIT_FAILURE
    else
        log echo
        log echo "ABI TEST PASSED"
        log echo
    fi
}

prepare_files

build_examples cpu_gpu_dpcpp
run_examples cpu_gpu_dpcpp
check_result

build_examples cpu_icc
run_examples cpu_icc
check_result
