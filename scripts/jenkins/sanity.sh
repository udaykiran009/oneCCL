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

set_default_values()

if [ -z "${BUILD_COMPILER_TYPE}" ]
then
    BUILD_COMPILER_TYPE="clang"
fi

if [ "${BUILD_COMPILER_TYPE}" == "gnu" ]
 then
    BUILD_COMPILER=/usr/bin
    C_COMPILER=${BUILD_COMPILER}/gcc
    CXX_COMPILER=${BUILD_COMPILER}/g++
else
    if [ -z "${SYCL_BUNDLE_ROOT}" ]
    then
        echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: /p/pdsd/scartch/Software/dpc/_install/dpcpp_bundle_1.0_prealpha_u9_023"
        source /p/pdsd/scratch/Software/dpc/_install/dpcpp_bundle_1.0_prealpha_u9_023/dpc++vars.sh -r sycl
        #source /opt/intel/dpcpp_bundle_1.0_prealpha_u9_023/dpcvars.sh -r sycl
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

#export I_MPI_JOB_TIMEOUT=400

if [ -n "${TESTING_ENVIRONMENT}" ]
then
    echo "TESTING_ENVIRONMENT is " ${TESTING_ENVIRONMENT}
    for line in ${TESTING_ENVIRONMENT}
    do
        export "$line"
    done
fi

pwd
rm -rf ${WORK_DIR}/_log
mkdir -p ${WORK_DIR}/_log

if [ -z "$runtime" ]
then
    runtime="ofi"
fi


# if [ -z "${CCL_ROOT}" ]
# then
if [ -f ${MLSL_PATH}/vars.sh ]
then
    source ${MLSL_PATH}/vars.sh
else
    echo "ERROR: ${CCL_INSTALL_DIR}/intel64/bin/cclvars.sh doesn't exist"
    exit 1
fi

if [ -z "${IMPI_PATH}" ]
then
    echo "WARNING: I_MPI_ROOT isn't set, last oneAPI pack will be used."
    source /p/pdsd/scratch/jenkins/artefacts/impi-ch4-build-linux-with-scans/4/oneapi_impi/env/vars.sh release_mt
else
    source ${IMPI_PATH}/env/vars.sh release_mt
fi

cd ${WORK_DIR}/../../testspace/$runtime/tests/functional
mkdir -p build
cd ./build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="${C_COMPILER}" \
    -DCMAKE_CXX_COMPILER="${CXX_COMPILER}"
make all
case "$runtime" in
       mpi )
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
            export CCL_ATL_TRANSPORT=MPI
            ctest -VV -C Mpi
            ;;
       mpi_adjust )
            export CCL_TEST_BUFFER_COUNT=0
            export CCL_TEST_CACHE_TYPE=1
            export CCL_TEST_COMPLETION_TYPE=1
            export CCL_TEST_DATA_TYPE=0
            export CCL_TEST_PLACE_TYPE=1
            export CCL_TEST_PRIORITY_TYPE=0
            export CCL_TEST_REDUCTION_TYPE=1
            export CCL_TEST_SIZE_TYPE=0
            export CCL_TEST_SYNC_TYPE=0
            export CCL_TEST_PROLOG_TYPE=1
            export CCL_TEST_PLACE_TYPE=1                
            export CCL_ATL_TRANSPORT=MPI
            for bcast in "ring" "double_tree" "direct"
                do
                    CCL_BCAST=$bcast ctest -VV -C mpi_bcast_$bcast
                done
            for reduce in "tree" "double_tree" "direct"
                do
                    CCL_REDUCE=$reduce ctest -VV -C mpi_reduce_$reduce
                done
            for allreduce in "tree" "starlike" "ring" "double_tree" "direct"
                do
                    CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                done
            for allgatherv in "direct" "naive" "multi_bcast" "flat"
                do
                    CCL_ALLGATHERV=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
                done
           ;;
       ofi_adjust )
            export CCL_TEST_BUFFER_COUNT=0
            export CCL_TEST_CACHE_TYPE=1
            export CCL_TEST_COMPLETION_TYPE=1
            export CCL_TEST_DATA_TYPE=0
            export CCL_TEST_PLACE_TYPE=1
            export CCL_TEST_PRIORITY_TYPE=0
            export CCL_TEST_REDUCTION_TYPE=1
            export CCL_TEST_SIZE_TYPE=0
            export CCL_TEST_SYNC_TYPE=0
            export CCL_TEST_PROLOG_TYPE=1
            export CCL_TEST_PLACE_TYPE=1
            for bcast in "ring" "double_tree"
                do
                    CCL_BCAST=$bcast ctest -VV -C mpi_bcast_$bcast
                done
            for reduce in "tree" "double_tree"
                do
                    CCL_REDUCE=$reduce ctest -VV -C mpi_reduce_$reduce
                done
            for allreduce in "tree" "starlike" "ring" "ring_rma" "double_tree"
                do
                    if [ "$allreduce" == "ring_rma" ];
                    then
                        CCL_RMA=1 CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                    else
                        CCL_ALLREDUCE=$allreduce ctest -VV -C mpi_allreduce_$allreduce
                    fi
                done
            for allgatherv in "naive" "multi_bcast" "flat"
                do
                    CCL_ALLGATHERV=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
                done
           ;;
        priority_mode )
            export CCL_TEST_BUFFER_COUNT=1
            export CCL_TEST_CACHE_TYPE=1
            export CCL_TEST_COMPLETION_TYPE=1
            export CCL_TEST_DATA_TYPE=0
            export CCL_TEST_PLACE_TYPE=1
            export CCL_TEST_PRIORITY_TYPE=1
            export CCL_TEST_REDUCTION_TYPE=0
            export CCL_TEST_SIZE_TYPE=0
            export CCL_TEST_SYNC_TYPE=0
            export CCL_TEST_PROLOG_TYPE=0
            export CCL_TEST_PLACE_TYPE=0
            CCL_PRIORITY=lifo ctest -VV -C Default
            CCL_PRIORITY=direct ctest -VV -C Default
           ;;
        dynamic_pointer_mode )
            export CCL_TEST_BUFFER_COUNT=1
            export CCL_TEST_CACHE_TYPE=1
            export CCL_TEST_COMPLETION_TYPE=1
            export CCL_TEST_DATA_TYPE=0
            export CCL_TEST_PLACE_TYPE=1
            export CCL_TEST_PRIORITY_TYPE=0
            export CCL_TEST_REDUCTION_TYPE=0
            export CCL_TEST_SIZE_TYPE=0
            export CCL_TEST_SYNC_TYPE=0
            export CCL_TEST_PROLOG_TYPE=0
            export CCL_TEST_PLACE_TYPE=0
            CCL_TEST_DYNAMIC_POINTER=1 ctest -VV -C Default
           ;;
        out_of_order_mode )
            export CCL_TEST_BUFFER_COUNT=1
            export CCL_TEST_CACHE_TYPE=1
            export CCL_TEST_COMPLETION_TYPE=1
            export CCL_TEST_DATA_TYPE=1
            export CCL_TEST_PLACE_TYPE=1
            export CCL_TEST_PRIORITY_TYPE=0
            export CCL_TEST_REDUCTION_TYPE=0
            export CCL_TEST_SIZE_TYPE=0
            export CCL_TEST_SYNC_TYPE=0
            export CCL_TEST_PROLOG_TYPE=0
            export CCL_TEST_PLACE_TYPE=0
            CCL_UNORDERED_COLL=1 ctest -VV -C Default
           ;;
       * )
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
            ctest -VV -C Default
            ;;
esac
