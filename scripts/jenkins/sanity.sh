#!/bin/bash -x

echo "DEBUG: GIT_BRANCH = ${GIT_BRANCH}"
echo "DEBUG: CCL_BUILD_ID = ${CCL_BUILD_ID}"

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

CURRENT_CCL_BUILD_DIR="${CCL_REPO_DIR}/${CCL_BUILD_ID}"
CCL_INSTALL_DIR="${SCRIPT_DIR}/../../build/_install"
BASENAME=`basename $0 .sh`
WORK_DIR=`cd ${SCRIPT_DIR}/../../ && pwd -P`
HOSTNAME=`hostname -s`

echo "SCRIPT_DIR = $SCRIPT_DIR"
echo "WORK_DIR = $WORK_DIR"
echo "CCL_INSTALL_DIR = $CCL_INSTALL_DIR"


#==============================================================================
#                                Defaults
#==============================================================================
set_default_values()
{
    ENABLE_DEBUG="no"
    ENABLE_VERBOSE="yes"
    ENABLE_BUILD_TESTS="yes"
    ENABLE_RUN_TESTS="yes"
}
#==============================================================================
#                                Functions
#==============================================================================

# Logging. Always log. Echo if ENABLE_VERBOSE is "yes".
log()
{
    if [ "${ENABLE_VERBOSE}" = "yes" ]; then
        eval $* 2>&1 | tee -a ${LOG_FILE}
    else
        eval $* >> ${LOG_FILE} 2>&1
    fi
}

# Print usage and help information
print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh <options>"
    echo_log ""
    echo_log "<options>:"
    echo_log "   ------------------------------------------------------------"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -h|-H|-help|--help"
    echo_log "        Print help information"
    echo_log ""
}

# Echo with logging. Always echo and always log
echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}
# echo the debug information
echo_debug()
{
    if [ ${ENABLE_DEBUG} = "yes" ]; then
        echo_log "DEBUG: $*"
    fi
}

echo_log_separator()
{
    echo_log "#=============================================================================="
}

check_command_exit_code() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

set_external_env(){
    if [ -n "${TESTING_ENVIRONMENT}" ]
    then
        echo "TESTING_ENVIRONMENT is " ${TESTING_ENVIRONMENT}
        for line in ${TESTING_ENVIRONMENT}
        do
            export "$line"
        done
    fi
}

enable_part_test_scope()
{
    export CCL_TEST_BUFFER_COUNT=0
    export CCL_TEST_CACHE_TYPE=1
    export CCL_TEST_COMPLETION_TYPE=1
    export CCL_TEST_DATA_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
    export CCL_TEST_PRIORITY_TYPE=0
    export CCL_TEST_REDUCTION_TYPE=0
    export CCL_TEST_SIZE_TYPE=0
    export CCL_TEST_SYNC_TYPE=0
    export CCL_TEST_PROLOG_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
}

enable_unordered_coll_test_scope()
{
    export CCL_TEST_BUFFER_COUNT=0
    export CCL_TEST_CACHE_TYPE=1
    export CCL_TEST_COMPLETION_TYPE=1
    export CCL_TEST_DATA_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
    export CCL_TEST_PRIORITY_TYPE=0
    export CCL_TEST_REDUCTION_TYPE=1
    export CCL_TEST_SIZE_TYPE=0
    export CCL_TEST_SYNC_TYPE=0
    export CCL_TEST_PROLOG_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
}

