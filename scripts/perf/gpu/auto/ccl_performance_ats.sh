#!/bin/bash

CheckCommandExitCode() 
{
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

echo_log()
{
    msg=$1
    date=`date`
    echo "[$date] > $msg"
}

set_default_values()
{
    LOGIN=$(whoami)
    SYSTEM_LOGIN="gta"
    SYSTEM="10.1.40.43"
    SYSTEM_NAME="ATS"
    LOG_PATH="/p/pdsd/Users/dsazanov/ccl-performance/LOG"
    COMPILER_PATH=""
    MPI_PATH=""
    CCL_PATH=""
    PERF_WORK_DIR="/home/gta/dsazanov/performance"
    CCL_PATH_FIRST=""
    CCL_PATH_SECOND=""
    BENCH_CONF_N="2"
    BENCH_CONF_PPN="1"
    RAW_CONVERTER_PATH="/p/pdsd/scratch/Uploads/oneCCL_performance_tool/Core/"
    CCL_BUILD="0"
    START_BENCHMARK_TYPE="default"
    CHECK_COMPONENTS=true
}

check_def() 
{
    if [[ ${CCL_PATH} == "def" ]]; then
        export CCL_PATH=""
    fi

    if [[ ${MPI_PATH} == "def" ]]; then
        export MPI_PATH=""
    fi

    if [[ ${COMPILER_PATH} == "def" ]]; then
        export COMPILER_PATH=""
    fi

    if [[ ${CCL_PATH_FIRST} == "no" ]] && [[ ${CCL_PATH_SECOND} == "no" ]]; then
        export CCL_PATH_FIRST=""
        export CCL_PATH_SECOND=""
    fi
}

info() {

    echo ""
    echo "   INFO: LOGIN             = ${LOGIN}"
    echo "   INFO: SYSTEM_LOGIN      = ${SYSTEM_LOGIN}"
    echo "   INFO: SYSTEM            = ${SYSTEM}"
    echo "   INFO: LOG_PATH          = ${LOG_PATH}"
    echo "   INFO: COMPILER_PATH     = ${COMPILER_PATH}"
    echo "   INFO: MPI_PATH          = ${MPI_PATH}"
    echo "   INFO: CCL_PATH          = ${CCL_PATH}"
    echo "   INFO: PERF_WORK_DIR     = ${PERF_WORK_DIR}"
    if [ "${CCL_PATH_FIRST}" ] && [ "${CCL_PATH_SECOND}" ]; then
        echo "   INFO: CCL_PATH_FIRST    = ${CCL_PATH_FIRST}"
        echo "   INFO: CCL_PATH_SECOND   = ${CCL_PATH_SECOND}"
    fi
    echo "   INFO: BENCH_CONF_N      = ${BENCH_CONF_N}"
    echo "   INFO: BENCH_CONF_PPN    = ${BENCH_CONF_PPN}"
    echo "   INFO: CCL_BUILD         = ${CCL_BUILD}"
    echo ""

}

create_perf_dir()
(
    ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "cd ${PERF_WORK_DIR};
    mkdir \
    -p ccl \
    -p logs/single/ATS_L0 \
    -p logs/single/ATS_OCL \
    -p oneapi/compiler \
    -p oneapi/mpi_oneapi"
)

copy_hfl_scripts()
(
    echo_log "MESSAGE: Copy ccl_benchmark_run.sh & installing_components.sh"
    scp -q /nfs/inn/proj/mpi/users/dsazanov/ccl-performance/ccl_benchmark_run.sh ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}
    scp -q /nfs/inn/proj/mpi/users/dsazanov/ccl-performance/installing_components.sh ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/oneapi
)

