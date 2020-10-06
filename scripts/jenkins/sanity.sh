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
BASENAME=`basename $0 .sh`
CURRENT_WORK_DIR=`cd ${SCRIPT_DIR}/../../ && pwd -P`
if [ -z $CCL_INSTALL_DIR ]
then
    CCL_INSTALL_DIR=`cd ${SCRIPT_DIR}/../../build/_install/ && pwd -P`
fi
HOSTNAME=`hostname -s`

echo "SCRIPT_DIR = $SCRIPT_DIR"
echo "CURRENT_WORK_DIR = $CURRENT_WORK_DIR"

export CCL_WORKER_AFFINITY=auto

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
    if [ ${TEST_CONFIGURATIONS} == "test:nightly" ]
    then
        echo "Nightly test scope will be started."
        export TESTING_ENVIRONMENT="CCL_TEST_BUFFER_COUNT=0 \
        CCL_TEST_CACHE_TYPE=1 \
        CCL_TEST_COMPLETION_TYPE=1 \
        CCL_TEST_DATA_TYPE=1 \
        CCL_TEST_PLACE_TYPE=1 \
        CCL_TEST_PRIORITY_TYPE=0 \
        CCL_TEST_REDUCTION_TYPE=1 \
        CCL_TEST_SIZE_TYPE=0 \
        CCL_TEST_SYNC_TYPE=1 \
        CCL_TEST_EPILOG_TYPE=1 \
        CCL_TEST_PROLOG_TYPE=1" 
    fi
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
    echo "Partly test scope will be started."
    export CCL_TEST_BUFFER_COUNT=0
    export CCL_TEST_CACHE_TYPE=0
    export CCL_TEST_COMPLETION_TYPE=0
    export CCL_TEST_DATA_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
    export CCL_TEST_PRIORITY_TYPE=1
    export CCL_TEST_REDUCTION_TYPE=0
    export CCL_TEST_SIZE_TYPE=1
    export CCL_TEST_SYNC_TYPE=1
    export CCL_TEST_EPILOG_TYPE=1
    export CCL_TEST_PROLOG_TYPE=1
}

enable_unordered_coll_test_scope()
{
    echo "Unordered coll test scope will be started."
    export CCL_TEST_BUFFER_COUNT=0
    export CCL_TEST_CACHE_TYPE=1
    export CCL_TEST_COMPLETION_TYPE=1
    export CCL_TEST_DATA_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
    export CCL_TEST_PRIORITY_TYPE=1
    export CCL_TEST_REDUCTION_TYPE=1
    export CCL_TEST_SIZE_TYPE=0
    export CCL_TEST_SYNC_TYPE=0
    export CCL_TEST_PROLOG_TYPE=0
    export CCL_TEST_EPILOG_TYPE=0
}

