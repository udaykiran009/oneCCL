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

current_date=`date "+%Y_%m_%d_%H%M%S"`
LOG_FILE="${SCRIPT_DIR}/log_${current_date}.txt"
touch ${LOG_FILE}

CCL_LINK="https://github.com/intel-innersource/libraries.performance.communication.oneccl.git"
CCL_BRANCH="master"

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"
DEFAULT_BUILD_CCL="0"
DEFAULT_RUN_BENCH="0"
DEFAULT_CHECK_ANR="0"
DEFAULT_CHECK_CXI="0"
DEFAULT_PROXY="http://proxy-us.intel.com:912"

CURRENT_WORK_DIR=$(realpath ${SCRIPT_DIR}/../../../../)
source ${CURRENT_WORK_DIR}/scripts/utils/common.sh
source ${CURRENT_WORK_DIR}/scripts/utils/pvc_helper.sh

print_help() {
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -build <bool_flag>"
    echo_log "      Build CCL"
    echo_log "  -run < 1 | 2 | 3 >"
    echo_log "      1 - Run CCL benchmark with default config"
    echo_log "      2 - Run CCL benchmark with PVC 1 node perf config"
    echo_log "      3 - Run CCL benchmark with PVC 2 nodes perf config"
    echo_log "  -freq <frequency>"
    echo_log "      Sets a fixed frequency in megaherz on all tiles"
    echo_log "  -check_anr <bool_flag>"
    echo_log "      Perfom ANR links check"
    echo_log "  -check_cxi <bool_flag>"
    echo_log "      Perfom CXI cards check"
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
    CHECK_ANR=${DEFAULT_CHECK_ANR}
    CHECK_CXI=${DEFAULT_CHECK_CXI}
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
            "-freq")
                GPU_FREQUENCY="${2}"
                set_frequency
                exit 0
                ;;
            "-check_anr")
                CHECK_ANR=${2}
                shift
                ;;
            "-check_cxi")
                CHECK_CXI=${2}
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
        CHECK_ANR="1"
        CHECK_CXI="1"

        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    echo_log "CCL_ROOT: $CCL_ROOT"
    ccl_vars_file_1="${CCL_ROOT}/env/setvars.sh"
    ccl_vars_file_2="${CCL_INSTALL_DIR}/env/setvars.sh"

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE      = ${FULL_SCOPE}"
    echo_log "BUILD_CCL       = ${BUILD_CCL}"
    echo_log "RUN_BENCH       = ${RUN_BENCH}"
    echo_log "CHECK_ANR       = ${CHECK_ANR}"
    echo_log "CHECK_CXI       = ${CHECK_CXI}"
    echo_log "PROXY           = ${PROXY}"
}

set_frequency() {
    for freq_file in `ls /sys/class/drm/card*/gt/gt*/rps_m*_freq_mhz`
    do
        echo "${GPU_FREQUENCY}" | sudo tee ${freq_file} > /dev/null
        echo `echo ${freq_file} | awk -F/ '{print $5,$7,$8 " = "}' && cat ${freq_file}`
    done
}

check_build_env() {
    echo_log "\nDPCPP path:"
    echo_log `which dpcpp`

    echo_log "\nDPCPP version:"
    echo_log `dpcpp -v`
}

set_run_env() {
    set_build_env

    # CCL
    if [[ -f ${ccl_vars_file_1} ]]
    then
        source ${ccl_vars_file_1}
    elif [[ -f ${ccl_vars_file_2} ]]
    then
        source ${ccl_vars_file_2}
    else
        check_command_exit_code 1 "Can not find CCL vars file: ${ccl_vars_file_1} or ${ccl_vars_file_2}"
    fi

    # MPICH
    set_pvc_mpich_ofi_env

    # compute-runtime
    set_agama_env
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

    # sycl_ls=`sycl-ls`
    # echo_log "\nsycl-ls:"
    # echo_log "${sycl_ls}"

    mpich_version=`mpichversion | grep "MPICH Custom Information"`
    echo_log "\nmpich version:"
    echo_log "${mpich_version}"
}

