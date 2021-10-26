#!/bin/bash

echo_log()
{
    msg=$1
    date=`date`
    echo "[$date] > $msg"
}

check_exit_code()
{
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

print_help()
{
    echo_log "HELP" #WIP
}

set_default_values()
{
    CURRENT_DATE="`date +%Y%m%d.%H%M%S`"
    LOG_DIR="/home/dsazanov/performance/logs"
    SHARED_SYSTEM="10.125.87.66" #hsw03
    PERFORMANCE_WORK_DIR="/home/dsazanov/performance/${CURRENT_DATE}"
    PACKAGE_1=""
    PACKAGE_2=""
    BENCH_ENVIROMENT=""
    BENCH_PARAMETERS=""
    GIT_REPOSITORY="https://github.com/intel-innersource/libraries.performance.communication.oneccl.git"
    GIT_BRANCH="master"
    COMPILER_PATH="/home/sys_ctlab/oneapi/compiler/last"
    MPI_PATH="/home/sys_ctlab/oneapi/mpi_oneapi/last"
    TEST_SCOPE="default"
}

print_values()
{
    echo_log "------------------------------------------------------------"
    echo_log "                         PARAMETERS                         "
    echo_log "------------------------------------------------------------"
    echo_log "LOG_DIR                 = ${LOG_DIR}"
    echo_log "SHARED_SYSTEM           = ${SHARED_SYSTEM}"
    echo_log "PERFORMANCE_WORK_DIR    = ${PERFORMANCE_WORK_DIR}"
    echo_log "PACKAGE_1               = ${PACKAGE_1}"
    echo_log "PACKAGE_2               = ${PACKAGE_2}"
    echo_log "BENCH_ENVIROMENT        = ${BENCH_ENVIROMENT}"
    echo_log "BENCH_PARAMETERS        = ${BENCH_PARAMETERS}"
    echo_log "GIT_REPOSITORY          = ${GIT_REPOSITORY}"
    echo_log "GIT_BRANCH              = ${GIT_BRANCH}"
    echo_log "COMPILER_PATH           = ${COMPILER_PATH}"
    echo_log "MPI_PATH                = ${MPI_PATH}"
    echo_log "TEST_SCOPE              = ${TEST_SCOPE}"
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-l"|"--log_dir")
                LOG_DIR=${2}
                shift
                ;;
            "-s"|"--shared_system")
                SHARED_SYSTEM=${2}
                shift
                ;;
            "-w"|"--work_dir")
                PERFORMANCE_WORK_DIR=${2}
                shift
                ;;
            "-p1"|"--package_1")
                PACKAGE_1=${2}
                shift
                ;;
            "-p2"|"--package_2")
                PACKAGE_2="${2}"
                shift
                ;;
            "-be"|"--bench_enviroment")
                while [[ ${2} != -* ]]
                do
                    BENCH_ENVIROMENT+=" ${2}"
                    shift
                done
                ;;
            "-bp"|"--bench_parameters")
                while [ $# -ne 1 ]
                do
                    BENCH_PARAMETERS+=" ${2}"
                    shift
                done
                ;;
            "-gr"|"--git_repository")
                GIT_REPOSITORY="${2}"
                shift
                ;;
            "-gb"|"--git_branch")
                GIT_BRANCH="${2}"
                shift
                ;;
            "-c"|"--compiler_path")
                COMPILER_PATH="${2}"
                shift
                ;;
            "-m"|"--mpi_path")
                MPI_PATH="${2}"
                shift
                ;;
            "-t"|"--test_scope")
                TEST_SCOPE="${2}"
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
}

copy_packages()
{
    mkdir -p ${PERFORMANCE_WORK_DIR}/{package_1,package_2}
    scp sys_ctlab@${SHARED_SYSTEM}:${PACKAGE_1} ${PERFORMANCE_WORK_DIR}/package_1
    scp sys_ctlab@${SHARED_SYSTEM}:${PACKAGE_2} ${PERFORMANCE_WORK_DIR}/package_2
}

unpack_packages()
{
    for package in package_1 package_2
    do
        tar zxvf ${PERFORMANCE_WORK_DIR}/${package}/*.tgz -C ${PERFORMANCE_WORK_DIR}/${package}
        unzip -P accept ${PERFORMANCE_WORK_DIR}/${package}/`ls ${PERFORMANCE_WORK_DIR}/${package} | grep -v .tgz`/package.zip -d ${PERFORMANCE_WORK_DIR}/${package}
        ${PERFORMANCE_WORK_DIR}/${package}/install.sh -s -d ${PERFORMANCE_WORK_DIR}/${package}/_install
    done
}

set_build_enviroment()
{
    source ${COMPILER_PATH}/compiler/latest/env/vars.sh intel64
    source ${MPI_PATH}/mpi/latest/env/vars.sh
}

build_benchmark()
{
    cd ${PERFORMANCE_WORK_DIR}
    git clone ${GIT_REPOSITORY} -b ${GIT_BRANCH}

    set_build_enviroment
    mkdir -p ${PERFORMANCE_WORK_DIR}/libraries.performance.communication.oneccl/build
    cd ${PERFORMANCE_WORK_DIR}/libraries.performance.communication.oneccl/build
    cmake ..
    make -j install
}

run_cmd()
{
    echo_log "${1}" 2>&1 | tee -a ${log_file}
    eval ${1} 2>&1 | tee -a ${log_file}
    check_exit_code $? "cmd failed"
}

run_benchmark()
{
    set_build_enviroment

    if [ "${TEST_SCOPE}" == "regular" ]
    then
        colls="allreduce alltoall"
        runtimes="level_zero opencl"

        for package in package_1 package_2
        do
            for coll in ${colls}
            do
                for runtime in ${runtimes}
                do
                    source ${PERFORMANCE_WORK_DIR}/${package}/_install/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        
                    cmd=" SYCL_DEVICE_FILTER=${runtime}"
                    cmd+=" mpiexec -n 2"
                    cmd+=" ${PERFORMANCE_WORK_DIR}/libraries.performance.communication.oneccl/build/_install/examples/benchmark/benchmark"
                    cmd+=" -i 100 -t 2097152 -j off -l ${coll}"

                    log_file=${LOG_DIR}/${package}_${CURRENT_DATE}_regular.txt
                    run_cmd "${cmd}"
                done
            done
        done

    elif [ "${TEST_SCOPE}" == "default" ]
    then
        for package in package_1 package_2
        do
            source ${PERFORMANCE_WORK_DIR}/${package}/_install/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        
            cmd=" ${BENCH_ENVIROMENT}"
            cmd+=" mpiexec -n 2"
            cmd+=" ${PERFORMANCE_WORK_DIR}/libraries.performance.communication.oneccl/build/_install/examples/benchmark/benchmark"
            cmd+=" ${BENCH_PARAMETERS}"

            log_file=${LOG_DIR}/${package}_${CURRENT_DATE}_default.txt
            run_cmd "${cmd}"
        done
    fi
}


set_default_values
parse_arguments $@
print_values
copy_packages
unpack_packages
build_benchmark
run_benchmark