set_environment()
{
    if [ -z "$build_type" ]
    then
        build_type="release"
    fi

    if [ -z  "${node_label}" ]
    then
        build_compiler="gnu"
        source ${CCL_INSTALL_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_icc
    elif [ $node_label == "mlsl2_test_gpu" ]
    then
        build_compiler="sycl"
        source ${CCL_INSTALL_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
    else
        build_compiler="gnu"
        source ${CCL_INSTALL_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_icc
    fi

    if [ -z "${build_compiler}" ]
    then
        build_compiler="sycl"
    fi
    if [ $build_compiler == "gnu" ]
    then
        BUILD_COMPILER_TYPE="gnu"
    elif [ $build_compiler == "sycl" ] || [ $build_compiler == "nosycl" ]
    then
        BUILD_COMPILER_TYPE="clang"
    elif [ $build_compiler == "intel" ]
    then
        BUILD_COMPILER_TYPE="intel"
    fi
    # $BUILD_COMPILER_TYPE may be set up by user: clang/gnu/intel
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
        source /nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/bin/compilervars.sh intel64
        BUILD_COMPILER=/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/bin/intel64/
        C_COMPILER=${BUILD_COMPILER}/icc
        CXX_COMPILER=${BUILD_COMPILER}/icpc
    else
        if [ -z "${SYCL_BUNDLE_ROOT}" ]
        then
            SYCL_BUNDLE_ROOT="/p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/inteloneapi/compiler/latest/linux/"
            echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: $SYCL_BUNDLE_ROOT"
        fi
        source ${SYCL_BUNDLE_ROOT}/../../../setvars.sh
        BUILD_COMPILER=${SYCL_BUNDLE_ROOT}/compiler/latest/linux/bin
        C_COMPILER=${BUILD_COMPILER}/clang
        CXX_COMPILER=${BUILD_COMPILER}/clang++
    fi
    
    if [ -z "$worker_count" ]
    then
        worker_count="2"
    fi
    export CCL_WORKER_COUNT=$worker_count
    if [ -z "$runtime" ]
    then
        runtime="ofi"
    fi

    if [ -z "$build_type" ]
    then
        build_type="release"
    fi
    if [ -z ${CCL_ROOT} ]
    then 
        if [ -z  "${node_label}" ]
        then
            source ${CCL_INSTALL_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu_icc
        elif [ $node_label == "mlsl2_test_gpu" ]
        then
            source ${CCL_INSTALL_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
            export DASHBOARD_GPU_DEVICE_PRESENT="yes"
        else
            source ${CCL_INSTALL_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu_icc
        fi
    fi

    if [ "${ENABLE_CODECOV}" = "yes" ]
    then
        CODECOV_FLAGS="TRUE"
    else
        CODECOV_FLAGS=""
    fi
}

set_impi_environment()
{
    if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
    then
        if [ -f ${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts ]
        then
            export I_MPI_HYDRA_HOST_FILE=${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
        else
            echo "WARNING: hostfile (${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts) isn't available"
            echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set"
        fi
    fi
    if [ -z "${IMPI_PATH}" ]
    then
        echo "WARNING: IMPI_PATH isn't set, last oneAPI pack will be used."
        export IMPI_PATH=/p/pdsd/scratch/Uploads/IMPI/linux/functional_testing/impi/2021.1-beta09/
    fi
    source ${IMPI_PATH}/env/vars.sh -i_mpi_library_kind=release_mt
}

make_tests()
{
    cd ${CURRENT_WORK_DIR}/tests/functional
    mkdir -p build
    cd ./build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="${C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${CXX_COMPILER}"
    make all
}

run_compatibitily_tests()
{
    export EXAMPLE_WORK_DIR="${CCL_ROOT}/examples/build"
    mkdir -p ${EXAMPLE_WORK_DIR}
    echo "EXAMPLE_WORK_DIR =" $EXAMPLE_WORK_DIR
    set_external_env
    cd ${EXAMPLE_WORK_DIR}
    if [ ${node_label} == "mlsl2_test_gpu" ]
    then
        export FI_TCP_IFACE=eno1
        ${CURRENT_WORK_DIR}/examples/run.sh gpu
    else
        ${CURRENT_WORK_DIR}/examples/run.sh cpu
    fi
    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "compatibitily testing ... OK"
        exit 0
    else
        echo "compatibitily testing ... NOK"
        exit 1
    fi
}

set_modulefile_environment()
{
    source /etc/profile.d/modules.sh
    export CCL_CONFIGURATION=cpu_icc
    if [ -z "$build_type" ]
    then
        build_type="release"
    fi

    module load ${CCL_INSTALL_DIR}/l_ccl_${build_type}*/modulefiles/ccl

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "module file load ... OK"
    else
        echo "module file load ... NOK"
        exit 1
    fi
}

unset_modulefile_environment()
{
    module unload ccl

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "module file unload ... OK"
    else
        echo "module file unload ... NOK"
        exit 1
    fi
}

run_modulefile_tests()
{
    set_modulefile_environment

    export EXAMPLE_WORK_DIR="${CCL_ROOT}/examples/build"
    mkdir -p ${EXAMPLE_WORK_DIR}
    echo "EXAMPLE_WORK_DIR =" $EXAMPLE_WORK_DIR

    set_external_env

    cd ${EXAMPLE_WORK_DIR}
    ${CURRENT_WORK_DIR}/examples/run.sh cpu

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "module file testing ... OK"
        exit 0
    else
        echo "module file testing ... NOK"
        exit 1
    fi

    unset_modulefile_environment
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
                
                for algo in "direct" "naive" "flat" "ring"
                do
                    CCL_ALLGATHERV=$algo ctest -VV -C mpi_allgatherv_$algo
                done

                for algo in "direct" "rabenseifner" "starlike" "ring" "double_tree" "recursive_doubling"
                do
                    if [ "$algo" == "ring" ];
                    then
                        CCL_RS_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_"$algo"_chunked
                    elif [ "$algo" == "starlike" ];
                    then
                        CCL_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_"$algo"_chunked
                    fi
                    CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_$algo
                done

                for algo in "direct" "naive" "scatter" "scatter_barrier"
                do
                    if [ "$algo" == "scatter_barrier" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=1 CCL_ALLTOALL_SCATTER_PLAIN=1 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALL=$algo ctest -VV -C mpi_alltoall_"$algo"_chunked
                    fi
                    CCL_ALLTOALL=$algo ctest -VV -C mpi_alltoall_$algo
                done

                for algo in "direct" "naive" "scatter" "scatter_barrier"
                do
                    if [ "$algo" == "scatter_barrier" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=1 CCL_ALLTOALL_SCATTER_PLAIN=1 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALLV=$algo ctest -VV -C mpi_alltoallv_"$algo"_chunked
                    fi
                    CCL_ALLTOALLV=$algo ctest -VV -C mpi_alltoallv_$algo
                done

                for algo in "direct" "ring" "double_tree" "naive"
                do
                    CCL_BCAST=$algo ctest -VV -C mpi_bcast_$algo
                done

                for algo in "direct" "rabenseifner" "tree"
                do
                    CCL_REDUCE=$algo ctest -VV -C mpi_reduce_$algo
                done

                for algo in "direct" "ring"
                do
                    CCL_REDUCE_SCATTER=$algo ctest -VV -C mpi_reduce_scatter_$algo
                done

               ;;
           ofi_adjust )

                export CCL_ATL_TRANSPORT=ofi

                for algo in "naive" "flat" "multi_bcast" "ring"
                do
                    CCL_ALLGATHERV=$algo ctest -VV -C mpi_allgatherv_$algo
                done

                for algo in "rabenseifner" "starlike" "ring" "ring_rma" "double_tree" "recursive_doubling" "2d"
                do
                    if [ "$algo" == "ring_rma" ];
                    then
                        CCL_RMA=1 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_$algo
                    else
                        if [ "$algo" == "starlike" ];
                        then
                            CCL_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_"$algo"_chunked
                        elif [ "$algo" == "ring" ];
                        then
                            CCL_RS_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_"$algo"_chunked
                        elif [ "$algo" == "2d" ];
                        then
                            CCL_AR2D_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_"$algo"_chunked
                        fi
                        CCL_ALLREDUCE=$algo ctest -VV -C mpi_allreduce_$algo
                    fi
                done

                for algo in "naive" "scatter" "scatter_barrier"
                do
                    if [ "$algo" == "scatter_barrier" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=1 CCL_ALLTOALL_SCATTER_PLAIN=1 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALL=$algo ctest -VV -C mpi_alltoall_"$algo"_chunked
                    fi
                    CCL_ALLTOALL=$algo ctest -VV -C mpi_alltoall_$algo
                done

                for algo in "naive" "scatter" "scatter_barrier"
                do
                    if [ "$algo" == "scatter_barrier" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=1 CCL_ALLTOALL_SCATTER_PLAIN=1 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALLV=$algo ctest -VV -C mpi_alltoallv_"$algo"_chunked
                    fi
                    CCL_ALLTOALLV=$algo ctest -VV -C mpi_alltoallv_$algo
                done

                for algo in "ring" "double_tree" "naive"
                do
                    CCL_BCAST=$algo ctest -VV -C mpi_bcast_$algo
                done

                for algo in "rabenseifner" "tree" "double_tree"
                do
                    CCL_REDUCE=$algo ctest -VV -C mpi_reduce_$algo
                done

                for algo in "ring"
                do
                    CCL_REDUCE_SCATTER=$algo ctest -VV -C mpi_reduce_scatter_$algo
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
            fusion_mode )
                CCL_ATL_TRANSPORT=ofi CCL_FUSION=1 ctest -VV -C Default
                CCL_ATL_TRANSPORT=mpi CCL_FUSION=1 ctest -VV -C Default
               ;;
           * )
                echo "Please specify runtime mode: runtime=ofi|mpi|ofi_adjust|mpi_adjust|priority_mode|unordered_coll_mode|dynamic_pointer_mode|fusion_mode|"
                exit 1
               ;;
    esac
}

clean_nodes() {
    echo "Start cleaning nodes..."
    if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
    then
        echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set, only current node will be cleaned."
        using_nodes=`hostname`
    else
        using_nodes=`cat ${I_MPI_HYDRA_HOST_FILE}`
    fi
    user='sys_ctl'
    exceptions='java\|awk\|bash\|grep\|intelremotemond\|sshd\|grep\|ps\|mc'
    for host_name in ${using_nodes}; do
     ssh  "$host_name" "bash -s $user $exceptions " <<'EOF'
       echo "Host: $(hostname)"
       User=$1
       Exceptions=$2
       ps -aux | grep PID | grep -v 'grep'
       for pid in $(ps aux | grep -e "^${User}" \
                   | grep -E -v "\b(${Exceptions})\b" \
                   | awk '{print $2}'); do
           echo "Killed:" &&  ps -aux | grep -v 'grep' | grep ${pid}
           echo "-------------------------------------------------"
           kill -6 ${pid}
       done
EOF
    done
    echo "Cleaning was finished"
}

#==============================================================================
#                              MAIN
#==============================================================================

set_default_values
set_impi_environment
set_environment

clean_nodes
while [ $# -ne 0 ]
do
    case $1 in
    "-compatibility_tests" )
        run_compatibitily_tests
        shift
        ;;
    "-modulefile_tests" )
        run_modulefile_tests
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
clean_nodes

