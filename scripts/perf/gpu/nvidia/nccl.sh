#!/bin/bash

BASENAME=`basename $0 .sh`
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
else
    echo "SCRIPT_DIR: ${SCRIPT_DIR}"
fi

current_date=`date "+%Y%m%d%H%M%S"`
LOG_FILE="${SCRIPT_DIR}/log_${current_date}.txt"
touch ${LOG_FILE}

NCCL_LINK="https://github.com/NVIDIA/nccl.git"
NCCL_TESTS_LINK="https://github.com/NVIDIA/nccl-tests.git"
NCCL_SRC_DIR="nccl"
TESTS_SRC_DIR="nccl-tests"

SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
FULL_SCOPE="0"
BUILD_NCCL="0"
BUILD_TESTS="0"
DOWNLOAD_NCCL="0"
DOWNLOAD_TESTS="0"
PROXY=""
CUDA=""
MPI=""
NCCL=""


echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

check_exit_code() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}"
        exit $1
    fi
}

run_cmd() {
    echo_log ${1}
    eval ${1}
    check_exit_code $? "cmd failed"
}

print_help() {
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -cuda <cuda_home>"
    echo_log "      Path to CUDA"
    echo_log "  -mpi <mpi_home>"
    echo_log "      Path to MPI"
    echo_log "  -nccl <nccl_home>"
    echo_log "      Path to NCCL"
    echo_log "  -download_nccl <bool_flag>"
    echo_log "      Download NCCL"
    echo_log "  -download_tests <bool_flag>"
    echo_log "      Download NCCL tests"
    echo_log "  -build_nccl <bool_flag>"
    echo_log "      Build NCCL"
    echo_log "  -build_tests <bool_flag>"
    echo_log "      Build NCCL tests"
    echo_log "  -run <bool_flag>"
    echo_log "      Run NCCL allreduce tests"
    echo_log "  -p <url>"
    echo_log "      https proxy"
    echo_log ""
    echo_log "Usage example:"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-work_dir")
                SCRIPT_WORK_DIR=${2}
                shift
                ;;
            "-full")
                FULL_SCOPE=${2}
                shift
                ;;
            "-cuda")
                CUDA=${2}
                shift
                ;;
            "-mpi")
                MPI=${2}
                shift
                ;;
            "-nccl")
                NCCL=${2}
                shift
                ;;
            "-dowload_nccl")
                DOWNLOAD_NCCL=${2}
                shift
                ;;
            "-download_tests")
                DOWNLOAD_TESTS=${2}
                shift
                ;;
            "-build_nccl")
                BUILD_NCCL=${2}
                shift
                ;;
            "-build_tests")
                BUILD_TESTS=${2}
                shift
                ;;
            "-run")
                RUN_TESTS=${2}
                shift
                ;;
            "-proxy")
                PROXY="${2}"
                shift
                ;;
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    mkdir -p ${SCRIPT_WORK_DIR}

    if [[ "${FULL_SCOPE}" == "1" ]]
    then
        DOWNLOAD_NCCL="1"
        DOWNLOAD_TESTS="1"
        BUILD_NCCL="1"
        BUILD_TESTS="1"
        RUN_TESTS="1"

        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    if [[ "${CUDA}" == "" ]]
    then
        echo_log "ERROR: CUDA path must be set"
        print_help
        check_exit_code 1 "Parameters parsing failed"
    fi

    if [ "${MPI}" == "" ] && [ ${BUILD_TESTS} == "1" ]
    then
       echo_log "ERROR: MPI path must be set"
       print_help
       check_exit_code 1 "Parameters parsing failed"
    fi

    if [ "${NCCL}" == "" ] && [ ${BUILD_NCCL} != "1" ]
    then
       echo_log "ERROR: NCCL path must be set when -build_nccl option is not used"
       print_help
       check_exit_code 1 "Parameters parsing failed"
    fi 
 
    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE      = ${FULL_SCOPE}"
    echo_log "CUDA            = ${CUDA}"
    echo_log "MPI             = ${MPI}"
    echo_log "NCCL            = ${NCCL}"
    echo_log "DOWNLOAD_NCCL   = ${DOWNLOAD_NCCL}"
    echo_log "DOWNLOAD_TESTS  = ${DOWNLOAD_TESTS}"
    echo_log "BUILD_NCCL      = ${BUILD_NCCL}"
    echo_log "BUILD_TESTS     = ${BUILD_TESTS}"
    echo_log "RUN_TESTS       = ${RUN_TESTS}"
    echo_log "PROXY           = ${PROXY}"
}