build_ccl() {
    if [[ -d ${CCL_SRC_DIR} ]]
    then
        rm -rf ${CCL_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}

    https_proxy=${PROXY} git clone --branch ${CCL_BRANCH} \
        --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
    check_command_exit_code $? "Download CCL failed"

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo_log "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-build 1\""
        check_command_exit_code 1 "Install CCL failed"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build

    cmake .. -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp \
        -DCOMPUTE_BACKEND=dpcpp_level_zero \
        -DBUILD_CONFIG=0 -DBUILD_FT=0 -DBUILD_EXAMPLES=1 \
        -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR}
    check_command_exit_code $? "Configure CCL failed"

    make -j16 install
    check_command_exit_code $? "Install CCL failed"
}

run_bench() {

    bench_params="-w 4 -c last -j off"

    if [[ ${RUN_BENCH} = "1" ]]
    then
        node_counts="1 2"
        ppns="2 6 12"
        algos="topo ring"
        copy_engines="none main link"
        hosts="-hosts c001n0001,c001n0002"
        ulls_modes="1"
        runtimes="level_zero"
        colls="allreduce"
        cache_modes="1"
        bench_params+=" -i 20 -t 8388608 -d int32"
    fi

    if [[ ${RUN_BENCH} = "2" ]]
    then
        node_counts="1"
        ppns="2 4 12"
        algos="topo"
        copy_engines="none"
        hosts="-hosts c001n0001"
        ulls_modes="0 1"
        runtimes="level_zero"
        colls="reduce allreduce"
        cache_modes="0 1"
        bench_params+=" -i 100 -y 2097152"
    fi

    if [[ ${RUN_BENCH} = "3" ]]
    then
        node_counts="2"
        ppns="4"
        algos="topo"
        copy_engines="none"
        hosts="-hosts c001n0001,c001n0002"
        ulls_modes="0 1"
        runtimes="level_zero"
        colls="reduce allreduce"
        cache_modes="0 1"
        bench_params+=" -i 100 -y 2097152"
    fi

    base_env="FI_PROVIDER=cxi CCL_ATL_TRANSPORT=mpi"
    base_env+=" CCL_LOG_LEVEL=info I_MPI_DEBUG=12"
    base_env+=" CCL_MNIC=local CCL_MNIC_COUNT=4"
    base_env+=" EngineInstancedSubDevices=0"

    # https://jira.devtools.intel.com/browse/XDEPS-2195
    base_env+=" CCL_STAGING_BUFFER=regular"

    base_env+=" I_MPI_PIN_PROCESSOR_LIST=1-6,56-61 CCL_WORKER_AFFINITY=7-12,62-67"
    base_env+=" I_MPI_FABRICS=shm:ofi CCL_BARRIER=direct CCL_ATL_SYNC_COLL=1"

    for runtime in ${runtimes}
    do
        for coll in ${colls}
        do
            for cache_mode in ${cache_modes}
            do
                for ulls_mode in ${ulls_modes}
                do
                    for node_count in ${node_counts}
                    do
                        for ppn in ${ppns}
                        do
                            for algo in ${algos}
                            do
                                for copy_engine in ${copy_engines}
                                do
                                    if [[ ${algo} = "ring" ]] && [[ ${copy_engine} != "none" ]]
                                    then
                                        continue
                                    fi

                                    proc_count=$((node_count*ppn))

                                    exec_env="${base_env} CCL_ALLGATHERV=${algo}"
                                    exec_env+=" CCL_ALLREDUCE=${algo}"
                                    exec_env+=" CCL_REDUCE=${algo}"
                                    exec_env+=" CCL_ZE_COPY_ENGINE=${copy_engine}"
                                    exec_env+=" EnableDirectSubmission=${ulls_mode}"
                                    exec_env+=" SYCL_DEVICE_FILTER=${runtime}"

                                    cmd="${exec_env} ${CCL_ROOT}/bin/mpiexec"
                                    cmd+=" -n ${proc_count} -ppn ${ppn} -l ${hosts}"
                                    cmd+=" ${CCL_ROOT}/examples/benchmark/benchmark"
                                    cmd+=" ${bench_params} -p ${cache_mode} -l ${coll}"
                                    run_cmd "${cmd}"
                                done
                            done
                        done
                    done
                done
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

set_run_env
check_run_env

if [[ ${CHECK_ANR} = "1" ]]
then
    check_anr
fi

if [[ ${CHECK_CXI} = "1" ]]
then
    check_cxi
fi

if [[ ${RUN_BENCH} = "1" || ${RUN_BENCH} = "2" || ${RUN_BENCH} = "3" ]]
then
    run_bench
fi
