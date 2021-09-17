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

CCL_LINK="https://github.com/intel-innersource/libraries.performance.communication.oneccl.git"
CCL_BRANCH="master"

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"
DEFAULT_BUILD_CCL="0"
DEFAULT_RUN_BENCH="0"
DEFAULT_PROXY="http://proxy-us.intel.com:912"

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
    echo_log "  -build <bool_flag>"
    echo_log "      Build CCL"
    echo_log "  -run <bool_flag>"
    echo_log "      Run CCL benchmark with different configs"
    echo_log "  -p <url>"
    echo_log "      https proxy"
    echo_log ""
    echo_log "Usage example:"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}
    BUILD_CCL=${DEFAULT_BUILD_CCL}
    RUN_BENCH=${DEFAULT_RUN_BENCH}
    PROXY=${DEFAULT_PROXY}

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
                FULL_SCOPE=$2
                shift
                ;;
            "-build")
                BUILD_CCL=${2}
                shift
                ;;
            "-run")
                RUN_BENCH=${2}
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

    CCL_SRC_DIR=${SCRIPT_WORK_DIR}/ccl
    CCL_INSTALL_DIR=${CCL_SRC_DIR}/_install

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        BUILD_CCL="1"
        RUN_BENCH="1"

        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE      = ${FULL_SCOPE}"
    echo_log "BUILD_CCL       = ${BUILD_CCL}"
    echo_log "RUN_BENCH       = ${RUN_BENCH}"
    echo_log "PROXY           = ${PROXY}"
}

set_build_env() {
    module unload oneapi
    source /home/aanenkov/intel/oneapi/compiler/latest/env/vars.sh
}

check_build_env() {
    echo_log "\nDPCPP path:"
    echo_log `which dpcpp`

    echo_log "\nDPCPP version:"
    echo_log `dpcpp -v`
}

set_run_env() {
    module use -a /opt/hit/hpval/modulefiles
    module avail oneapi
    module load oneapi/2021.3.0.20210816
    source /home/aanenkov/intel/oneapi/compiler/latest/env/vars.sh

    ccl_vars_file="${CCL_INSTALL_DIR}/env/setvars.sh"
    if [[ -f ${ccl_vars_file} ]]
    then
        source ${ccl_vars_file}
    else
        check_exit_code 1 "Can not find CCL vars file: ${ccl_vars_file}"
    fi

    echo_log "CCL_ROOT: $CCL_ROOT"

    # MPICH
    module unload mpich/icc-cxi
    module use -a /home/tdoodi/drop42-release/install/modulefiles
    module load mpich/icc-cxi/42.2
    export LD_LIBRARY_PATH=/usr/lib64/:${LD_LIBRARY_PATH}

    # UMD with ULLS support
    module unload graphics-compute-runtime
    module use /home/ftartagl/graphics-compute-runtime/modulefilesmodule \
        load graphics-compute-runtime/ci-neo-020957
}

check_run_env() {
    level_zero_version=`rpm -qa | grep level-zero`
    echo_log "\nLevel Zero version:"
    echo_log "${level_zero_version}"

    driver_version=`clinfo -a | grep -i "driver version"`
    echo_log "\nDriver version:"
    echo_log "${driver_version}"

    device_list=`clinfo -l`
    echo_log "\nDevice list:"
    echo_log "${device_list}"

    cxi_state=`fi_info -p cxi -v | grep -i state | grep -i up | wc -l`
    if [ "${cxi_state}" != 8 ]
    then
        check_exit_code 1 "\nCXI check failed"
    else
        echo_log "\nCXI check passed"
    fi

    return

    # ANR
    check_log="anr_check.log"
    test_path="/home/mshiryae/shared/zello_ipc_copy_dma_buf_p2p.out"
    total_fails=0
    for first_idx in `seq 0 5`
    do
        for second_idx in `seq 0 5`
        do
            if [ "${first_idx}" == "${second_idx}" ]
            then
                continue
            fi
            cmd="ZE_AFFINITY_MASK=${first_idx}.0,${second_idx}.0 EnableCrossDeviceAccess=1"
            cmd="${cmd} ${test_path} > ${check_log} 2>&1"
            eval ${cmd}
            failed=$(cat ${check_log} | grep "FAILED" | wc -l)
            passed=$(cat ${check_log} | grep "PASSED" | wc -l)
            if [ "${failed}" == "1" ]
            then
                echo "config: ${first_idx}:${second_idx} failed"
                total_fails=$((total_fails + 1))
            fi
        done
    done

    if [ "${total_fails}" != "0" ]
    then
        check_exit_code 1 "\nANR check failed"
    else
        echo_log "\nANR check passed"
    fi
}

build_ccl() {
    if [[ -d ${CCL_SRC_DIR} ]]
    then
        rm -rf ${CCL_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}

    https_proxy=${PROXY} git clone --branch ${CCL_BRANCH} \
        --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
    check_exit_code $? "Download CCL failed"

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo_log "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-build 1\""
        check_exit_code 1 "Install CCL failed"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build

    cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp \
        -DCOMPUTE_BACKEND=dpcpp_level_zero \
        -DBUILD_CONFIG=0 -DBUILD_FT=0 -DBUILD_EXAMPLES=0 \
        -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR}
    check_exit_code $? "Configure CCL failed"

    make -j16 install
    check_exit_code $? "Install CCL failed"
}

run_bench() {
    #scales="2 12 24"
    scales="4"
    #algos="ring topo_ring"
    algos="topo_ring"
    #copy_engines="none main link"
    copy_engines="none"
    bench_params="-w 4 -i 16 -c off -t 8388608 -j off"
    hosts="-hosts c001n0001,c001n0002"

    base_env="CCL_LOG_LEVEL=debug FI_PROVIDER=cxi EnableDirectSubmission=1"
    base_env="CCL_LOG_LEVEL=debug FI_PROVIDER=cxi CCL_STAGING_BUFFER=regular"
    base_env="${base_env} SYCL_DEVICE_FILTER=level_zero"

    for scale in ${scales}
    do
        for algo in ${algos}
        do
            for copy_engine in ${copy_engines}
            do
                if [[ ${algo} = "ring" ]] && [[ ${copy_engine} != "none" ]]
                then
                    continue
                fi

                exec_env="${base_env} CCL_ALLREDUCE=${algo} CCL_ZE_COPY_ENGINE=${copy_engine}"
                cmd="${exec_env} mpirun -n ${scale} -ppn 12 -l ${hosts} ${CCL_ROOT}/examples/benchmark/benchmark ${bench_params}"
                run_cmd "${cmd}"
            done
        done
    done
}

parse_arguments $@

set_build_env
check_build_env

if [[ ${BUILD_CCL} = "1" ]]
then
    build_ccl
fi

if [[ ${RUN_BENCH} = "1" ]]
then
    set_run_env
    check_run_env
    run_bench
fi
