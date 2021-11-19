#!/bin/bash -x

# The script is only for Jenkins usage.
# It is invoked by jenkins' jobs with separate parametres.
# For the knobs - see help: sanity.sh -help

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
ARTEFACT_ROOT_DIR="/p/pdsd/scratch/jenkins/artefacts"
ARTEFACT_DIR="${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}/"
CCL_ONEAPI_DIR="/p/pdsd/scratch/Uploads/CCL_oneAPI/"
ONEAPI_DIR="/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/oneAPI/"
SOFTWARE_DIR="/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Software/"
HOSTNAME=`hostname -s`
US_PROXY="http://proxy-us.intel.com:912"

echo "SCRIPT_DIR = $SCRIPT_DIR"
echo "CURRENT_WORK_DIR = $CURRENT_WORK_DIR"

export CCL_WORKER_AFFINITY=auto

#==============================================================================
#                                Defaults
#==============================================================================
function set_default_values()
{
    ENABLE_DEBUG="no"
    ENABLE_VERBOSE="yes"

    # Testing knobs
    ENABLE_REGULAR_TESTS="no"
    ENABLE_HOROVOD_TESTS="no"
    ENABLE_PYTORCH_TESTS="no"
    ENABLE_MODULEFILE_TESTS="no"
    ENABLE_FUNCTIONAL_TESTS="no"
    ENABLE_VALGRIND_TESTS="no"
    ENABLE_REG_TESTS="no"
    ENABLE_MPICH_OFI_TESTS="no"
}
#==============================================================================
#                                Functions
#==============================================================================

# Logging. Always log. Echo if ENABLE_VERBOSE is "yes".
function log()
{
    if [ "${ENABLE_VERBOSE}" = "yes" ]; then
        eval $* 2>&1 | tee -a ${LOG_FILE}
    else
        eval $* >> ${LOG_FILE} 2>&1
    fi
}

function parse_arguments() {
    while [ $# -ne 0 ]
    do
        case $1 in
        "-regular_tests" )
            ENABLE_REGULAR_TESTS="yes"
            shift
            ;;
        "-compatibility_tests" )
            ENABLE_REGULAR_TESTS="yes"
            shift
            ;;
        "-horovod_tests" )
            ENABLE_HOROVOD_TESTS="yes"
            shift
            ;;
        "-pytorch_tests" )
            ENABLE_PYTORCH_TESTS="yes"
            shift
            ;;
        "-modulefile_tests" )
            ENABLE_MODULEFILE_TESTS="yes"
            shift
            ;;
        "-functional_tests" )
            ENABLE_FUNCTIONAL_TESTS="yes"
            shift
            ;;
        "-valgrind_check" )
            ENABLE_VALGRIND_TESTS="yes"
            shift
            ;;
        "-reg_tests" )
            ENABLE_REG_TESTS="yes"
            shift
            ;;
        "-mpich_ofi" )
            export ENABLE_MPICH_OFI_TESTS="yes"
            shift
            ;;
        *)
            echo "$(basename ${0}): ERROR: unknown option (${1})"
            print_help
            exit 1
            ;;
        esac
    done

    echo "ENABLE_REGULAR_TESTS      = ${ENABLE_REGULAR_TESTS}"
    echo "ENABLE_HOROVOD_TESTS      = ${ENABLE_HOROVOD_TESTS}"
    echo "ENABLE_PYTORCH_TESTS      = ${ENABLE_PYTORCH_TESTS}"
    echo "ENABLE_MODULEFILE_TESTS   = ${ENABLE_MODULEFILE_TESTS}"
    echo "ENABLE_FUNCTIONAL_TESTS   = ${ENABLE_FUNCTIONAL_TESTS}"
    echo "ENABLE_VALGRIND_TESTS     = ${ENABLE_VALGRIND_TESTS}"
    echo "ENABLE_REG_TESTS          = ${ENABLE_REG_TESTS}"
    echo "ENABLE_MPICH_OFI_TESTS    = ${ENABLE_MPICH_OFI_TESTS}"
}