set_environment()
{
    if [ -z "${BUILD_COMPILER_TYPE}" ]
    then
        BUILD_COMPILER_TYPE="clang"
    fi

    if [ "${BUILD_COMPILER_TYPE}" == "gnu" ]
     then
        BUILD_COMPILER=/usr/bin
        C_COMPILER=${BUILD_COMPILER}/gcc
        CXX_COMPILER=${BUILD_COMPILER}/g++
    elif [ "${BUILD_COMPILER_TYPE}" = "intel" ]
    then
        BUILD_COMPILER=/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2019.4.243/linux/bin/intel64/
        C_COMPILER=${BUILD_COMPILER}/icc
        CXX_COMPILER=${BUILD_COMPILER}/icpc
    else        
        if [ -z "${SYCL_BUNDLE_ROOT}" ]
        then
            echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: /p/pdsd/scartch/Software/dpc/_install/dpcpp_bundle_1.0_prealpha_u9_023"
            source /p/pdsd/scratch/Software/dpc/_install/dpcpp_bundle_1.0_prealpha_u9_023/dpc++vars.sh sycl
        fi
        BUILD_COMPILER=${SYCL_BUNDLE_ROOT}/bin
        C_COMPILER=${BUILD_COMPILER}/clang
        CXX_COMPILER=${BUILD_COMPILER}/clang++
    fi
    if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
    then
        if [ -f ${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts ]
        then
            export I_MPI_HYDRA_HOST_FILE=${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
        else
            echo "WARNING: hostfile (${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts) isn't available"
            echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set"
        fi
    fi

    if [ -z "$runtime" ]
    then
        runtime="ofi"
    fi

    if [ -z "${CCL_ROOT}" ]
    then
        if [ -f ${MLSL_PATH}/vars.sh ]
        then
            source ${MLSL_PATH}/vars.sh
        else
            echo "ERROR: ${CCL_INSTALL_DIR}/intel64/bin/cclvars.sh doesn't exist"
            exit 1
        fi
    fi
    if [ -z "${IMPI_PATH}" ]
    then
        echo "WARNING: I_MPI_ROOT isn't set, last oneAPI pack will be used."
        source /nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/IMPI/linux/functional_testing/impi/ww31_20190802_oneapi/env/vars.sh release_mt
    else
        source ${IMPI_PATH}/env/vars.sh release_mt
    fi

    if [ "${ENABLE_CODECOV}" = "yes" ]
    then
        CODECOV_FLAGS="TRUE"
    else
        CODECOV_FLAGS=""
    fi
}

make_tests()
{
    cd ${WORK_DIR}/../../testspace/$runtime/tests/functional
    mkdir -p build
    cd ./build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="${C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${CXX_COMPILER}"
    make all
}

run_examples()
{
    for transport in "ofi" "mpi";
    do
        examples_to_run=`ls ${WORK_DIR}/../build/_install/examples/`
        if [ "$transport" == "mpi" ];
        then
            examples_to_run=`ls ${WORK_DIR}/../build/_install/examples/ | grep -v '.log' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce' | grep -v 'sparse_allreduce' `
        else
            examples_to_run=`ls ${WORK_DIR}/../build/_install/examples/ | grep -v '.log' | grep -v 'sparse_allreduce' `
        fi
        echo "run examples with $transport transport (${examples_to_run})"
        for example in $examples_to_run
        do
            CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -n 2 -ppn 1 -l ${WORK_DIR}/../build/_install/examples/$example | tee ${WORK_DIR}/../build/_install/examples/$example.log
            grep -r 'FAILED' ${WORK_DIR}/../build/_install/examples/$example.log > /dev/null 2>&1
            log_status_fail=${PIPESTATUS[0]}
            if [ "$log_status_fail" -ne 0 ]
            then
                echo "example testing ... OK"
            else
                echo "example testing ... NOK"
                exit 1
            fi
        done
    done
    exit 0
}

run_tests()
{
    enable_part_test_scope
    set_external_env
    case "$runtime" in
           ofi )
               export CCL_ATL_TRANSPORT=ofi
               ctest -VV -C Default
               ctest -VV -C Custom
               ;;
           mpi )
                export CCL_ATL_TRANSPORT=mpi
                ctest -VV -C Default
                ;;
           mpi_adjust )
                export CCL_ATL_TRANSPORT=mpi
                for allgatherv in "direct" "naive" "flat" "multi_bcast"
                    do
                        CCL_ALLGATHERV=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
                    done
                for allreduce in "direct" "tree" "starlike" "ring" "double_tree" "recursive_doubling"
                    do
                        CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                    done
                for bcast in "direct" "ring" "double_tree" "naive"
                    do
                        CCL_BCAST=$bcast ctest -VV -C mpi_bcast_$bcast
                    done
                for reduce in "direct" "tree" "double_tree"
                    do
                        CCL_REDUCE=$reduce ctest -VV -C mpi_reduce_$reduce
                    done
               ;;
           ofi_adjust )
                export CCL_ATL_TRANSPORT=ofi
                for allgatherv in "naive" "flat" "multi_bcast"
                    do
                        CCL_ALLGATHERV=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
                    done
                for allreduce in "tree" "starlike" "ring" "ring_rma" "double_tree" "recursive_doubling"
                    do
                        if [ "$allreduce" == "ring_rma" ];
                        then
                            CCL_RMA=1 CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                        else
                            CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                        fi
                done
                for bcast in "ring" "double_tree" "naive"
                    do
                        CCL_BCAST=$bcast ctest -VV -C mpi_bcast_$bcast
                    done
                for reduce in "tree" "double_tree"
                    do
                        CCL_REDUCE=$reduce ctest -VV -C mpi_reduce_$reduce
                    done
               ;;
            priority_mode )
                CCL_ATL_TRANSPORT=mpi CCL_PRIORITY=lifo ctest -VV -C Default
                CCL_ATL_TRANSPORT=mpi CCL_PRIORITY=direct ctest -VV -C Default
                CCL_ATL_TRANSPORT=ofi CCL_PRIORITY=lifo ctest -VV -C Default
                CCL_ATL_TRANSPORT=ofi CCL_PRIORITY=direct ctest -VV -C Default
               ;;
            dynamic_pointer_mode )
                CCL_ATL_TRANSPORT=mpi CCL_TEST_DYNAMIC_POINTER=1 ctest -VV -C Default
                CCL_ATL_TRANSPORT=ofi CCL_TEST_DYNAMIC_POINTER=1 ctest -VV -C Default
               ;;
            unordered_coll_mode )
                enable_unordered_coll_test_scope
                CCL_ATL_TRANSPORT=ofi CCL_UNORDERED_COLL=1 ctest -VV -C Default
               ;;
           * )
                ctest -VV -C Default
                ;;
    esac
}

set_default_values
set_environment

export CCL_WORKER_COUNT=2

while [ $# -ne 0 ]
do
    case $1 in
    "-example" )
        run_examples
        shift
        ;;
    *)
        echo "WARNING: example testing not started"
        exit 0
        shift
        ;;
    esac
done
make_tests
run_tests
