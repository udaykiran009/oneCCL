#!/bin/bash

BASENAME=`basename $0 .sh`
current_date=`date "+%Y_%m_%d_%H%M%S"`
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

CURRENT_WORK_DIR=$(realpath ${SCRIPT_DIR}/../../../../)
source ${CURRENT_WORK_DIR}/scripts/utils/common.sh
source ${CURRENT_WORK_DIR}/scripts/utils/pvc_helper.sh

print_help() {
    echo "Usage: ${BASENAME}.sh <options>"
    echo "  -work_dir <dir>"
    echo "      Use existing work directory"
    echo "  -ccl_path <path>"
    echo "      Use for set oneCCL source path"
    echo "      If CCL_ROOT is set, try to use this path first"
    echo "  -full"
    echo "      Run full scope of steps"
    echo "  -build"
    echo "      Build CCL"
    echo "  -run"
    echo "      Run CCL benchmark"
    echo "  -check_anr"
    echo "      Perfom ANR links check"
    echo "  -check_cxi"
    echo "      Perfom CXI cards check"
    echo "  -p <url>"
    echo "      https proxy"
    echo "  -colls <values>"
    echo "      Set colls values"
    echo "  -cache_modes <values>"
    echo "      Set cache_modes values"
    echo "  -ulls_modes <values>"
    echo "      Set ulls_modes values"
    echo "  -proc_maps <n:ppns>"
    echo "      Set values for n and ppns"
    echo "  -algos <values>"
    echo "      Set algos values"
    echo "  -hosts <values>"
    echo "      Set hosts values"
    echo "  -bench_params <params>"
    echo "      Set benchmark params"
    echo "  -freq <frequency>"
    echo "      Sets a fixed frequency in megahertz on all tiles"
    echo "  -config <1|2|3>"
    echo "      Set config"
    echo "      1 - Run CCL benchmark with default config"
    echo "      2 - Run CCL benchmark with PVC 1 node performance config"
    echo "      3 - Run CCL benchmark with PVC 2 nodes performance config"
    echo "  -agama <version>"
    echo "      Sets agama version for benchmark run"
    echo ""
    echo "Usage examples:"
    echo "  ${BASENAME}.sh -build"
    echo "  ${BASENAME}.sh -work_dir <dir> -run -config 2"
    echo "  ${BASENAME}.sh -work_dir <dir> -run -freq 1000 -hosts c001n0002"
    echo "  ${BASENAME}.sh -work_dir <dir> -run -hosts c001n0001,c001n0002 -proc_maps 2:2/4:2 -bench_params \"-i 20 -t 8388608\""
    echo ""
}

remove_commas() {
    echo "`echo ${1} | tr ',' ' '`"
}

set_default_values() {
    CCL_LINK="https://github.com/intel-innersource/libraries.performance.communication.oneccl.git"
    CCL_BRANCH="master"
    LOG_FILE="${SCRIPT_DIR}/log_${current_date}.txt"

    SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir"
    CCL_PATH="${SCRIPT_WORK_DIR}/build/_install"
    BUILD_CCL="no"
    RUN_BENCH="no"
    CHECK_ANR="no"
    CHECK_CXI="no"
    PROXY="http://proxy-us.intel.com:912"

    #run_bench params
    COLLS="reduce"
    CACHE_MODES="1"
    ULLS_MODES="1"
    PROC_MAPS="2:2"
    ALGOS="topo"
    COPY_ENGINE="auto"

    HOSTS="c001n0001"
    BENCH_PARAMS=""
    AGAMA="308"
}