# Print usage and help information
function print_help()
{
    echo -e "Usage:\n" \
    "    ./${BASENAME}.sh <options> \n" \
    "<options>:\n" \
    "-regular_tests:       run examples\n" \
    "-modulefile_tests:    run modulefile tests\n" \
    "-functional_tests:    run functional tests\n" \
    "-valgrind_check:      run valgrind check\n" \
    "-reg_tests:           run regression tests\n" \
    "-mpich_ofi:           run tests with mpich-ofi\n" \
    "-help:                print this help information"
    exit 0
}

# Echo with logging. Always echo and always log
function echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

# echo the debug information
function echo_debug()
{
    if [ ${ENABLE_DEBUG} = "yes" ]; then
        echo_log "DEBUG: $*"
    fi
}

function echo_log_separator()
{
    echo_log "#=============================================================================="
}

function check_command_exit_code() {
    local arg=$1
    local err_msg=$2

    if [ $arg -ne 0 ]; then
        echo_log "ERROR: ${err_msg}" 1>&2
        exit $1
    fi
}

function set_external_env() {
    if [ ${TEST_CONFIGURATIONS} == "test:nightly" ]
    then
        echo "Nightly test scope will be started."
        export TESTING_ENVIRONMENT="\
        CCL_TEST_DATA_TYPE=1 \
        CCL_TEST_SIZE_TYPE=1 \
        CCL_TEST_BUF_COUNT_TYPE=1 \
        CCL_TEST_PLACE_TYPE=1 \
        CCL_TEST_START_ORDER_TYPE=1 \
        CCL_TEST_COMPLETE_ORDER_TYPE=0 \
        CCL_TEST_COMPLETE_TYPE=0 \
        CCL_TEST_CACHE_TYPE=1 \
        CCL_TEST_SYNC_TYPE=1 \
        CCL_TEST_REDUCTION_TYPE=1"
    elif [[ ${TEST_CONFIGURATIONS} == "test:pr" ]]
    then
        export scope="pr"
    else
        export scope="all"
    fi

    if [[ ${BUILDER_NAME} == "ccl-nightly" ]] || [[ ${BUILDER_NAME} == "ccl-weekly" ]]
    then
        export valgrind_scope="regular"
    else
        export valgrind_scope="short"
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

function set_mpich_ofi_env() {
    if [[ ${ENABLE_MPICH_OFI_TESTS} = "no" ]]
    then
        return
    fi

    ${CURRENT_WORK_DIR}/scripts/mpich_ofi/mpich_ofi.sh
    MPICH_OFI_INSTALL_PATH=$( cd ${CURRENT_WORK_DIR}/scripts/mpich_ofi/install/usr/mpi/mpich-ofi*/ && pwd -P )
    export LD_LIBRARY_PATH="${MPICH_OFI_INSTALL_PATH}/lib:${LD_LIBRARY_PATH}"
    unset FI_PROVIDER_PATH
    mkdir -p ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    cp ${I_MPI_ROOT}/libfabric/lib/prov/libpsm3-fi.so ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    export FI_PROVIDER_PATH="${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov"
}

function set_provider_env() {
    if [[ "${node_label}" = "ccl_test_ats" ]] &&
       [[ ${ENABLE_FUNCTIONAL_TESTS} = "yes" ||
          ${ENABLE_REGULAR_TESTS} = "yes" ||
          ${ENABLE_REG_TESTS} = "yes" ]]
    then
        export FI_PROVIDER=tcp
    fi

    if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
    then
        if [[ "${node_label}" = "ccl_test_ats" ]] &&
           [[ ${ENABLE_REG_TESTS} = "yes" ]]
        then
            export FI_PROVIDER=sockets
        else
            export FI_PROVIDER=psm3
        fi
    fi
}

function set_transport_env() {
    if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
    then
        export CCL_ATL_TRANSPORT_LIST="mpi"
    else
        export CCL_ATL_TRANSPORT_LIST="ofi mpi"
    fi

    if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
    then
        REG_TESTS_TRANSPORT=mpich
    else
        REG_TESTS_TRANSPORT=impi
    fi
}

function set_runtime_env() {
    export CCL_RUNTIME_LIST="level_zero"
}

function set_functional_tests_env()
{
    echo "Use default env"
    export CCL_LOG_LEVEL=info
    # flush cache inside ccl::barrier to avoid OOM
    # in case of caching and large number of different match_ids
    export CCL_CACHE_FLUSH=1
    export CCL_MNIC=global
    export I_MPI_DEBUG=12

    if [[ "${node_label}" = "ccl_test_ats" ]]
    then
        export CCL_MNIC_NAME=eno,eth,hfi,^mlx,unknown
    else
        export CCL_MNIC_NAME=br,eno,eth,hfi,mlx,^unknown
    fi
}

function set_functional_tests_scope()
{
    echo "Set default func tests scope"
    export CCL_TEST_DATA_TYPE=1
    export CCL_TEST_SIZE_TYPE=1
    export CCL_TEST_BUF_COUNT_TYPE=1
    export CCL_TEST_PLACE_TYPE=1
    export CCL_TEST_START_ORDER_TYPE=0
    export CCL_TEST_COMPLETE_ORDER_TYPE=0
    export CCL_TEST_COMPLETE_TYPE=0
    export CCL_TEST_CACHE_TYPE=1
    export CCL_TEST_SYNC_TYPE=0
    export CCL_TEST_REDUCTION_TYPE=0

    if [[ ${node_label} == "ccl_test_gen9" ]] || [[ ${node_label} == "ccl_test_ats" ]]
    then
        export CCL_TEST_DYNAMIC_POINTER=0
    else
        export CCL_TEST_DYNAMIC_POINTER=1
    fi
}

function set_unordered_coll_test_scope()
{
    echo "Set unordered coll tests scope"
    export CCL_TEST_DATA_TYPE=0
    export CCL_TEST_SIZE_TYPE=1
    export CCL_TEST_BUF_COUNT_TYPE=1
    export CCL_TEST_PLACE_TYPE=0
    export CCL_TEST_START_ORDER_TYPE=1
    export CCL_TEST_COMPLETE_ORDER_TYPE=1
    export CCL_TEST_COMPLETE_TYPE=1
    export CCL_TEST_CACHE_TYPE=1
    export CCL_TEST_SYNC_TYPE=0
    export CCL_TEST_REDUCTION_TYPE=0
}

function set_ats_environment()
{
    source ${SCRIPT_DIR}/ats_helper.sh
    source ~/.jenkins_helper

    ATS_WORKSPACE_DIR="/home/sys_ctlab/workspace/workspace/"
    ATS_ARTEFACT_DIR="${ATS_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
    export BUILD_COMPILER_TYPE="dpcpp"
    export SYCL_BUNDLE_ROOT="/home/sys_ctlab/oneapi/compiler/last/compiler/latest/linux/"
    export ICX_BUNDLE_ROOT="/home/sys_ctlab/oneapi/compiler/last/compiler/latest/linux/"
    export IMPI_PATH="/home/sys_ctlab/oneapi/mpi_oneapi/last/mpi/latest/"
    #export CCL_STAGING_BUFFER=regular

    if [ -z ${CCL_ROOT} ]
    then
        install_oneccl_package ${ATS_ARTEFACT_DIR}
        source ${ATS_ARTEFACT_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        export DASHBOARD_GPU_DEVICE_PRESENT="yes"
    fi
    export DASHBOARD_PLATFORM_HW_DISCRETE_GPU="ats"
}

function set_environment()
{
    if [ -z "${build_type}" ]
    then
        export build_type="release"
    fi

    if [[ "${node_label}" = "ccl_test_ats" ]]
    then
        set_ats_environment
    fi

    # $BUILD_COMPILER_TYPE may be set up by user: dpcpp/gnu/intel/icx
    if [ -z "${BUILD_COMPILER_TYPE}" ]
    then
        if [ ${node_label} == "ccl_test_gen9" ] || [ ${node_label} == "ccl_vlgd" ] || [ ${build_compiler} == "sycl" ]
        then
            BUILD_COMPILER_TYPE="dpcpp"
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        elif [ ${node_label} == "ccl_test_cpu" ] || [ ${build_compiler} == "gnu" ]
        then
            BUILD_COMPILER_TYPE="gnu"
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu
        elif [ ${build_compiler} == "intel" ]
        then
            BUILD_COMPILER_TYPE="intel"
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu
        elif [ ${build_compiler} == "icx" ]
        then
            BUILD_COMPILER_TYPE="icx"
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu
        else
            echo "WARNING: the 'node_label' variable is not set. 'dpcpp' will be used by default."
            BUILD_COMPILER_TYPE="dpcpp"
        fi
    fi

    if [ "${BUILD_COMPILER_TYPE}" == "gnu" ]
    then
        BUILD_COMPILER_PATH=/usr/bin
        C_COMPILER=${BUILD_COMPILER_PATH}/gcc
        CXX_COMPILER=${BUILD_COMPILER_PATH}/g++
    elif [ "${BUILD_COMPILER_TYPE}" = "intel" ]
    then
        BUILD_COMPILER_PATH=/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/
        source ${BUILD_COMPILER_PATH}/linux/bin/compilervars.sh intel64
        C_COMPILER=${BUILD_COMPILER_PATH}/linux/bin/intel64/icc
        CXX_COMPILER=${BUILD_COMPILER_PATH}/linux/bin/intel64/icpc
    elif [ "${BUILD_COMPILER_TYPE}" = "icx" ]
    then
        if [ -z "${ICX_BUNDLE_ROOT}" ]
        then
            ICX_BUNDLE_ROOT="${CCL_ONEAPI_DIR}/compiler/last/compiler/latest/linux/"
            echo "WARNING: ICX_BUNDLE_ROOT is not defined, will be used default: $ICX_BUNDLE_ROOT"
        fi
        source ${ICX_BUNDLE_ROOT}/../../../setvars.sh
        BUILD_COMPILER_PATH=${ICX_BUNDLE_ROOT}/bin
        C_COMPILER=${BUILD_COMPILER_PATH}/icx
        CXX_COMPILER=${BUILD_COMPILER_PATH}/icpx
    elif [ "${BUILD_COMPILER_TYPE}" = "dpcpp" ]
    then
        if [ -z "${SYCL_BUNDLE_ROOT}" ]
        then
            SYCL_BUNDLE_ROOT="${CCL_ONEAPI_DIR}/compiler/last/compiler/latest/linux/"
            echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: $SYCL_BUNDLE_ROOT"
        fi
        source ${SYCL_BUNDLE_ROOT}/../../../setvars.sh
        BUILD_COMPILER_PATH=${SYCL_BUNDLE_ROOT}/bin
        C_COMPILER=${BUILD_COMPILER_PATH}/icx
        CXX_COMPILER=${BUILD_COMPILER_PATH}/dpcpp
        COMPUTE_BACKEND=dpcpp_level_zero
    else
        echo "ERROR: unsupported BUILD_COMPILER_TYPE"
        exit 1
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
        export build_type="release"
    fi
    if [ -z ${CCL_ROOT} ]
    then
        if [ -z  "${node_label}" ]
        then
            source ${ARTEFACT_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu
        elif [ $node_label == "ccl_test_gen9" ] || [ $node_label == "ccl_vlgd" ]
        then
            source ${ARTEFACT_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
            export DASHBOARD_GPU_DEVICE_PRESENT="yes"
        else
            source ${ARTEFACT_DIR}/l_ccl_$build_type*/env/vars.sh --ccl-configuration=cpu
        fi
    fi

    export CCL_PROCESS_CLEANUP="yes"
    set_external_env
}

function set_reg_tests_environment()
{
    export CCL_WORKER_COUNT=1
    export CCL_LOG_LEVEL=info
    export I_MPI_DEBUG=12
    if [[ "${node_label}" = "ccl_test_gen9" ]]
    then
        export CCL_WORKER_OFFLOAD=0
        export CCL_YIELD=sched_yield
    fi
}

function set_impi_environment()
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
        echo "WARNING: IMPI_PATH isn't set"
        if [[ -d "${ARTEFACT_ROOT_DIR}/impi-ch4-weekly/last/oneapi_impi/" ]]
        then
            weekly_mpiexec_version=$(${ARTEFACT_ROOT_DIR}/impi-ch4-weekly/last/oneapi_impi/bin/mpiexec --version)
            weekly_build_date=$(echo ${weekly_mpiexec_version#*Build} | awk '{print $1;}')

            lkg_mpiexec_version=$(${CCL_ONEAPI_DIR}/mpi_oneapi/last/mpi/latest/bin/mpiexec --version)
            lkg_build_date=$(echo ${lkg_mpiexec_version#*Build} | awk '{print $1;}')

            if [ "$weekly_build_date" = "$lkg_build_date" ]; then
                export IMPI_PATH="${CCL_ONEAPI_DIR}/mpi_oneapi/last/mpi/latest/"
            elif expr "$weekly_build_date" "<" "$lkg_build_date" >/dev/null; then
                export IMPI_PATH="${CCL_ONEAPI_DIR}/mpi_oneapi/last/mpi/latest/"
            else
                export IMPI_PATH="${ARTEFACT_ROOT_DIR}/impi-ch4-weekly/last/oneapi_impi"
            fi
        else
            export IMPI_PATH="${CCL_ONEAPI_DIR}/mpi_oneapi/last/mpi/latest/"
        fi
    fi
    source ${IMPI_PATH}/env/vars.sh
}

function make_tests()
{
    cd ${CURRENT_WORK_DIR}/tests/functional
    mkdir -p build
    cd ./build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="${C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" -DCOMPUTE_BACKEND="${COMPUTE_BACKEND}"
    make all
    check_command_exit_code $? "Compilation of functional tests is FAILED"
}

function run_valgrind_check()
{
    if [[ ${ENABLE_VALGRIND_TESTS} = "no" ]]
    then
        return
    fi
    export EXAMPLE_WORK_DIR="${CURRENT_WORK_DIR}/examples/build"
    unset I_MPI_HYDRA_HOST_FILE
    mkdir -p ${EXAMPLE_WORK_DIR}
    echo "EXAMPLE_WORK_DIR =" $EXAMPLE_WORK_DIR
    cd ${EXAMPLE_WORK_DIR}
    if [ ${node_label} == "ccl_vlgd" ]
    then
        export FI_TCP_IFACE=eno1
        ${CURRENT_WORK_DIR}/scripts/valgrind/valgrind.sh gpu ${valgrind_scope}
    else
        ${CURRENT_WORK_DIR}/scripts/valgrind/valgrind.sh cpu ${valgrind_scope}
    fi
    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "valgrind testing ... OK"
        exit 0
    else
        echo "valgrind testing ... NOK"
        exit 1
    fi
}

function run_reg_tests()
{
    if [[ ${ENABLE_REG_TESTS} = "no" ]]
    then
        return
    fi

    set_reg_tests_environment

    if [[ ${node_label} == "ccl_test_gen9" ]]
    then
        ${CURRENT_WORK_DIR}/tests/reg_tests/run.sh --mode gpu --platform gen9
    elif [[ ${node_label} == "ccl_test_ats" ]]
    then
        ${CURRENT_WORK_DIR}/tests/reg_tests/run.sh --mode gpu --platform ats --transport ${REG_TESTS_TRANSPORT}
    else
        ${CURRENT_WORK_DIR}/tests/reg_tests/run.sh --mode cpu
    fi

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "regression testing ... OK"
        exit 0
    else
        echo "regression testing ... NOK"
        exit 1
    fi
}

function run_regular_tests()
{
    if [[ ${ENABLE_REGULAR_TESTS} = "no" ]]
    then
        return
    fi

    if [[ ${node_label} == "ccl_test_gen9" ]] || [[ ${node_label} == "ccl_test_ats" ]]
    then
        export FI_TCP_IFACE=eno1
        export DASHBOARD_GPU_DEVICE_PRESENT=1
        ${CURRENT_WORK_DIR}/examples/run.sh --mode gpu --scope ${scope} --cleanup
    else
        ${CURRENT_WORK_DIR}/examples/run.sh --mode cpu --scope ${scope} --cleanup
    fi

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "regular testing ... OK"
        exit 0
    else
        echo "regular testing ... NOK"
        exit 1
    fi
}


function run_horovod_tests()
{
    if [[ ${ENABLE_HOROVOD_TESTS} = "no" ]]
    then
        return
    fi

    # IPEX has a dependency for mkl and tbb
    source ${CCL_ONEAPI_DIR}/onemkl/last/mkl/latest/env/vars.sh
    source ${CCL_ONEAPI_DIR}/tbb_oneapi/last/tbb/latest/env/vars.sh

    pushd ${CURRENT_WORK_DIR}/scripts/framework/horovod/
    ./horovod.sh -download_pt 1 -install_pt 1 \
                 -download_ipex 1 -install_ipex 1 \
                 -download_hvd 1 -install_hvd 1 \
                 -download_conda 1 -create_conda 1 -remove_conda 1 \
                 -transport mpi -provider ${FI_PROVIDER} \
                 -token "${CURRENT_WORK_DIR}/gitpass.sh" -username ${USERNAME_1S}
    log_status_fail=${PIPESTATUS[0]}
    popd
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "Horovod testing ... OK"
    else
        echo "Horovod testing ... NOK"
        exit 1
    fi

    pushd ${CURRENT_WORK_DIR}/scripts/framework/horovod/
    ./horovod.sh -install_tf 1 -tf_path "${SOFTWARE_DIR}/Tensorflow/latest" \
                 -install_itex 1 -itex_path "${SOFTWARE_DIR}/ITEX/latest" \
                 -download_hvd 1 -install_hvd 1 \
                 -download_conda 1 -create_conda 1 -remove_conda 1 \
                 -transport mpi -provider ${FI_PROVIDER} \
                 -token "${CURRENT_WORK_DIR}/gitpass.sh" -username ${USERNAME_1S}
    log_status_fail=${PIPESTATUS[0]}
    popd
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "Horovod testing ... OK"
    else
        echo "Horovod testing ... NOK"
        exit 1
    fi

    exit 0
}

function run_pytorch_tests()
{
    if [[ ${ENABLE_PYTORCH_TESTS} = "no" ]]
    then
        return
    fi

    pushd ${CURRENT_WORK_DIR}/scripts/framework/pytorch/
    ./pytorch.sh -full 1 -proxy ${US_PROXY} -remove_conda 1\
                 -token "${CURRENT_WORK_DIR}/gitpass.sh" -username ${USERNAME_1S}
    log_status_fail=${PIPESTATUS[0]}
    popd
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "PyTorch testing ... OK"
    else
        echo "PyTorch testing ... NOK"
        exit 1
    fi

    exit 0
}

function set_modulefile_environment()
{
    source /etc/profile.d/modules.sh
    export CCL_CONFIGURATION=cpu
    if [ -z "$build_type" ]
    then
        build_type="release"
    fi

    module load ${ARTEFACT_DIR}/l_ccl_${build_type}*/modulefiles/ccl

    log_status_fail=${PIPESTATUS[0]}
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "module file load ... OK"
    else
        echo "module file load ... NOK"
        exit 1
    fi
}

function unset_modulefile_environment()
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

function run_modulefile_tests()
{
    if [[ ${ENABLE_MODULEFILE_TESTS} = "no" ]]
    then
        return
    fi

    set_modulefile_environment

    ${CURRENT_WORK_DIR}/examples/run.sh --mode cpu --scope $scope

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

function run_functional_tests()
{
    if [[ ${ENABLE_FUNCTIONAL_TESTS} = "no" ]]
    then
        return
    fi

    make_tests
    set_functional_tests_env
    set_functional_tests_scope

    ppns="1 2"
    allgatherv_algos="naive flat ring"
    allreduce_algos="rabenseifner nreduce ring double_tree recursive_doubling"
    alltoall_algos="naive scatter"
    alltoallv_algos=${alltoall_algos}
    bcast_algos="ring double_tree naive"
    reduce_algos="rabenseifner tree"
    reduce_scatter_algos="ring"

    if [ ${runtime} == "mpi_adjust" ]
    then
        allgatherv_algos="${allgatherv_algos} direct"
        allreduce_algos="${allreduce_algos} direct"
        alltoall_algos="${alltoall_algos} direct"
        alltoallv_algos=${alltoall_algos}
        bcast_algos="${bcast_algos} direct"
        reduce_algos="${reduce_algos} direct"
        reduce_scatter_algos="${reduce_scatter_algos} direct"
    fi

    if [ ${runtime} == "ofi_adjust" ]
    then
        allgatherv_algos="${allgatherv_algos} multi_bcast"
        allreduce_algos="${allreduce_algos} 2d" # ring_rma
    fi

    if [ ${node_label} == "ccl_test_gen9" ] || [ "${node_label}" == "ccl_test_ats" ]
    then
        allreduce_algos="${allreduce_algos} topo"
        bcast_algos="${bcast_algos} topo"
        reduce_algos="${reduce_algos} topo"
    fi

    case "$runtime" in
           ofi )
               export CCL_ATL_TRANSPORT=ofi
               ctest -VV -C default
               ctest -VV -C regression
               ;;
           mpi )
                export CCL_ATL_TRANSPORT=mpi
                ctest -VV -C default
                ctest -VV -C regression
                ;;
           mpi_adjust )
                export CCL_ATL_TRANSPORT=mpi

                for algo in ${allgatherv_algos}
                do
                    CCL_ALLGATHERV=$algo ctest -VV -C allgatherv_"$algo"
                done

                for algo in ${allreduce_algos}
                do
                    if [ "$algo" == "ring" ];
                    then
                        CCL_RS_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_chunked
                    elif [ "$algo" == "nreduce" ];
                    then
                        CCL_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_chunked
                    fi
                done

                for ppn in $ppns
                do
                    for algo in ${allreduce_algos}
                    do
                        CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_"$ppn"
                    done

                    for algo in ${bcast_algos}
                    do
                        CCL_BCAST=$algo ctest -VV -C bcast_"$algo"_"$ppn"
                    done

                    for algo in ${reduce_algos}
                    do
                        CCL_REDUCE=$algo ctest -VV -C reduce_"$algo"_"$ppn"
                    done
                done

                for algo in ${alltoall_algos}
                do
                    if [ "$algo" == "scatter" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=2 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALL=$algo ctest -VV -C alltoall_"$algo"_chunked
                    fi
                    CCL_ALLTOALL=$algo ctest -VV -C alltoall_"$algo"
                done

                for algo in ${alltoallv_algos}
                do
                    if [ "$algo" == "scatter" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=2 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALLV=$algo ctest -VV -C alltoallv_"$algo"_chunked
                    fi
                    CCL_ALLTOALLV=$algo ctest -VV -C alltoallv_"$algo"
                done

                for algo in ${reduce_scatter_algos}
                do
                    CCL_REDUCE_SCATTER=$algo ctest -VV -C reduce_scatter_"$algo"
                done
               ;;
           ofi_adjust )

                export CCL_ATL_TRANSPORT=ofi

                for algo in ${allgatherv_algos}
                do
                    CCL_ALLGATHERV=$algo ctest -VV -C allgatherv_"$algo"
                done

                for algo in ${allreduce_algos}
                do
                    if [ "$algo" == "ring_rma" ];
                    then
                        CCL_ATL_RMA=1 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"
                    else
                        if [ "$algo" == "nreduce" ];
                        then
                            CCL_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_chunked
                        elif [ "$algo" == "ring" ];
                        then
                            CCL_RS_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_chunked
                        elif [ "$algo" == "2d" ];
                        then
                            CCL_AR2D_CHUNK_COUNT=2 CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_chunked
                        fi
                    fi
                done

                for ppn in $ppns
                do
                    for algo in ${allreduce_algos}
                    do
                        CCL_ALLREDUCE=$algo ctest -VV -C allreduce_"$algo"_"$ppn"
                    done

                    for algo in ${bcast_algos}
                    do
                        CCL_BCAST=$algo ctest -VV -C bcast_"$algo"_"$ppn"
                    done

                    for algo in ${reduce_algos}
                    do
                        CCL_REDUCE=$algo ctest -VV -C reduce_"$algo"_"$ppn"
                    done
                done

                for algo in ${alltoall_algos}
                do
                    if [ "$algo" == "scatter" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=2 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALL=$algo ctest -VV -C alltoall_"$algo"_chunked
                    fi
                    CCL_ALLTOALL=$algo ctest -VV -C alltoall_"$algo"
                done

                for algo in ${alltoallv_algos}
                do
                    if [ "$algo" == "scatter" ];
                    then
                        CCL_ALLTOALL_SCATTER_MAX_OPS=2 CCL_CHUNK_COUNT=${worker_count} \
                            CCL_ALLTOALLV=$algo ctest -VV -C alltoallv_"$algo"_chunked
                    fi
                    CCL_ALLTOALLV=$algo ctest -VV -C alltoallv_"$algo"
                done

                for algo in ${reduce_scatter_algos}
                do
                    CCL_REDUCE_SCATTER=$algo ctest -VV -C reduce_scatter_"$algo"
                done
               ;;
            priority_mode )
                # TODO: At the moment priority_mode and unordered_coll_mode launches only
                # with CCL_ATS_TRANSPORT=ofi, so these confs are missed in mpich_ofi testing.
                # We would like to add them in the future.
                CCL_ATL_TRANSPORT=ofi CCL_PRIORITY=lifo ctest -VV -C default
                CCL_ATL_TRANSPORT=ofi CCL_PRIORITY=direct ctest -VV -C default
               ;;
            dynamic_pointer_mode )
                for transport in ${CCL_ATL_TRANSPORT_LIST}
                do
                    CCL_ATL_TRANSPORT=${transport} ctest -VV -C default
                done
               ;;
            unordered_coll_mode )
                set_unordered_coll_test_scope
                CCL_ATL_TRANSPORT=ofi CCL_UNORDERED_COLL=1 ctest -VV -C default
               ;;
            fusion_mode )
                for transport in ${CCL_ATL_TRANSPORT_LIST}
                do
                    CCL_ATL_TRANSPORT=${transport} CCL_FUSION=1 ctest -VV -C allreduce_fusion
                done
               ;;
           * )
                echo "Please specify runtime mode: runtime=ofi|mpi|ofi_adjust|mpi_adjust|priority_mode|unordered_coll_mode|dynamic_pointer_mode|fusion_mode|"
                exit 1
               ;;
    esac
}

function clean_nodes() {
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
     ssh  "${host_name}" "bash -s ${user} ${exceptions} " <<'EOF'
       echo "Host: $(hostname)"
       User=$1
       Exceptions=$2
       ps -aux | grep PID | grep -v 'grep'
       for pid in $(ps aux | grep -e "^${User}" \
                   | grep -v "${Exceptions}" \
                   | awk '{print $2}'); do
           (cat /proc/${pid}/environ | tr '\0' '\n' | grep "CCL_PROCESS_CLEANUP=yes") >/dev/null 2>&1
           rc=$?
           if [[ ${rc} = 0 ]]
           then
               echo "Killed:"
               ps -aux | grep -v 'grep' | grep ${pid}
               echo "-------------------------------------------------"
               kill -9 ${pid}
           fi
       done
EOF
    done
    echo "Start cleaning nodes...DONE"
}

#==============================================================================
#                              MAIN
#==============================================================================

clean_nodes
set_default_values
set_environment
set_impi_environment
parse_arguments "$@"
set_provider_env
set_transport_env
set_runtime_env

set_mpich_ofi_env
run_horovod_tests
run_pytorch_tests
run_modulefile_tests
run_functional_tests
run_valgrind_check
run_reg_tests
run_regular_tests

clean_nodes
