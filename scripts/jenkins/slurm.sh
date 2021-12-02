#!/bin/bash -x

BASENAME=$(basename $0 .sh)
SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
cp ${SCRIPT_DIR}/sanity.sh ${SCRIPT_DIR}/slurm_sanity.sh

function set_default_env()
{
    export CURRENT_WORK_DIR=$(realpath ${SCRIPT_DIR}/../../)
    export PVC_WORKSPACE_DIR="/home/sys_ctlab/workspace/workspace/"
    export PVC_ARTEFACT_DIR="${PVC_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
    export NUM_OF_HOSTS="1"

    source ${CURRENT_WORK_DIR}/scripts/utils/common.sh
}

function parse_arguments()
{
    if [ $# -eq 0 ]; then
        print_help
        exit 1
    fi

    while [ $# -ne 0 ]
    do
        case $1 in
        "-regular_tests")
            ENABLE_REGULAR_TESTS="yes"
            JOB_NAME="oneCCL_CI_regular_tests_${build_type}_${worker_count}"
            ;;
        "-horovod_tests")
            ENABLE_HOROVOD_TESTS="yes"
            JOB_NAME="oneCCL_CI_horovod_tests"
            ;;
        "-functional_tests")
            ENABLE_FUNCTIONAL_TESTS="yes"
            JOB_NAME="oneCCL_CI_func_tests_${build_type}_${worker_count}_${runtime}"
            ;;
        "-reg_tests")
            ENABLE_REG_TESTS="yes"
            JOB_NAME="oneCCL_CI_reg_tests_${build_type}"
            ;;
        "-reservation")
            RESERVATION_NAME="${2}"
            shift
            ;;
        "-num_of_hosts")
            NUM_OF_HOSTS="${2}"
            shift
            ;;
        "-help"|"--help"|"-h"|"-H")
            print_help
            exit 0
            ;;
        *)
            echo "$(basename ${0}): ERROR: unknown option (${1})"
            print_help
            exit 1
            ;;
        esac
        shift
    done

    echo "ENABLE_REGULAR_TESTS      = ${ENABLE_REGULAR_TESTS}"
    echo "ENABLE_HOROVOD_TESTS      = ${ENABLE_HOROVOD_TESTS}"
    echo "ENABLE_FUNCTIONAL_TESTS   = ${ENABLE_FUNCTIONAL_TESTS}"
    echo "ENABLE_REG_TESTS          = ${ENABLE_REG_TESTS}"
    echo "JOB_NAME                  = ${JOB_NAME}"
    echo "RESERVATION_NAME          = ${RESERVATION_NAME}"
    echo "NUM_OF_HOSTS              = ${NUM_OF_HOSTS}"
}

function print_help()
{
    echo -e "Usage:\n" \
    "    ./${BASENAME}.sh <options> \n" \
    "<options>:\n" \
    "-regular_tests:          run examples\n" \
    "-horovod_tests:          run horovod\n" \
    "-functional_tests:       run functional tests\n" \
    "-reg_tests:              run regression tests\n" \
    "-reservation <string>:   set reservation name\n" \
    "-num_of_hosts <number>:  set the number of hosts\n" \
    "-help:                   print this help information"
    exit 0
}

function check_testing_status()
{
    failed_pattern="failed|testing ... NOK"
    exclude_pattern="^\+|^--|0 tests failed"
    grep -i -w -E "${failed_pattern}" ${1} | grep -Ev "${exclude_pattern}"
    if [[ $? = 0 ]]
    then
        exit 1
    fi
}

function set_reservation_env()
{
    if [[ -z ${RESERVATION_NAME} ]]
    then
        RESERVATION_NAME="OneCCL"
    fi
    NAME=$(sinfo -T | grep -e "${RESERVATION_NAME}.* ACTIVE" | awk '{print $1}')
    echo "DEBUG: RESERVATION NAME = ${NAME}"
    if [[ -z ${NAME} ]]
    then
        echo "ERROR: There is no active reservation"
        exit 1
    fi
    export SALLOC_RESERVATION=${NAME}
    export SBATCH_RESERVATION=${NAME}
    export SLURM_RESERVATION=${NAME}
}

function set_sbatch_header()
{
    sed -i "2i#SBATCH -J ${JOB_NAME}" ${SCRIPT_DIR}/slurm_sanity.sh
    sed -i "2i#SBATCH -N ${NUM_OF_HOSTS}" ${SCRIPT_DIR}/slurm_sanity.sh
    sed -i "2i#SBATCH -t 60" ${SCRIPT_DIR}/slurm_sanity.sh
    sed -i "2i#SBATCH -o ${SCRIPT_DIR}/slurm_sanity.log" ${SCRIPT_DIR}/slurm_sanity.sh
    sed -i "2i#SBATCH -e ${SCRIPT_DIR}/slurm_sanity.log" ${SCRIPT_DIR}/slurm_sanity.sh
}

function install_oneccl()
{
    install_oneccl_package ${PVC_ARTEFACT_DIR}
}

function run_tests() 
{
    set_reservation_env
    set_sbatch_header

    if [[ ${ENABLE_REGULAR_TESTS} = "yes" ]]
    then
        sbatch ${SCRIPT_DIR}/slurm_sanity.sh -regular_tests -mpich_ofi
    fi
    if [[ ${ENABLE_HOROVOD_TESTS} = "yes" ]]
    then
        sbatch ${SCRIPT_DIR}/slurm_sanity.sh -horovod_tests -mpich_ofi
    fi
    if [[ ${ENABLE_FUNCTIONAL_TESTS} = "yes" ]]
    then
        sbatch ${SCRIPT_DIR}/slurm_sanity.sh -functional_tests -mpich_ofi
    fi
    if [[ ${ENABLE_REG_TESTS} = "yes" ]]
    then
        sbatch ${SCRIPT_DIR}/slurm_sanity.sh -reg_tests -mpich_ofi
    fi

    JOB_ID=$(squeue --name=${JOB_NAME} --user=$(whoami) -h | awk '{print $1}')
    while true
    do
        squeue | grep ${JOB_ID}
        if [[ $? != 0 ]]
        then
            echo "DEBUG: Job ${JOB_ID} was complited"
            break
        else
            echo "DEBUG: Job ${JOB_ID} is still running..."
        fi
        sleep 60
    done

    cat ${SCRIPT_DIR}/slurm_sanity.log
    check_testing_status ${SCRIPT_DIR}/slurm_sanity.log
}

set_default_env
parse_arguments "$@"
install_oneccl
run_tests
