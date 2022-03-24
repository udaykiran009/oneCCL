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

BASENAME=`basename $0 .sh`
if [[ -z ${CURRENT_WORK_DIR} ]]
then
    CURRENT_WORK_DIR=`cd ${SCRIPT_DIR}/../../ && pwd -P`
fi
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

source ${CURRENT_WORK_DIR}/scripts/utils/common.sh
source ${CURRENT_WORK_DIR}/scripts/utils/ats_helper.sh
source ${CURRENT_WORK_DIR}/scripts/utils/pvc_helper.sh

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
    ENABLE_TORCH_CCL_TESTS="no"
    ENABLE_MODULEFILE_TESTS="no"
    ENABLE_FUNCTIONAL_TESTS="no"
    ENABLE_VALGRIND_TESTS="no"
    ENABLE_REG_TESTS="no"
    ENABLE_MPICH_OFI_TESTS="no"

    if [[ ${node_label} = "ccl_test_ats" ]]
    then
        PROC_MAPS="2:2/4:4"
    elif [[ ${node_label} = "ccl_test_pvc" ]]
    then
        PROC_MAPS="2:1,2"
    else
        PROC_MAPS="2:1,2/4:2,4"
    fi

    is_gpu_node
}
#==============================================================================
#                                Functions
#==============================================================================

function parse_arguments() {
    if [ $# -eq 0 ]; then
        print_help
        exit 1
    fi

    while [ $# -ne 0 ]
    do
        case $1 in
        "-regular_tests" )
            ENABLE_REGULAR_TESTS="yes"
            ;;
        "-horovod_tests" )
            ENABLE_HOROVOD_TESTS="yes"
            ;;
        "-pytorch_tests" )
            ENABLE_PYTORCH_TESTS="yes"
            ;;
        "-torch_ccl_tests" )
            ENABLE_TORCH_CCL_TESTS="yes"
            ;;
        "-modulefile_tests" )
            ENABLE_MODULEFILE_TESTS="yes"
            ;;
        "-functional_tests" )
            ENABLE_FUNCTIONAL_TESTS="yes"
            ;;
        "-valgrind_check" )
            ENABLE_VALGRIND_TESTS="yes"
            ;;
        "-reg_tests" )
            ENABLE_REG_TESTS="yes"
            ;;
        "-mpich_ofi" )
            export ENABLE_MPICH_OFI_TESTS="yes"
            ;;
        "-proc_maps" )
            PROC_MAPS="${2}"
            shift
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
    echo "ENABLE_PYTORCH_TESTS      = ${ENABLE_PYTORCH_TESTS}"
    echo "ENABLE_MODULEFILE_TESTS   = ${ENABLE_MODULEFILE_TESTS}"
    echo "ENABLE_FUNCTIONAL_TESTS   = ${ENABLE_FUNCTIONAL_TESTS}"
    echo "ENABLE_VALGRIND_TESTS     = ${ENABLE_VALGRIND_TESTS}"
    echo "ENABLE_REG_TESTS          = ${ENABLE_REG_TESTS}"
    echo "ENABLE_MPICH_OFI_TESTS    = ${ENABLE_MPICH_OFI_TESTS}"
    echo "PROC_MAPS                 = ${PROC_MAPS}"
}

# Print usage and help information
function print_help()
{
    echo -e "Usage:\n" \
    "    ./${BASENAME}.sh <options> \n" \
    "<options>:\n" \
    "-regular_tests:       run examples\n" \
    "-horovod_tests:       run Horovod tests"\
    "-pytorch_tests:       run PyTorch tests"\
    "-torch_ccl_tests:     run Torch-ccl tests"\
    "-modulefile_tests:    run modulefile tests\n" \
    "-functional_tests:    run functional tests\n" \
    "-valgrind_check:      run valgrind check\n" \
    "-reg_tests:           run regression tests\n" \
    "-mpich_ofi:           run tests with mpich-ofi\n" \
    "-proc_maps:           set value for n and ppns\n" \
    "-help:                print this help information"\
    "   example: ./${BASENAME}.sh -proc_maps 2:1,2/4:1,2,4\n"
    exit 0
}