checking_and_copying_components() 
{
    #SET VALUES FOR COMPILER
    if [ -z "$COMPILER_PATH" ]; then
        echo_log "MESSAGE: Using the default compiler path"
        export COMPILER_PATH=`realpath "/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/CCL_oneAPI/compiler/last/"`
        NEW_COMPILER="$(ls ${COMPILER_PATH}/.. -t | grep .sh | head -1)"
        export COMPILER_PATH="${COMPILER_PATH}/../${NEW_COMPILER}"
        echo_log "   INFO: COMPILER_PATH = ${COMPILER_PATH}"
        echo_log "   INFO: NEW_COMPILER  = ${NEW_COMPILER}"
    else
        NEW_COMPILER="$(echo ${COMPILER_PATH} | grep -o '[^/]*$')"
        echo_log "   INFO: COMPILER_PATH = ${COMPILER_PATH}"
        echo_log "   INFO: NEW_COMPILER  = ${NEW_COMPILER}"
    fi

    #SET VALUES FOR MPI
    if [ -z "$MPI_PATH" ]; then
        echo_log "MESSAGE: Using the default MPI path"
        export MPI_PATH=`realpath "/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/CCL_oneAPI/mpi_oneapi/last/"`
        NEW_MPI="$(ls ${MPI_PATH}/.. -t | grep .sh | head -1)"
        export MPI_PATH="${MPI_PATH}/../${NEW_MPI}"
        echo_log "   INFO: MPI_PATH = ${MPI_PATH}"
        echo_log "   INFO: NEW_MPI  = ${NEW_MPI}"
    else
        NEW_MPI="$(echo ${MPI_PATH} | grep -o '[^/]*$')"
        echo_log "   INFO: MPI_PATH = ${MPI_PATH}"
        echo_log "   INFO: NEW_MPI  = ${NEW_MPI}"
    fi

    #GET INSALLED COMPONENTS
    ACTUAL_COMPILER="$(ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "ls ${PERF_WORK_DIR}/oneapi/compiler/ -t | tail -1")"
    echo_log "   INFO: ACTUAL_COMPILER = ${ACTUAL_COMPILER}"
    ACTUAL_MPI="$(ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "ls ${PERF_WORK_DIR}/oneapi/mpi_oneapi/ -t | tail -1")"
    echo_log "   INFO: ACTUAL_MPI = ${ACTUAL_MPI}"

    #COPY NEW COMPILER
    if [[ ${ACTUAL_COMPILER} == ${NEW_COMPILER} ]]; then
        echo_log "MESSAGE: This compiler is already installed"
    else
        echo_log "MESSAGE: Installing the latest compiler: ${ACTUAL_COMPILER} ---> ${NEW_COMPILER}"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "rm -rf ${PERF_WORK_DIR}/oneapi/compiler/*"
        scp -q ${COMPILER_PATH} ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/oneapi/compiler/
        CheckCommandExitCode $? "  ERROR: Copying the latest compiler: ERROR"
        echo_log "MESSAGE: Copying the latest compiler: SUCCESS"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "${PERF_WORK_DIR}/oneapi/installing_components.sh compiler -WDIR ${PERF_WORK_DIR}"
    fi

    #COPY NEW MPI
    if [[ ${ACTUAL_MPI} == ${NEW_MPI} ]]; then
        echo_log "MESSAGE: This MPI is already installed"
    else
        echo_log "MESSAGE: Installing the latest MPI: ${ACTUAL_MPI} ---> ${NEW_MPI}"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "rm -rf ${PERF_WORK_DIR}/oneapi/mpi_oneapi/*"
        scp -qr ${MPI_PATH} ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/oneapi/mpi_oneapi/
        CheckCommandExitCode $? "  ERROR: Copying the latest MPI: ERROR"
        echo_log "MESSAGE: Copying the latest MPI: SUCCESS"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "${PERF_WORK_DIR}/oneapi/installing_components.sh mpi_oneapi -WDIR ${PERF_WORK_DIR}"
    fi
}

copy_ccl() 
{
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]; then
        if [ -z ${CCL_PATH} ]; then
            echo_log "MESSAGE: Using the default ccl path"
            export CCL_PATH="/p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/"
            CCL_PACKAGE_NAME="ccl_src.tgz"
            export CCL_PATH=${CCL_PATH}/${CCL_PACKAGE_NAME}
            echo_log "   INFO: CCL_PACKAGE_NAME = ${CCL_PACKAGE_NAME}"
            echo_log "   INFO: CCL_PATH         = ${CCL_PATH}"
        else
            CCL_PACKAGE_NAME=$(echo ${CCL_PATH} | grep -o '[^/]*$')
            echo_log "   INFO: CCL_PACKAGE_NAME = ${CCL_PACKAGE_NAME}"  
            echo_log "   INFO: CCL_PATH         = ${CCL_PATH}"
        fi

        echo_log "MESSAGE: Removing old ccl"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "rm -rf ${PERF_WORK_DIR}/ccl/*"
        echo_log "MESSAGE: Copying ${CCL_PACKAGE_NAME}"
        scp -qr ${CCL_PATH} ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/ccl/
        CheckCommandExitCode $?  "  ERROR: copy ccl: ERROR"
        echo_log "MESSAGE: Copying ${CCL_PACKAGE_NAME}: SUCCESS"
    else
        echo_log "MESSAGE: Compare ccl packages"
        echo_log "   INFO: CCL_PATH_FIRST         = ${CCL_PATH_FIRST}"
        echo_log "   INFO: CCL_PATH_SECOND        = ${CCL_PATH_SECOND}"
        echo_log "MESSAGE: Removing old ccl_c1 & ccl_c2"
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "rm -rf ${PERF_WORK_DIR}/ccl_c1/* | rm -rf ${PERF_WORK_DIR}/ccl_c2/*"

        echo_log "MESSAGE: Copying ${CCL_PATH_FIRST}"
        scp -qr ${CCL_PATH_FIRST} ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/ccl_c1/
        CheckCommandExitCode $?  "  ERROR: copy ccl_c1: ERROR"
        echo_log "MESSAGE: Copying ${CCL_PATH_FIRST}: SUCCESS"

        echo_log "MESSAGE: Copying ${CCL_PATH_SECOND}"
        scp -qr ${CCL_PATH_SECOND} ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/ccl_c2/
        CheckCommandExitCode $?  "  ERROR: copy ccl_c2: ERROR"
        echo_log "MESSAGE: Copying ${CCL_PATH_SECOND}: SUCCESS"
    fi
}

