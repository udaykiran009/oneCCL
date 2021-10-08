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
    echo_log "  -freq <frequency>"
    echo_log "      Sets a fixed frequency in megaherz on all tiles"
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
            "-freq")
                GPU_FREQUENCY="${2}"
                set_frequency
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
    echo_log "PROXY           = ${PROXY}"
}

set_frequency() {
    for freq_file in `ls /sys/class/drm/card*/gt/gt*/rps_m*_freq_mhz`
    do
        echo "${GPU_FREQUENCY}" | sudo tee ${freq_file} > /dev/null
        echo `echo ${freq_file} | awk -F/ '{print $5,$7,$8 " = "}' && cat ${freq_file}`
    done
}

set_build_env() {
    module unload oneapi
    module use /home/ftartagl/modulefiles
    module load oneapi-testing/2021.4.0.001.rc1.20211005
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
        check_exit_code 1 "Can not find CCL vars file: ${ccl_vars_file_1} or ${ccl_vars_file_2}"
    fi

    # MPICH
    module unload mpich/icc-cxi
    module use -a /home/tdoodi/drop42-release/install/modulefiles
    module load mpich/icc-cxi/42.2

    # TODO: add ATL/MPI changes to enable 43.x
    #module use -a /home/tdoodi/drop43-release/mpich-ofi/modulefiles
    #module load mpich/43.x

    export LD_LIBRARY_PATH=/usr/lib64/:${LD_LIBRARY_PATH}

    # UMD with ULLS fix 1 (for hang issue)
    #module use -a /opt/hit/hpval/modulefiles
    #module add umd/agama-ci-prerelease-207

    # UMD with ULLS fix 2 (for CCS > 0)
    module use -a /home/ftartagl/graphics-compute-runtime/modulefiles
    module load graphics-compute-runtime/ci-neo-021202

    # TODO: frequency script
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

    sycl_ls=`sycl-ls`
    echo_log "\nsycl-ls:"
    echo_log "${sycl_ls}"

    cxi_state=`fi_info -p cxi -v | grep -i state | grep -i up | wc -l`
    if [ "${cxi_state}" != 8 ]
    then
        check_exit_code 1 "\nCXI check failed\n"
    else
        echo_log "\nCXI check passed\n"
    fi

    return

    # ANR check
    check_log="anr_check.log"
    test_path="/home/mshiryae/shared/zello_ipc_copy_dma_buf_p2p.out"
    total_fails=0

    for tile_idx in `seq 0 1`
    do
        for first_card_idx in `seq 0 5`
        do
            for second_card_idx in `seq $((first_card_idx+1)) 5`
            do
                cmd="ZE_AFFINITY_MASK=${first_card_idx}.${tile_idx},${second_card_idx}.${tile_idx} EnableCrossDeviceAccess=1"
                cmd="${cmd} ${test_path} > ${check_log} 2>&1"
                echo "${cmd}"
                timeout 4s bash -c "eval ${cmd}"

                test_exit_code=$?
                failed=$(cat ${check_log} | grep "FAILED" | wc -l)
                passed=$(cat ${check_log} | grep "PASSED" | wc -l)

                if [ "${failed}" == "1" ] || [ "${test_exit_code}" != "0" ]
                then
                    echo "config: ${first_card_idx}.${tile_idx}:${second_card_idx}.${tile_idx} failed"
                    total_fails=$((total_fails + 1))
                fi
            done
        done
    done

    if [ "${total_fails}" != "0" ]
    then
        check_exit_code 1 "\nANR check failed\n"
    else
        echo_log "\nANR check passed\n"
    fi

    # hdr=y
    # for f in $(ls /sys/kernel/debug/iaf/iaf.*/sd.*/port.*/port_show)
    # do
    #     if [ $hdr == "y" ]
    #     then
    #         echo -n "PortName "
    #         cat ${f} | tr -d ' ' | cut -d: -f 1 | tr '\n' ' '
    #         hdr=n
    #         echo
    #     fi
    #     echo "${f}" | cut -d/ -f 6-8 | tr '\n' ' '
    #     cat ${f} | tr -d ' ' | cut -d: -f 2 | tr '\n' ' '
    #     echo
    # done | grep -e VARIABLE | grep -v -e HEALTHY
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
        -DBUILD_CONFIG=0 -DBUILD_FT=0 -DBUILD_EXAMPLES=1 \
        -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR}
    check_exit_code $? "Configure CCL failed"

    make -j16 install
    check_exit_code $? "Install CCL failed"
}

run_bench() {
    node_counts="1 2"
    ppns="2 6 12"
    algos="ring topo_ring"
    copy_engines="none main link"
    bench_params="-w 4 -i 16 -c last -t 33554432 -j off -q 0"
    hosts="-hosts c001n0001,c001n0002"

    base_env="FI_PROVIDER=cxi CCL_ATL_TRANSPORT=mpi"
    base_env+=" CCL_LOG_LEVEL=info I_MPI_DEBUG=12"

    # TODO: debug hang
    # base_env+=" CCL_MNIC=local CCL_MNIC_COUNT=3"

    # https://jira.devtools.intel.com/browse/XDEPS-2195
    base_env+=" CCL_STAGING_BUFFER=regular"

    base_env+=" I_MPI_PIN_PROCESSOR_LIST=1-6,56-61 CCL_WORKER_AFFINITY=7-12,62-67"
    base_env+=" SYCL_DEVICE_FILTER=level_zero"
    base_env+=" EnableDirectSubmission=1"

    # TODO: enable fast barrier
    #base_env+=" I_MPI_FABRICS=shm:ofi CCL_BARRIER=direct CCL_ATL_SYNC_COLL=1"

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

                    exec_env="${base_env} CCL_ALLREDUCE=${algo} CCL_ZE_COPY_ENGINE=${copy_engine}"
                    cmd="${exec_env} ${CCL_ROOT}/bin/mpirun -n ${proc_count} -ppn ${ppn} -l ${hosts} ${CCL_ROOT}/examples/benchmark/benchmark ${bench_params}"
                    run_cmd "${cmd}"
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

if [[ ${RUN_BENCH} = "1" ]]
then
    set_run_env
    check_run_env
    run_bench
fi