function set_external_env() {
    if [[ ${TEST_CONFIGURATIONS} == "test:pr" ]]
    then
        export regular_scope="pr"
    else
        export regular_scope="all"
    fi

    if [[ ${BUILDER_NAME} == "ccl-nightly" ]] || [[ ${BUILDER_NAME} == "ccl-weekly" ]]
    then
        export valgrind_scope="regular"
    else
        export valgrind_scope="short"
    fi
}

function set_mpich_ofi_env() {
    if [[ ${ENABLE_MPICH_OFI_TESTS} = "no" ]]
    then
        return
    fi

    export HYDRA_LAUNCHER="ssh"
    export HYDRA_LAUNCHER_EXEC="/usr/bin/ssh"

    if [[ ${node_label} = "ccl_test_ats" ]]
    then
        set_ats_mpich_ofi_env
    elif [[ ${node_label} = "ccl_test_pvc" ]]
    then
        set_pvc_mpich_ofi_env
    fi
}

function set_provider_env() {

    if [[ "${node_label}" = "ccl_test_ats" ]]
    then
        if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
        then
            export FI_PROVIDER=sockets
        else
            export FI_PROVIDER=psm3
        fi
    elif [[ "${node_label}" = "ccl_test_gen9" ]]
    then
        export FI_PROVIDER=tcp
    elif [[ "${node_label}" = "ccl_test_pvc" ]]
    then
        export FI_PROVIDER=cxi
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

function set_regular_tests_environment()
{
    export I_MPI_JOB_TIMEOUT=360
}

function set_functional_tests_env()
{
    echo "Use default env"

    func_exec_env+=" CCL_LOG_LEVEL=info"
    # flush cache inside ccl::barrier to avoid OOM
    # in case of caching and large number of different match_ids
    func_exec_env+=" CCL_CACHE_FLUSH=1"
    func_exec_env+=" CCL_MNIC=global"
    func_exec_env+=" I_MPI_DEBUG=12"
    func_exec_env+=" I_MPI_JOB_TIMEOUT=360"

    if [[ "${node_label}" = "ccl_test_gen9" ]]
    then
        func_exec_env+=" CCL_YIELD=sched_yield"
    fi

    if [[ "${node_label}" = "ccl_test_ats" ]]
    then
        func_exec_env+=" CCL_MNIC_NAME=eno,eth,hfi,mlx,^unknown"
    elif [[ "${node_label}" = "ccl_test_pvc" ]]
    then
        func_exec_env+=" CCL_MNIC_NAME=cxi,eno,eth,hfi,mlx,^unknown"
    else
        func_exec_env+=" CCL_MNIC_NAME=br,eno,eth,hfi,mlx,^unknown"
    fi
}

function set_functional_tests_scope()
{
    echo "Set default func tests scope"
    func_exec_env+=" CCL_TEST_DATA_TYPE=1"
    func_exec_env+=" CCL_TEST_SIZE_TYPE=1"
    func_exec_env+=" CCL_TEST_BUF_COUNT_TYPE=1"
    func_exec_env+=" CCL_TEST_PLACE_TYPE=1"
    func_exec_env+=" CCL_TEST_START_ORDER_TYPE=0"
    func_exec_env+=" CCL_TEST_COMPLETE_ORDER_TYPE=0"
    func_exec_env+=" CCL_TEST_COMPLETE_TYPE=0"
    func_exec_env+=" CCL_TEST_CACHE_TYPE=1"
    func_exec_env+=" CCL_TEST_SYNC_TYPE=0"
    func_exec_env+=" CCL_TEST_REDUCTION_TYPE=0"

    if [[ ${IS_GPU_NODE} = "yes" ]]
    then
        func_exec_env+=" CCL_TEST_DYNAMIC_POINTER=0"
    else
        func_exec_env+=" CCL_TEST_DYNAMIC_POINTER=1"
    fi
}

function set_unordered_coll_test_scope()
{
    echo "Set unordered coll tests scope"
    func_exec_env=$(set_tests_option "CCL_TEST_DATA_TYPE=0" "${func_exec_env}")
    func_exec_env=$(set_tests_option "CCL_TEST_PLACE_TYPE=0" "${func_exec_env}")
    func_exec_env=$(set_tests_option "CCL_TEST_START_ORDER_TYPE=1" "${func_exec_env}")
    func_exec_env=$(set_tests_option "CCL_TEST_COMPLETE_ORDER_TYPE=1" "${func_exec_env}")
    func_exec_env=$(set_tests_option "CCL_TEST_COMPLETE_TYPE=1" "${func_exec_env}")
}

function set_ats_environment()
{
    ATS_WORKSPACE_DIR="/home/sys_ctlab/jenkins/workspace/workspace/"
    ATS_ARTEFACT_DIR="${ATS_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
    CCL_ONEAPI_DIR="/home/sys_ctlab/oneapi"
    export BUILD_COMPILER_TYPE="dpcpp"
    export IMPI_PATH="${CCL_ONEAPI_DIR}/mpi_oneapi/last/mpi/latest/"

    if [ -z ${CCL_ROOT} ]
    then
        source ${ATS_ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        export DASHBOARD_GPU_DEVICE_PRESENT="yes"
    fi
    export DASHBOARD_PLATFORM_HW_DISCRETE_GPU="ats"
}

function set_pvc_environment()
{
    # Unload default system's oneapi package
    module unload oneapi

    module load cmake
    PVC_WORKSPACE_DIR="/home/sys_ctlab/workspace/workspace/"
    PVC_ARTEFACT_DIR="${PVC_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"

    # Compiler
    export BUILD_COMPILER_TYPE="dpcpp"
    export SYCL_BUNDLE_ROOT="/home/sys_ctlab/oneapi/compiler/last/"

    if [ -z ${CCL_ROOT} ]
    then
        source ${PVC_ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
        export DASHBOARD_GPU_DEVICE_PRESENT="yes"
    fi

    # Get slurm job nodelist
    scontrol show hostnames > "${PVC_ARTEFACT_DIR}/nodelist"
    export I_MPI_HYDRA_HOST_FILE="${PVC_ARTEFACT_DIR}/nodelist"
    export HYDRA_HOST_FILE="${PVC_ARTEFACT_DIR}/nodelist"
    export HYDRA_LAUNCHER="ssh"

    # Compute runtime
    set_pvc_agama_env
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
    if [[ "${node_label}" = "ccl_test_pvc" ]]
    then
        set_pvc_environment
    fi

    if [[ ${IS_GPU_NODE} = "yes" ]]
    then
        BUILD_COMPILER_TYPE="dpcpp"
    elif [[ ${IS_GPU_NODE} = "no" ]] || [[ ${build_compiler} = "gnu" ]]
    then
        BUILD_COMPILER_TYPE="gnu"
    elif [[ ${build_compiler} = "intel" ]]
    then
        BUILD_COMPILER_TYPE="intel"
    elif [[ ${build_compiler} = "icx" ]]
    then
        BUILD_COMPILER_TYPE="icx"
    else
        BUILD_COMPILER_TYPE="dpcpp"
    fi

    set_compute_runtime_env
    if [[ "${BUILD_COMPILER_TYPE}" = "gnu" ]]
    then
        set_gnu_compiler
    elif [[ "${BUILD_COMPILER_TYPE}" = "intel" ]]
    then
        set_intel_compiler
    elif [[ "${BUILD_COMPILER_TYPE}" = "icx" ]]
    then
        set_icx_compiler
    elif [[ "${BUILD_COMPILER_TYPE}" = "dpcpp" ]]
    then
        set_dpcpp_compiler
    else
        echo "ERROR: unsupported BUILD_COMPILER_TYPE"
        exit 1
    fi

    clinfo -a | grep -i "driver version"

    set_impi_environment
    set_ccl_environment

    if [[ -z "${worker_count}" ]]
    then
        worker_count="2"
    fi
    export CCL_WORKER_COUNT=${worker_count}

    if [[ -z "${runtime}" ]]
    then
        runtime="ofi"
    fi

    if [[ -z "${build_type}" ]]
    then
        export build_type="release"
    fi

    export CCL_PROCESS_CLEANUP="yes"
    set_external_env

    if [[ ${node_label} = "ccl_test_pvc" ]]
    then
        check_cxi
        check_anr
    fi

    # HVD env
    if [[ -z "${fw}" ]]
    then
        export fw="pt"
    fi
    if [[ -z "${enable_mpich}" ]]
    then
        export enable_mpich="0"
    fi
}

function set_reg_tests_environment()
{
    if [[ "${node_label}" = "ccl_test_gen9" ]]
    then
        export PLATFORM="gen"
    elif [[ "${node_label}" = "ccl_test_ats" ]]
    then
        export PLATFORM="ats"
    elif [[ "${node_label}" = "ccl_test_pvc" ]]
    then
        export PLATFORM="pvc"
    fi
}

function set_impi_environment()
{
    if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
    then
        return
    fi

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

function set_ccl_environment()
{
    if [[ -z ${CCL_ROOT} ]]
    then
        if [[ ${IS_GPU_NODE} = "yes" ]]
        then
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
            export DASHBOARD_GPU_DEVICE_PRESENT="yes"
        else
            source ${ARTEFACT_DIR}/l_ccl_${build_type}*/env/vars.sh --ccl-configuration=cpu
        fi
    fi
}

function make_tests()
{
    cd ${CURRENT_WORK_DIR}/tests/functional
    mkdir -p build
    cd ./build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="${C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
        -DPROC_MAPS="`echo ${PROC_MAPS} | tr '/' ';'`"
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
    if [[ ${IS_GPU_NODE} = "yes" ]]
    then
        if [[ $(hostname) = *"nnlmpinuc09"* ]]
        then
            export FI_TCP_IFACE=br
        else
            export FI_TCP_IFACE=eno1
        fi
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

    if [[ ${IS_GPU_NODE} = "yes" ]]
    then
        ${CURRENT_WORK_DIR}/tests/reg_tests/run.sh --mode gpu --platform ${PLATFORM} --transport ${REG_TESTS_TRANSPORT}
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

    set_regular_tests_environment

    for proc_map in `echo ${PROC_MAPS} | tr '/' ' '`
    do
        if [[ ${IS_GPU_NODE} = "yes" ]]
        then
            if [[ $(hostname) = *"nnlmpinuc09"* ]]
            then
                export FI_TCP_IFACE=br
            else
                export FI_TCP_IFACE=eno1
            fi
            ${CURRENT_WORK_DIR}/examples/run.sh --mode gpu --scope ${regular_scope} --cleanup --proc-map ${proc_map}
        else
            ${CURRENT_WORK_DIR}/examples/run.sh --mode cpu --scope ${regular_scope} --cleanup --proc-map ${proc_map}
        fi
    done

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

function run_hvd_rn50_test()
{
    local fw="${1}"
    local enable_mpich="${2}"

    if [[ ! -z ${PR_NUMBER} ]]
    then
        pr_number_option="-ccl_pr ${PR_NUMBER}"
    fi

    pushd ${CURRENT_WORK_DIR}/scripts/framework/horovod/

    HVD_WORK_DIR="${CURRENT_WORK_DIR}/scripts/framework/horovod/work_dir_${fw}_mpich_${enable_mpich}"

    # jfcst-xe compute nodes doesn't have access to the network
    # The download/install sections are performed in advance on the head node.
    if [[ ${FW_DOWNLOAD_COMPONENTS} = "yes" ]]
    then
        ./horovod.sh "-prepare_${fw}" 1 -work_dir "${HVD_WORK_DIR}" ${pr_number_option} -with_mpich ${enable_mpich}\
                     -token "${CURRENT_WORK_DIR}/gitpass.sh" -username ${USERNAME_1S}
        check_command_exit_code $? "ERROR: prepare Horovod/${fw} failed"
    else
        ./horovod.sh -install_hvd 1 -install_ccl 1 "-install_${fw}" 1 "-run_model_${fw}" 1 \
                     "-check_hvd_${fw}" 1 "-check_${fw}" 1 -remove_conda 1 -with_mpich ${enable_mpich} \
                     -transport mpi -provider ${FI_PROVIDER} -work_dir "${HVD_WORK_DIR}" \
                     -proc_maps ${PROC_MAPS}
        check_command_exit_code $? "ERROR: Horovod/${fw} run failed"
    fi
    popd
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

    run_hvd_rn50_test "${fw}" "${enable_mpich}"

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

function run_torch_ccl_tests() 
{

    if [[ ${ENABLE_TORCH_CCL_TESTS} = "no" ]]
    then
        return
    fi

    if [[ ! -z ${PR_NUMBER} ]]
    then
        pr_number_option="-ccl_pr ${PR_NUMBER}"
    fi

    source ${CCL_ONEAPI_DIR}/onemkl/last/mkl/latest/env/vars.sh
    source ${CCL_ONEAPI_DIR}/tbb_oneapi/last/tbb/latest/env/vars.sh

    TORCH_CCL_WORK_DIR="${CURRENT_WORK_DIR}/scripts/framework/pytorch/torch_ccl/work_dir_torch_ccl"
    pushd ${CURRENT_WORK_DIR}/scripts/framework/pytorch/torch_ccl/
    if [[ ${FW_DOWNLOAD_COMPONENTS} = "yes" ]]
    then
        ./torch_ccl.sh -prepare 1 ${pr_number_option} -work_dir ${TORCH_CCL_WORK_DIR} \
                       -token "${CURRENT_WORK_DIR}/gitpass.sh" -username ${USERNAME_1S}
    else
        ./torch_ccl.sh -install_torch_ccl 1 -run_ut 1 -check_pt 1 -check_ipex 1 -proc_maps ${PROC_MAPS} \
                       -remove_conda 1 -work_dir ${TORCH_CCL_WORK_DIR}
    fi
    log_status_fail=${PIPESTATUS[0]}
    popd
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "Torch-ccl testing ... OK"
    else
        echo "Torch-ccl testing ... NOK"
        exit 1
    fi

    exit 0
}

function set_modulefile_environment()
{
    MODULES_DIR="/p/pdsd/scratch/Software/modules/latest"
    source ${MODULES_DIR}/init/profile.sh
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

    ${CURRENT_WORK_DIR}/examples/run.sh --mode cpu --scope ${regular_scope}
    log_status_fail=${PIPESTATUS[0]}

    unset_modulefile_environment
    if [ "$log_status_fail" -eq 0 ]
    then
        echo "module file testing ... OK"
        exit 0
    else
        echo "module file testing ... NOK"
        exit 1
    fi
}

function run_functional_tests()
{
    if [[ ${ENABLE_FUNCTIONAL_TESTS} = "no" ]]
    then
        return
    fi

    func_exec_env=""
    make_tests
    set_functional_tests_env
    set_functional_tests_scope

    case "$runtime" in
           ofi )
                func_exec_env+=" CCL_ATL_TRANSPORT=ofi"
                run_test_cmd "${func_exec_env} ctest -VV -C default"
                run_test_cmd "${func_exec_env} ctest -VV -C regression"
                ;;
           mpi )
                func_exec_env+=" CCL_ATL_TRANSPORT=mpi"
                run_test_cmd "${func_exec_env} ctest -VV -C default"
                run_test_cmd "${func_exec_env} ctest -VV -C regression"
                ;;
           ofi_adjust | mpi_adjust )

                allgatherv_algos="naive flat ring"
                allreduce_algos="rabenseifner nreduce ring double_tree recursive_doubling 2d"
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

                    func_exec_env+=" CCL_ATL_TRANSPORT=mpi"
                fi

                if [ ${runtime} == "ofi_adjust" ]
                then
                    allgatherv_algos="${allgatherv_algos} multi_bcast"
                    # allreduce_algos="${allreduce_algos} ring_rma"

                    func_exec_env+=" CCL_ATL_TRANSPORT=ofi"
                fi

                if [[ ${IS_GPU_NODE} = "yes" ]]
                then
                    allreduce_algos="${allreduce_algos} topo"
                    alltoall_algos="${alltoall_algos} topo"
                    alltoallv_algos="${alltoallv_algos} topo"
                    allgatherv_algos="${allgatherv_algos} topo"
                    bcast_algos="${bcast_algos} topo"
                    reduce_algos="${reduce_algos} topo"
                fi

                for proc_map in `echo ${PROC_MAPS} | tr '/' ' '`
                do
                    n=`echo ${proc_map%:*}`
                    ppns=`echo ${proc_map#*:} | tr ',' ' '`

                    for ppn in $ppns
                    do
                        if [[ "$ppn" -gt "$n" ]]
                        then
                            continue
                        fi

                        for algo in ${allgatherv_algos}
                        do
                            allgatherv_exec_env=$(set_tests_option "CCL_ALLGATHERV=${algo}" "${func_exec_env}")
                            allgatherv_exec_env=$(set_tests_option "CCL_TEST_DYNAMIC_POINTER=0" "${allgatherv_exec_env}")
                            run_test_cmd "${allgatherv_exec_env} ctest -VV -C allgatherv_${algo}_${n}_${ppn}"
                        done

                        for algo in ${allreduce_algos}
                        do
                            # if [[ "$algo" == "ring_rma" ]]
                            # then
                            #     allreduce_ring_rma_exec_env="${func_exec_env} CCL_ATL_RMA=1 CCL_ALLREDUCE=${algo}"
                            #     run_test_cmd "${allreduce_ring_rma_exec_env} ctest -VV -C allreduce_${algo}"
                            # fi
                            allreduce_exec_env=$(set_tests_option "CCL_ALLREDUCE=${algo}" "${func_exec_env}")
                            run_test_cmd "${allreduce_exec_env} ctest -VV -C allreduce_${algo}_${n}_${ppn}"
                        done

                        for algo in ${alltoall_algos}
                        do
                            alltoall_exec_env=$(set_tests_option "CCL_ALLTOALL=${algo}" "${func_exec_env}")
                            run_test_cmd "${alltoall_exec_env} ctest -VV -C alltoall_${algo}_${n}_${ppn}"
                        done

                        for algo in ${alltoallv_algos}
                        do
                            alltoallv_exec_env=$(set_tests_option "CCL_ALLTOALLV=${algo}" "${func_exec_env}")
                            run_test_cmd "${alltoallv_exec_env} ctest -VV -C alltoallv_${algo}_${n}_${ppn}"
                        done

                        for algo in ${bcast_algos}
                        do
                            bcast_exec_env=$(set_tests_option "CCL_BCAST=${algo}" "${func_exec_env}")
                            run_test_cmd "${bcast_exec_env} ctest -VV -C bcast_${algo}_${n}_${ppn}"
                        done

                        for algo in ${reduce_algos}
                        do
                            reduce_exec_env=$(set_tests_option "CCL_REDUCE=${algo}" "${func_exec_env}")
                            run_test_cmd "${reduce_exec_env} ctest -VV -C reduce_${algo}_${n}_${ppn}"
                        done

                        for algo in ${reduce_scatter_algos}
                        do
                            reduce_scatter_exec_env=$(set_tests_option "CCL_REDUCE_SCATTER=${algo}" "${func_exec_env}")
                            run_test_cmd "${reduce_scatter_exec_env} ctest -VV -C reduce_scatter_${algo}_${n}_${ppn}"
                        done
                    done
                done
               ;;
            priority_mode )
                # TODO: At the moment priority_mode and unordered_coll_mode launches only
                # with CCL_ATS_TRANSPORT=ofi, so these confs are missed in mpich_ofi testing.
                # We would like to add them in the future.
                func_exec_env+=" CCL_ATL_TRANSPORT=ofi"
                func_exec_env+=" CCL_PRIORITY=lifo"
                run_test_cmd "${func_exec_env} ctest -VV -C default"
                func_exec_env=$(set_tests_option "CCL_PRIORITY=direct" "${func_exec_env}")
                run_test_cmd "${func_exec_env} ctest -VV -C default"
                ;;
            dynamic_pointer_mode )
                for transport in ${CCL_ATL_TRANSPORT_LIST}
                do
                    func_exec_env=$(set_tests_option "CCL_ATL_TRANSPORT=${transport}" "${func_exec_env}")
                    run_test_cmd "${func_exec_env} ctest -VV -C default"
                done
                ;;
            fusion_mode )
                func_exec_env+=" CCL_FUSION=1"
                for transport in ${CCL_ATL_TRANSPORT_LIST}
                do
                    func_exec_env=$(set_tests_option "CCL_ATL_TRANSPORT=${transport}" "${func_exec_env}")
                    run_test_cmd "${func_exec_env} ctest -VV -C allreduce_fusion"
                done
                ;;
           * )
                echo "Please specify runtime mode: runtime=ofi|mpi|ofi_adjust|mpi_adjust|priority_mode|dynamic_pointer_mode|fusion_mode|"
                exit 1
               ;;
    esac
}

#==============================================================================
#                              MAIN
#==============================================================================

set_default_values
set_environment
parse_arguments "$@"
set_provider_env
set_transport_env
set_runtime_env

set_mpich_ofi_env
run_horovod_tests
run_pytorch_tests
run_torch_ccl_tests
run_modulefile_tests
run_functional_tests
run_valgrind_check
run_reg_tests
run_regular_tests