start_bench() 
{
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]; then
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "${PERF_WORK_DIR}/ccl_benchmark_run.sh -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} ${SYSTEM_NAME} -UIC ${CCL_BUILD} -sl ${SYSTEM_LOGIN} -bt ${START_BENCHMARK_TYPE}"
    else
        ssh -q ${SYSTEM_LOGIN}@${SYSTEM} "${PERF_WORK_DIR}/ccl_benchmark_run.sh -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} ${SYSTEM_NAME} -c -UIC ${CCL_BUILD} -sl ${SYSTEM_LOGIN} -bt ${START_BENCHMARK_TYPE}"
    fi
    echo_log "MESSAGE: Benchmark: SUCCESS"
}

get_logs() 
{
    echo_log "MESSAGE: Start copying logs"
    LOG_FOLDER_NAME="$(date +%Y%m%d-%H%M)_${SYSTEM_NAME}_${SYSTEM_LOGIN}_${START_BENCHMARK_TYPE}"
    echo_log "   INFO: LOG_FOLDER_NAME        = ${LOG_FOLDER_NAME}"
    mkdir -p ${LOG_PATH}/${LOG_FOLDER_NAME}
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]; then
        scp -r ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/logs/single/* ${LOG_PATH}/${LOG_FOLDER_NAME}
        rm ${LOG_PATH}/last_single
        ln -s ${LOG_PATH}/${LOG_FOLDER_NAME} ${LOG_PATH}/last_single
    else
        scp -r ${SYSTEM_LOGIN}@${SYSTEM}:${PERF_WORK_DIR}/logs/comparsion/* ${LOG_PATH}/${LOG_FOLDER_NAME}
        ln -s ${LOG_PATH}/${LOG_FOLDER_NAME} ${LOG_PATH}/last_comparsion
    fi
}

raw_convert() 
{
    echo_log "MESSAGE: Converting logs to raw."
    echo_log "   INFO: LOG_PATH:             = ${LOG_PATH}"
    echo_log "   INFO: RAW_CONVERTER_PATH:   = ${RAW_CONVERTER_PATH}"
    python3 ${RAW_CONVERTER_PATH}/convert_to_raw.py -f ${LOG_PATH}/${LOG_FOLDER_NAME}
    CheckCommandExitCode $?  "  Converting logs to raw: ERROR"
}

parse_parameters() 
{
    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            -sl|--slogin)
            export SYSTEM_LOGIN="$2"; shift; shift;
            ;;
            -s|--system)
            export SYSTEM="$2"; shift; shift;
            ;;
            -w|--work_dir)
            export PERF_WORK_DIR="$2"; shift; shift;
            ;;                
            -c|--comparison)
            export CCL_PATH_FIRST="$2"; export CCL_PATH_SECOND="$3"; shift; shift; shift;
            ;;
            -n)
            export BENCH_CONF_N="$2"; shift; shift;
            ;;
            -ppn)
            export BENCH_CONF_PPN="$2"; shift; shift;
            ;;
            --ccl_path)
            export CCL_PATH="$2"; shift; shift;
            ;;
            --log_path)
            export LOG_PATH="$2"; shift; shift;
            ;;
            --mpi_path)
            export MPI_PATH="$2"; shift; shift;
            ;;
            --compiler_path)
            export COMPILER_PATH="$2"; shift; shift;
            ;;
            --raw_conv_path)
            export RAW_CONVERTER_PATH="$2"; shift; shift;
            ;;
            --ccl_build)
            export CCL_BUILD="$2"; shift; shift;
            ;;
            -sb)
            export START_BENCHMARK_TYPE="$2"; shift; shift;
            ;;
            -CHECK_COMPONENTS)
            export CHECK_COMPONENTS="$2"; shift; shift;
            ;;
            -*|--*)
            echo "  ERROR: Unsupported flag $key"; exit 1
            ;;
        esac
    done
}

#==============================================================================
#                                     MAIN
#==============================================================================

set_default_values
parse_parameters $@
check_def
info
create_perf_dir
copy_hfl_scripts

if $CHECK_COMPONENTS; then
    checking_and_copying_components
fi

if [[ "${CCL_BUILD}" == "1" ]]; then
    copy_ccl
fi

start_bench
get_logs
raw_convert