parse_arguments() {

    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")        print_help ; exit 0 ;;
            "-work_dir")                      SCRIPT_WORK_DIR="${2}" ; shift ;;
            "-ccl_path")                      CCL_PATH="${2}" ; shift ;;
            "-full")                          BUILD_CCL="yes" ; RUN_BENCH="yes" ; CHECK_ANR="yes" ; CHECK_CXI="yes" ;;
            "-build")                         BUILD_CCL="yes" ;;
            "-run")                           RUN_BENCH="yes" ;;
            "-check_anr")                     CHECK_ANR="yes" ;;
            "-check_cxi")                     CHECK_CXI="yes" ;;
            "-proxy")                         PROXY="${2}" ; shift ;;
            "-colls")                         COLLS=`remove_commas ${2}` ; shift ;;
            "-cache_modes")                   CACHE_MODES=`remove_commas ${2}` ; shift ;;
            "-ulls_modes")                    ULLS_MODES=`remove_commas ${2}` ; shift ;;
            "-proc_maps")                     PROC_MAPS="${2}" ; shift ;;
            "-algos")                         ALGOS=`remove_commas ${2}` ; shift ;;
            "-hosts")                         HOSTS="${2}" ; shift ;;
            "-bench_params")                  BENCH_PARAMS="${2}" ; shift ;;
            "-freq")                          GPU_FREQUENCY="${2}" ; set_frequency ; shift ;;
            "-config")                        RUN_BENCH="yes" ; CONFIG="${2}" ; set_config ; shift ;;
            "-agama")                         AGAMA="${2}" ; shift ;;
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    touch ${LOG_FILE}

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR = ${SCRIPT_WORK_DIR}"
    echo_log "CCL_PATH        = ${CCL_PATH}"
    echo_log "BUILD_CCL       = ${BUILD_CCL}"
    echo_log "RUN_BENCH       = ${RUN_BENCH}"
    echo_log "CHECK_ANR       = ${CHECK_ANR}"
    echo_log "CHECK_CXI       = ${CHECK_CXI}"
    echo_log "PROXY           = ${PROXY}"
    echo_log "-----------------------------------------------------------"
    echo_log "COLLS           = ${COLLS}"
    echo_log "CACHE_MODES     = ${CACHE_MODES}"
    echo_log "ULLS_MODES      = ${ULLS_MODES}"
    echo_log "PROC_MAPS       = ${PROC_MAPS}"
    echo_log "ALGOS           = ${ALGOS}"
    echo_log "HOSTS           = ${HOSTS}"
    echo_log "BENCH_PARAMS    = ${BENCH_PARAMS}"
    echo_log "FREQ            = ${GPU_FREQUENCY}"
    echo_log "CONFIG          = ${CONFIG}"
    echo_log "AGAMA           = ${AGAMA}"
    echo_log "-----------------------------------------------------------"
}

set_config() {

    if [[ -z ${CONFIG} ]]
    then
        return
    elif [[ ${CONFIG} = "1" ]]
    then
        COLLS="allreduce"
        CACHE_MODES="1"
        ULLS_MODES="1"
        PROC_MAPS="4:2/12:6/24:12"
        ALGOS="topo ring"
        HOSTS="c001n0001,c001n0002"
        BENCH_PARAMS="-w 4 -c last -j off -i 20 -t 8388608 -d int32"
    elif [[ ${CONFIG} = "2" ]]
    then
        COLLS="reduce allreduce allgatherv alltoallv"
        CACHE_MODES="0 1"
        ULLS_MODES="0 1"
        PROC_MAPS="2:2/4:4/12:12"
        ALGOS="topo"
        HOSTS="c001n0001"
        BENCH_PARAMS="-w 4 -c last -j off -i 100 -y 2097152"
    elif [[ ${CONFIG} = "3" ]]
    then
        COLLS="reduce allreduce allgatherv alltoallv"
        CACHE_MODES="0 1"
        ULLS_MODES="0 1"
        PROC_MAPS="8:4/24:12"
        ALGOS="topo"
        HOSTS="c001n0001,c001n0002"
        BENCH_PARAMS="-w 4 -c last -j off -i 100 -y 2097152"
    else
        echo_log "ERROR: unknown config: ${CONFIG}"
        print_help
        exit 1
    fi
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
    check_build_env

    if [[ -f ${CCL_ROOT}/env/setvars.sh ]]
    then
        source ${CCL_ROOT}/env/setvars.sh
        echo_log "ccl source path: ${CCL_ROOT}/env/setvars.sh"
    elif [[ -f ${CCL_PATH}/env/setvars.sh ]]
    then
        source ${CCL_PATH}/env/setvars.sh
        echo_log "ccl source path: ${CCL_PATH}/env/setvars.sh"
    else
        check_command_exit_code 1 "Can not find CCL setvars.sh file.\n \
         CCL_PATH: ${CCL_PATH}/env/setvars.sh\n \
         CCL_ROOT: ${CCL_ROOT}/env/setvars.sh"
    fi

    # MPICH
    set_pvc_mpich_ofi_env

    # compute-runtime
    set_pvc_agama_env ${AGAMA}
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

    if [[ ${BUILD_CCL} = "no" ]]
    then
        return
    fi

    set_build_env
    check_build_env

    if [[ -d ${SCRIPT_WORK_DIR} ]]
    then
        rm -rf ${SCRIPT_WORK_DIR}
    fi

    https_proxy=${PROXY} git clone --branch ${CCL_BRANCH} \
        --single-branch ${CCL_LINK} ${SCRIPT_WORK_DIR}
    check_command_exit_code $? "Download CCL failed"

    if [[ ! -d ${SCRIPT_WORK_DIR} ]]
    then
        echo_log "ERROR: SCRIPT_WORK_DIR (${SCRIPT_WORK_DIR}) is not directory, try to run script with \"-build\""
        check_command_exit_code 1 "Install CCL failed"
    fi

    cd ${SCRIPT_WORK_DIR}

    rm -rf build
    mkdir build
    cd build

    cmake .. -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp \
        -DCOMPUTE_BACKEND=dpcpp \
        -DBUILD_CONFIG=0 -DBUILD_FT=0 -DBUILD_EXAMPLES=1
    check_command_exit_code $? "Configure CCL failed"

    make -j16 install
    check_command_exit_code $? "Install CCL failed"
}

