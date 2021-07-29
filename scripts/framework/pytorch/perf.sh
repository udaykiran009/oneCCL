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
LOG_FILE="${SCRIPT_DIR}/perf_log_${current_date}"
touch ${LOG_FILE}

CCL_ENV="CCL_LOG_LEVEL=info CCL_WORKER_AFFINITY=1-8 CCL_ATL_TRANSPORT=ofi"
CCL_ENV="${CCL_ENV} CCL_ALLREDUCE=ring CCL_MAX_SHORT_SIZE=16384"
MPI_ENV="I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=9"
PSM3_ENV="PSM3_MULTI_EP=1 PSM3_RDMA=2 PSM3_MR_CACHE_MODE=2"
BASE_ENV="${CCL_ENV} ${MPI_ENV} ${PSM3_ENV}"

PROVS="verbs psm3"
NODE_COUNTS="2 4 8 16"
WORKER_COUNTS="1 2 4 8"
COLLS="allreduce alltoall"
NUMA_NODES="0 1"

CCL_LINK="https://gitlab.devtools.intel.com/ict/ccl-team/ccl.git"
CCL_BRANCH="master"

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"
DEFAULT_DOWNLOAD_CCL="0"
DEFAULT_INSTALL_CCL="0"
DEFAULT_RUN_TESTS="0"

echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

check_exit_code() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}" 1>&2
        echo_log "SCRIPT FAILED"
        exit $1
    fi
}

print_help() {
    echo_log ""
    echo_log "Requirements:"
    echo_log "- export I_MPI_HYDRA_HOST_FILE=<file>"
    echo_log "  <file> should be path to host file, one unique host per line"
    echo_log ""
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -download_ccl <bool_flag>"
    echo_log "      Download oneCCL"
    echo_log "  -install_ccl <bool_flag>"
    echo_log "      Install oneCCL"
    echo_log "  -run_tests <bool_flag>"
    echo_log "      Run CCL benchmark with different options"
    echo_log ""
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}

    DOWNLOAD_CCL=${DEFAULT_DOWNLOAD_CCL}
    INSTALL_CCL=${DEFAULT_INSTALL_CCL}

    RUN_TESTS=${DEFAULT_RUN_TESTS}

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
            "-download_ccl")
                DOWNLOAD_CCL=${2}
                shift
                ;;
            "-install_ccl")
                INSTALL_CCL=${2}
                shift
                ;;
            "-run_tests")
                RUN_TESTS=${2}
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
    CCL_INSTALL_DIR=${CCL_SRC_DIR}/build/_install

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        DOWNLOAD_CCL="1"
        INSTALL_CCL="1"
        RUN_TESTS="1"

        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR    = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE         = ${FULL_SCOPE}"

    echo_log "DOWNLOAD_CCL       = ${DOWNLOAD_CCL}"
    echo_log "INSTALL_CCL        = ${INSTALL_CCL}"

    echo_log "RUN_TESTS          = ${RUN_TESTS}"
}

download_ccl() {
    if [[ ${DOWNLOAD_CCL} != "1" ]]
    then
        return
    fi

    if [[ -d ${CCL_SRC_DIR} ]]
    then
        rm -rf ${CCL_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}
    git clone --branch ${CCL_BRANCH} --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
    check_exit_code $? "failed to download CCL"
}