download_nccl() {
    if [[ "${DOWNLOAD_NCCL}" != "1" ]]
    then
         return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ -d ${NCCL_SRC_DIR} ]]
    then
         rm -rf ${NCCL_SRC_DIR}
    fi

    git clone ${NCCL_LINK} ${NCCL_SRC_DIR}
    check_exit_code $? "Download NCCL failed"

    cd ..
}


build_nccl() {
    if [[ "${BUILD_NCCL}" != "1" ]]
    then
         return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ ! -d ${NCCL_SRC_DIR} ]]
    then
        echo_log "ERROR: NCCL_SRC_DIR (${NCCL_SRC_DIR}) is not directory, try to run script with \"-download_nccl 1\""
        check_exit_code 1 "Install NCCL failed"
    fi

    cd ${NCCL_SRC_DIR}

    if [[ "${CUDA}" = "" || ! -d ${CUDA} ]]
    then
        echo_log "ERROR: CUDA path is not valid, try to run script with \"-cuda <cuda_home>\""
        check_exit_code 1 "Install NCCL failed"
    fi

    rm -rf build
    make -j src.build CUDA_HOME=${CUDA}
    check_exit_code $? "Install NCCL failed"

    CUR=`pwd`
    NCCL=$CUR/build
    export LD_LIBRARY_PATH=${NCCL}/lib:$LD_LIBRARY_PATH

    cd ../..
}

download_tests() {
    if [[ "${DOWNLOAD_TESTS}" != "1" ]]
    then
         return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ -d ${TESTS_SRC_DIR} ]]
    then
         rm -rf ${TESTS_SRC_DIR}
    fi

    git clone ${NCCL_TESTS_LINK} ${TESTS_SRC_DIR}
    check_exit_code $? "Download NCCL tests failed"

    cd ..
}

build_tests() {
    if [[ "${BUILD_TESTS}" != "1" ]]
    then
         return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ ! -d ${TESTS_SRC_DIR} ]]
    then
        echo_log "ERROR: TESTS_SRC_DIR (${TESTS_SRC_DIR}) is not directory, try to run script with \"-download_tests 1\""
        check_exit_code 1 "Install NCCL tests failed"
    fi

    cd ${TESTS_SRC_DIR}

    if [[ "${CUDA}" = "" || ! -d ${CUDA} ]]
    then
        echo_log "ERROR: CUDA path is not valid, try to run script with \"-cuda <cuda_home>\""
        check_exit_code 1 "Install NCCL tests failed"
    fi

    if [[ "${MPI}" = "" || ! -d ${MPI} ]]
    then
        echo_log "ERROR: MPI path is not valid, try to run script with \"-mpi <mpi_home>\""
        check_exit_code 1 "Install NCCL tests failed"
    fi

    if [[ "${NCCL}" = "" || ! -d ${NCCL} ]]
    then
        echo_log "ERROR: NCCL path is not valid, try to run script with \"-nccl <nccl_home>\" or \"-build_nccl 1\""
        check_exit_code 1 "Install NCCL tests failed"
    fi

    make NCCL_HOME=${NCCL} CUDA_HOME=${CUDA} MPI=1 MPI_HOME=${MPI}
    check_exit_code $? "Install NCCL tests failed"

    cd ../..
}

run_tests() {
    if [[ "${RUN_TESTS}" != "1" ]]
    then
         return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ ! -d ${TESTS_SRC_DIR} ]]
    then
        echo_log "ERROR: TESTS_SRC_DIR (${TESTS_SRC_DIR}) is not directory, try to run script with \"-download_tests 1\""
        check_exit_code 1 "Run NCCL tests failed"
    fi

    cd ${TESTS_SRC_DIR}
    FI_PROVIDER=tcp mpirun -n 2 ./build/all_reduce_perf -b 8 -e 256M -f 2 -g 1
    check_exit_code $? "Run NCCL tests CCL failed"

    cd ../..
}

parse_arguments $@
download_nccl
download_tests
build_nccl
build_tests
run_tests