run_bench() {

    if [[ ${RUN_BENCH} = "no" ]]
    then
        return
    fi

    set_run_env
    check_run_env

    base_env="FI_PROVIDER=cxi CCL_ATL_TRANSPORT=mpi"
    base_env+=" CCL_LOG_LEVEL=info I_MPI_DEBUG=12"
    base_env+=" CCL_MNIC=local CCL_MNIC_COUNT=4"
    base_env+=" EngineInstancedSubDevices=0"
    base_env+=" I_MPI_PIN_PROCESSOR_LIST=1-6,56-61 CCL_WORKER_AFFINITY=7-12,62-67"
    base_env+=" I_MPI_FABRICS=shm:ofi CCL_BARRIER=direct CCL_ATL_SYNC_COLL=1"

    for proc_map in `echo ${PROC_MAPS} | tr '/' ' '`
    do

        n=`echo ${proc_map%:*}`
        ppns=`echo ${proc_map#*:} | tr ',' ' '`

        for coll in ${COLLS}
        do
            for cache_mode in ${CACHE_MODES}
            do
                for ulls_mode in ${ULLS_MODES}
                do
                    for ppn in ${ppns}
                    do
                        for algo in ${ALGOS}
                        do

                            extra_env=""

                            if [[ ${algo} = "ring" ]] && [[ ${COPY_ENGINE} != "none" ]]
                            then
                                continue
                            fi

                            if [[ ${coll} = "alltoall"* ]]
                            then
                                extra_env="CCL_ZE_BIDIR_ALGO=1"
                            fi

                            exec_env="${base_env} ${extra_env}"
                            exec_env+=" CCL_ALLGATHERV=${algo}"
                            exec_env+=" CCL_ALLREDUCE=${algo}"
                            exec_env+=" CCL_REDUCE=${algo}"
                            exec_env+=" CCL_ZE_COPY_ENGINE=${COPY_ENGINE}"
                            exec_env+=" EnableDirectSubmission=${ulls_mode}"
                            exec_env+=" SYCL_DEVICE_FILTER=level_zero"

                            cmd="${exec_env} ${CCL_ROOT}/bin/mpiexec"
                            cmd+=" -n ${n} -ppn ${ppn} -l -hosts ${HOSTS}"
                            cmd+=" ${CCL_ROOT}/examples/benchmark/benchmark"
                            cmd+=" ${BENCH_PARAMS} -p ${cache_mode} -l ${coll}"
                            run_cmd "${cmd}"
                        done
                    done
                done
            done
        done
    done
}

set_default_values
parse_arguments "$@"
build_ccl

if [[ ${CHECK_ANR} = "yes" ]]
then
    check_anr
fi

if [[ ${CHECK_CXI} = "yes" ]]
then
    check_cxi
fi

run_bench