install_ccl() {
    if [[ ${INSTALL_CCL} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo_log "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-download_ccl 1\""
        check_exit_code 1 "failed to install CCL"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build

    # intentionally build CCL with MPI support
    # to get MPI launcher in CCL install dir to be used for test launches below
    cmake .. -DBUILD_FT=0 -DBUILD_EXAMPLES=1 -DBUILD_CONFIG=0 -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR} \
        -DENABLE_MPI=1 -DENABLE_MPI_TESTS=1
    check_exit_code $? "failed to configure CCL"

    make -j install
    check_exit_code $? "failed to install CCL"
}

run_tests() {
    if [[ ${RUN_TESTS} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_INSTALL_DIR} ]]
    then
        echo_log "can not find CCL install dir: ${CCL_INSTALL_DIR}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi

    bench_path="${CCL_INSTALL_DIR}/examples/benchmark/benchmark"
    if [[ ! -f ${bench_path} ]]
    then
        echo_log "can not find CCL benchmark: ${bench_path}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi

    vars_file="${CCL_INSTALL_DIR}/env/setvars.sh"
    if [[ ! -f ${vars_file} ]]
    then
        echo_log "can not find CCL vars file: ${vars_file}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi
    source ${vars_file}

    if [[ ! -d ${CCL_ROOT} ]]
    then
        echo_log "can not find CCL root dir: ${CCL_ROOT}"
        echo_log "please source CCL vars script"
        check_exit_code 1 "failed to run tests"
    fi
    echo "CCL_ROOT ${CCL_ROOT}"

    hostfile=${I_MPI_HYDRA_HOST_FILE}

    if [[ ! -f ${hostfile} ]]
    then
        echo_log "can not find hydra hostfile: ${hostfile}"
        echo_log "please set env var I_MPI_HYDRA_HOST_FILE with path to hostile"
        check_exit_code 1 "failed to run tests"
    fi
    echo "I_MPI_HYDRA_HOST_FILE ${hostfile}"

    unique_node_count=( `cat $hostfile | grep -v ^$ | uniq | wc -l` )
    total_node_count=( `cat $hostfile | grep -v ^$ | wc -l` )

    if [ "${unique_node_count}" != "${total_node_count}" ]
    then
        check_exit_code 1 "hostfile should contain unique hostnames"
    fi

    if [ "${total_node_count}" -eq "0" ]
    then
        check_exit_code 1 "hostfile should contain at least one row"
    fi

    for prov in ${PROVS}
    do
        PROV_LOG_FILE="${LOG_FILE}_${prov}"
        touch ${PROV_LOG_FILE}
        for node_count in ${NODE_COUNTS}
        do
            if [ "${node_count}" -gt "${total_node_count}" ]
            then
                echo_log "no enough nodes to run at scale ${node_count}, max available ${total_node_count}"
                continue
            fi

            for worker_count in ${WORKER_COUNTS}
            do
                for coll in ${COLLS}
                do
                    for numa_node in ${NUMA_NODES}
                    do
                        config="prov=${prov} nodes=${node_count} workers=${worker_count} coll=${coll} numa_node=${numa_node}"

                        echo -e "\nexec config: ${config}\n" | tee -a ${PROV_LOG_FILE}
                        
                        exec_env="${BASE_ENV} CCL_WORKER_COUNT=${worker_count} FI_PROVIDER=${prov}"
                        bench_args="-c off -i 40 -w 10 -j off -p 0 -l ${coll} -s ${numa_node} -f 16384 -t 67108864"
                        if [[ ${coll} = "allreduce" ]]
                        then
                            bench_args="-q 1 ${bench_args}"
                        fi
                        
                        cmd="${exec_env} mpiexec -l -n ${node_count} -ppn 1"
                        cmd="${cmd} ${bench_path} ${bench_args} 2>&1 | tee -a ${PROV_LOG_FILE}"

                        echo -e "\n${cmd}\n" | tee -a ${PROV_LOG_FILE}
                        eval ${cmd}
                        check_exit_code $? "${config} failed"
                    done
                done
            done
        done

        fail_pattern="abort|^bad$|corrupt|^fault$|invalid|kill|runtime_error|terminate|timed|unexpected"
        ignore_fail_pattern=""

        fail_strings=`grep -E -i "${fail_pattern}" ${PROV_LOG_FILE}`
        if [[ ! -z ${ignore_fail_pattern} ]]
        then
            fail_strings=`echo "${fail_strings}" | grep -E -i -v "${ignore_fail_pattern}"`
        fi
        fail_count=`echo "${fail_strings}" | sed '/^$/d' | wc -l`

        echo_log ""
        echo_log "prov log: ${PROV_LOG_FILE}"
        echo_log "work dir: ${SCRIPT_WORK_DIR}"
        echo_log ""

        if [[ ${fail_count} -ne 0 ]]
        then
            echo_log ""
            echo_log "fail strings:"
            echo_log ""
            echo_log "${fail_strings}"
            echo_log ""
            check_exit_code ${fail_count} "TEST FAILED"            
        fi
    done

    echo_log "TEST PASSED"
}

parse_arguments $@

download_ccl
install_ccl

run_tests
