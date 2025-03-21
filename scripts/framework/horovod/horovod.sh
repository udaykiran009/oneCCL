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

SCRIPTS_ROOT_DIR=$(realpath ${SCRIPT_DIR}/../../)
source ${SCRIPTS_ROOT_DIR}/utils/common.sh
source ${SCRIPTS_ROOT_DIR}/utils/ats_helper.sh

set_run_env() {
    # model
    export PYTHONPATH=${RN50_MODEL_TF_DIR}:${PYTHONPATH}

    # GPU SW
    export EnableDirectSubmission=1
    export DisableIndirectAccess=1

    # Tensorflow
    export TF_ENABLE_LAYOUT_OPT=0
    export ITEX_ENABLE_TILE_AS_DEVICE=1
    export ITEX_ENABLE_ONEDNN_LAYOUT_OPT=1

    # PyTorch
    export IPEX_TILE_AS_DEVICE=1
    # export IPEX_LAZY_REORDER=1 # this env var was deprecated, following one should be used
    export IPEX_ONEDNN_LAYOUT=1

    # HVD
    export HOROVOD_LOG_LEVEL=INFO
    export HOROVOD_THREAD_AFFINITY=0,1,2,3
    export HOROVOD_CCL_FIN_THREADS=1
    export HOROVOD_CCL_ADD_EXTRA_WAIT=1 # WA for crashes: issue TBD. CMPLRLLVM-32016 (hangs) related?
    #export HOROVOD_FUSION_THRESHOLD=150000000
    #export HOROVOD_DISABLE_GROUP_FUSION=1
    #export HOROVOD_CYCLE_TIME=0.1

    # CCL
    export CCL_LOG_LEVEL=INFO
    export CCL_WORKER_COUNT=1
    export CCL_WORKER_AFFINITY=8-11
    export CCL_ATL_TRANSPORT="${TRANSPORT}"
    export CCL_SYCL_OUTPUT_EVENT=1
    vars_file="${CCL_SRC_DIR}/build/_install/env/setvars.sh"
    if [[ -f ${vars_file} ]]
    then
        source ${vars_file}
        unset CCL_CONFIGURATION
    fi

    # IMPI
    export I_MPI_DEBUG=12
    export I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=${HOROVOD_THREAD_AFFINITY}
    export I_MPI_PIN_PROCESSOR_LIST=4,5,6,7
    BOOTSTRAP_OPTIONS="-launcher ssh -launcher-exec /usr/bin/ssh"

    # OFI
    export FI_PROVIDER="${PROVIDER}"
    # export FI_LOG_LEVEL=debug

    # SYCL
    export SYCL_PI_LEVEL_ZERO_BATCH_SIZE=1 # WA for NaN failure on A0/A1 stepping: CMPLRLLVM-35731
    export SYCL_DEVICE_FILTER=level_zero
    # export SYCL_PI_LEVEL_ZERO_TRACK_INDIRECT_ACCESS_MEMORY=1 # not needed since CCL commit 5141eaf70630b364c708cae9eb664511db51dc05

    # conda
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CONDA_PREFIX}/lib"
}

CONDA_LINK="https://repo.anaconda.com/miniconda/Miniconda3-py39_4.11.0-Linux-x86_64.sh"
CONDA_INSTALL_DIR=""

CONDA_PACKAGES="python=3.9"
CONDA_FILENAME="conda.sh"

CCL_BASE_LINK="github.com/intel-innersource/libraries.performance.communication.oneccl"
HOROVOD_BASE_LINK="github.com/intel-innersource/frameworks.ai.horovod.git"
MODEL_TF_BASE_LINK="github.com/intel-innersource/frameworks.ai.models.intel-models"
MODEL_TF_SRC_BRANCH="yang/resnet50-training-tmp"
MODEL_TF_SRC_COMMIT="20a8b0075cd75a5534007df5ba8cfc112a5a41c3"

MODEL_PT_FILE="bench_pt.py"
MODEL_PT_TOKEN="token=GHSAT0AAAAAABORQUKBMBDPQUJWIRRNYZOQYSKX2IQ"
MODEL_PT_BASE_LINK="https://raw.githubusercontent.com/intel-sandbox/dlutils/master"

if [ -z "$TF_LINK" ]
then
    TF_LINK="http://mlpc.intel.com/downloads/gpu-new/validation/ITEX/weekly/PVC/ww08/suse/for-hvd/tensorflow-2.8.0-cp39-cp39-linux_x86_64.whl"
fi

if [ -z "$ITEX_LINK" ]
then
    ITEX_LINK="http://mlpc.intel.com/downloads/gpu-new/validation/ITEX/weekly/PVC/ww08/ubuntu/intel_extension_for_tensorflow-0.3.0-cp39-cp39-linux_x86_64.whl"
fi

TF_NAME=`basename $TF_LINK`
ITEX_NAME=`basename $ITEX_LINK`

if [ -z "$PT_LINK" ]
then
    PT_LINK="http://mlpc.intel.com/downloads/gpu-new/validation/IPEX/weekly/PVC/ww11/py39/torch-1.10.0a0+gitf335324-cp39-cp39-linux_x86_64.whl"
fi

if [ -z "$IPEX_LINK" ]
then
    IPEX_LINK="http://mlpc.intel.com/downloads/gpu-new/validation/IPEX/weekly/PVC/ww11/py39/intel_extension_for_pytorch-1.10.100+769c22be-cp39-cp39-linux_x86_64.whl"
fi

PT_NAME=`basename $PT_LINK`
IPEX_NAME=`basename $IPEX_LINK`

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"

DEFAULT_DOWNLOAD_CCL="0"
DEFAULT_INSTALL_CCL="0"

DEFAULT_TRANSPORT="mpi"
DEFAULT_PROVIDER="tcp"

DEFAULT_WITH_MPICH="0"

DEFAULT_DOWNLOAD_CONDA="0"
DEFAULT_CREATE_CONDA="0"
DEFAULT_REMOVE_CONDA="0"
DEFAULT_CONDA_ENV_NAME="test_horovod"

DEFAULT_PREPARE_TF="0"
DEFAULT_CHECK_TF="0"
DEFAULT_FULL_TF="0"
DEFAULT_DOWNLOAD_TF="0"
DEFAULT_INSTALL_TF="0"
DEFAULT_TF_PATH=""
DEFAULT_DOWNLOAD_ITEX="0"
DEFAULT_INSTALL_ITEX="0"
DEFAULT_ITEX_PATH=""

DEFAULT_PREPARE_PT="0"
DEFAULT_CHECK_PT="0"
DEFAULT_FULL_PT="0"
DEFAULT_DOWNLOAD_PT="0"
DEFAULT_INSTALL_PT="0"
DEFAULT_PT_PATH=""
DEFAULT_DOWNLOAD_IPEX="0"
DEFAULT_INSTALL_IPEX="0"
DEFAULT_IPEX_PATH=""

DEFAULT_DOWNLOAD_HVD="0"
DEFAULT_INSTALL_HVD="0"
DEFAULT_CHECK_HVD_TF="0"
DEFAULT_CHECK_HVD_PT="0"
DEFAULT_HVD_BRANCH="xpu"

DEFAULT_DOWNLOAD_MODEL_TF="0"
DEFAULT_RUN_MODEL_TF="0"
DEFAULT_DOWNLOAD_MODEL_PT="0"
DEFAULT_RUN_MODEL_PT="0"
DEFAULT_BATCH_SIZE="16"
DEFAULT_ITER_COUNT="40"

DEFAULT_PROC_MAPS="2:2"
DEFAULT_PATH_TO_TOKEN_FILE_1S=""
DEFAULT_USERNAME_1S=""
DEFAULT_PROXY="http://proxy-us.intel.com:912"
DEFAULT_EXTRA_PROXY=0

CheckCommandExitCode() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}" 1>&2
        deactivate_conda
        remove_conda
        echo_log "TEST FAILED"
        exit $1
    fi
}

print_help() {
    echo_log ""
    echo_log "Requirements:"
    echo_log "- source DPCPP compiler before script launch"
    echo_log ""
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -download_conda <bool_flag>"
    echo_log "      Download miniconda"
    echo_log "  -create_conda <bool_flag>"
    echo_log "      Create conda environment"
    echo_log "  -remove_conda <bool_flag>"
    echo_log "      Remove conda environment after script completion"
    echo_log "  -env <name>"
    echo_log "      The name for conda environment"
    echo_log "  -download_ccl <bool_flag>"
    echo_log "      Download oneCCL"
    echo_log "  -install_ccl <bool_flag>"
    echo_log "      Install oneCCL"
    echo_log "  -download_tf <bool_flag>"
    echo_log "      Download TensorFlow"
    echo_log "  -prepare_tf <bool_flag>"
    echo_log "      Prepare workspace for testing with TF"
    echo_log "  -check_tf <bool_flag>"
    echo_log "      Run basic TensorFlow test"
    echo_log "  -full_tf <bool_flag>"
    echo_log "      Enable all TF processing"
    echo_log "  -install_tf <bool_flag>"
    echo_log "      Install TensorFlow; also affects HVD build"
    echo_log "  -tf_path <path>"
    echo_log "      Install TensorFlow from path (*.whl)"
    echo_log "  -download_itex <bool_flag>"
    echo_log "      Download ITEX"
    echo_log "  -install_itex <bool_flag>"
    echo_log "      Install ITEX"
    echo_log "  -itex_path <path>"
    echo_log "      Install ITEX from path (*.whl)"
    echo_log "  -prepare_pt <bool_flag>"
    echo_log "      Prepare workspace for testing with PT"
    echo_log "  -check_pt <bool_flag>"
    echo_log "      Run basic PyTorch test"
    echo_log "  -full_pt <bool_flag>"
    echo_log "      Enable all PT processing"
    echo_log "  -download_pt <bool_flag>"
    echo_log "      Download PyTorch"
    echo_log "  -install_pt <bool_flag>"
    echo_log "      Install PyTorch; also affects HVD build"
    echo_log "  -pt_path <path>"
    echo_log "      Install PyTorch from path (*.whl)"
    echo_log "  -download_ipex <bool_flag>"
    echo_log "      Download IPEX"
    echo_log "  -install_ipex <bool_flag>"
    echo_log "      Install IPEX"
    echo_log "  -ipex_path <path>"
    echo_log "      Install IPEX from path (*.whl)"
    echo_log "  -download_hvd <bool_flag>"
    echo_log "      Download Horovod"
    echo_log "  -install_hvd <bool_flag>"
    echo_log "      Install Horovod; see also install_tf & install_pt"
    echo_log "  -check_hvd_tf <bool_flag>"
    echo_log "      Run basic Horovod test and benchmark with Tensorflow"
    echo_log "  -check_hvd_pt <bool_flag>"
    echo_log "      Run basic Horovod test and benchmark with PyTorch"
    echo_log "  -hvd_branch <branch_name>"
    echo_log "      Set Horovod branch name to download"
    echo_log "  -download_model_tf <bool_flag>"
    echo_log "      Download repository with models to run with Tensorflow"
    echo_log "  -run_model_tf <bool_flag>"
    echo_log "      Run few iterations of RN50 training on 2 ranks with Tensorflow"
    echo_log "  -download_model_pt <bool_flag>"
    echo_log "      Download model to run with PyTorch"
    echo_log "  -run_model_pt <bool_flag>"
    echo_log "      Run few iterations of RN50 training on 2 ranks with PyTorch"
    echo_log "  -batch_size <size>"
    echo_log "      Batch size for RN50 training"
    echo_log "  -iter_count <count>"
    echo_log "      Iteration count for RN50 training"
    echo_log "  -proc_maps <n:ppns>/..."
    echo_log "      Map with n and ppns values"
    echo_log "  -token <path>"
    echo_log "      Path to file with github credentials"
    echo_log "  -username <name>"
    echo_log "      Github username with access to Horovod repo"
    echo_log "  -proxy <url>"
    echo_log "      https proxy"
    echo_log "  -set_extra_proxy <bool_flag>"
    echo_log "      Set extra proxy"
    echo_log "  -ccl_pr <number>"
    echo_log "      Checkout specific PR from oneCCL repository"
    echo_log "  -transport <string>"
    echo_log "      Set CCL ATL transport (mpi/ofi)"
    echo_log "  -provider <string>"
    echo_log "      Set OFI provider"
    echo_log "  -with_mpich <bool_flag>"
    echo_log "      Run with mpich mpiexec"
    echo_log ""
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh "
    echo_log "  ${BASENAME}.sh -install_tf 1 -tf_path /nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Software/Tensorflow/latest"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}

    DOWNLOAD_CCL=${DEFAULT_DOWNLOAD_CCL}
    INSTALL_CCL=${DEFAULT_INSTALL_CCL}

    TRANSPORT=${DEFAULT_TRANSPORT}
    PROVIDER=${DEFAULT_PROVIDER}

    WITH_MPICH=${DEFAULT_WITH_MPICH}

    DOWNLOAD_CONDA=${DEFAULT_DOWNLOAD_CONDA}
    CREATE_CONDA=${DEFAULT_CREATE_CONDA}
    REMOVE_CONDA=${DEFAULT_REMOVE_CONDA}
    CONDA_ENV_NAME=${DEFAULT_CONDA_ENV_NAME}

    PREPARE_TF=${DEFAULT_PREPARE_TF}
    CHECK_TF=${DEFAULT_CHECK_TF}
    DOWNLOAD_TF=${DEFAULT_DOWNLOAD_TF}
    INSTALL_TF=${DEFAULT_INSTALL_TF}
    TF_PATH=${DEFAULT_TF_PATH}
    DOWNLOAD_ITEX=${DEFAULT_DOWNLOAD_ITEX}
    INSTALL_ITEX=${DEFAULT_INSTALL_ITEX}
    ITEX_PATH=${DEFAULT_ITEX_PATH}

    PREPARE_PT=${DEFAULT_PREPARE_PT}
    CHECK_PT=${DEFAULT_CHECK_PT}
    DOWNLOAD_PT=${DEFAULT_DOWNLOAD_PT}
    INSTALL_PT=${DEFAULT_INSTALL_PT}
    PT_PATH=${DEFAULT_PT_PATH}
    DOWNLOAD_IPEX=${DEFAULT_DOWNLOAD_IPEX}
    INSTALL_IPEX=${DEFAULT_INSTALL_IPEX}
    IPEX_PATH=${DEFAULT_IPEX_PATH}

    DOWNLOAD_HVD=${DEFAULT_DOWNLOAD_HVD}
    INSTALL_HVD=${DEFAULT_INSTALL_HVD}
    CHECK_HVD_TF=${DEFAULT_CHECK_HVD_TF}
    CHECK_HVD_PT=${DEFAULT_CHECK_HVD_PT}
    HVD_BRANCH=${DEFAULT_HVD_BRANCH}

    FULL_TF=${DEFAULT_FULL_TF}
    DOWNLOAD_MODEL=${DEFAULT_DOWNLOAD_MODEL}
    RUN_MODEL_TF=${DEFAULT_RUN_MODEL_TF}
    FULL_PT=${DEFAULT_FULL_PT}
    DOWNLOAD_MODEL_PT=${DEFAULT_DOWNLOAD_MODEL_PT}
    RUN_MODEL_PT=${DEFAULT_RUN_MODEL_PT}
    BATCH_SIZE=${DEFAULT_BATCH_SIZE}
    ITER_COUNT=${DEFAULT_ITER_COUNT}

    PROC_MAPS=${DEFAULT_PROC_MAPS}
    PATH_TO_TOKEN_FILE_1S=${DEFAULT_PATH_TO_TOKEN_FILE_1S}
    USERNAME_1S=${DEFAULT_USERNAME_1S}
    PROXY=${DEFAULT_PROXY}
    SET_EXTRA_PROXY=${DEFAULT_EXTRA_PROXY}

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
                FULL_SCOPE=${2}
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
            "-download_conda")
                DOWNLOAD_CONDA=${2}
                shift
                ;;
            "-create_conda")
                CREATE_CONDA=${2}
                shift
                ;;
            "-remove_conda")
                REMOVE_CONDA=${2}
                shift
                ;;
            "-env")
                CONDA_ENV_NAME=${2}
                shift
                ;;
            "-prepare_tf")
                PREPARE_TF=${2}
                shift
                ;;
            "-check_tf")
                CHECK_TF="${2}"
                shift
                ;;
            "-full_tf")
                FULL_TF=${2}
                shift
                ;;
            "-download_tf")
                DOWNLOAD_TF=${2}
                shift
                ;;
            "-install_tf")
                INSTALL_TF=${2}
                shift
                ;;
            "-tf_path")
                TF_PATH=${2}
                shift
                ;;
            "-download_itex")
                DOWNLOAD_ITEX=${2}
                shift
                ;;
            "-install_itex")
                INSTALL_ITEX=${2}
                shift
                ;;
            "-itex_path")
                ITEX_PATH=${2}
                shift
                ;;
            "-prepare_pt")
                PREPARE_PT=${2}
                shift
                ;;
            "-check_pt")
                CHECK_PT="${2}"
                shift
                ;;
            "-full_pt")
                FULL_PT=${2}
                shift
                ;;
            "-download_pt")
                DOWNLOAD_PT=${2}
                shift
                ;;
            "-install_pt")
                INSTALL_PT=${2}
                shift
                ;;
            "-pt_path")
                PT_PATH=${2}
                shift
                ;;
            "-download_ipex")
                DOWNLOAD_IPEX=${2}
                shift
                ;;
            "-install_ipex")
                INSTALL_IPEX=${2}
                shift
                ;;
            "-ipex_path")
                IPEX_PATH=${2}
                shift
                ;;
            "-download_hvd")
                DOWNLOAD_HVD=${2}
                shift
                ;;
            "-install_hvd")
                INSTALL_HVD=${2}
                shift
                ;;
            "-hvd_branch")
                HVD_BRANCH=${2}
                shift
                ;;
            "-check_hvd_tf")
                CHECK_HVD_TF="${2}"
                shift
                ;;
            "-check_hvd_pt")
                CHECK_HVD_PT="${2}"
                shift
                ;;
            "-download_model_tf")
                DOWNLOAD_MODEL_TF=${2}
                shift
                ;;
            "-run_model_tf")
                RUN_MODEL_TF=${2}
                shift
                ;;
            "-download_model_pt")
                DOWNLOAD_MODEL_PT=${2}
                shift
                ;;
            "-run_model_pt")
                RUN_MODEL_PT=${2}
                shift
                ;;
            "-batch_size")
                BATCH_SIZE="${2}"
                shift
                ;;
            "-iter_count")
                ITER_COUNT="${2}"
                shift
                ;;
            "-proc_maps")
                PROC_MAPS="${2}"
                shift
                ;;
            "-token")
                PATH_TO_TOKEN_FILE_1S=${2}
                shift
                ;;
            "-username")
                USERNAME_1S="${2}@"
                shift
                ;;
            "-proxy")
                PROXY="${2}"
                shift
                ;;
            "-set_extra_proxy")
                SET_EXTRA_PROXY="${2}"
                shift
                ;;
            "-ccl_pr")
                CCL_PR_NUMBER="${2}"
                shift
                ;;
            "-transport")
                TRANSPORT="${2}"
                shift
                ;;
            "-provider")
                PROVIDER="${2}"
                shift
                ;;
            "-with_mpich")
                WITH_MPICH="${2}"
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
    HVD_SRC_DIR=${SCRIPT_WORK_DIR}/horovod
    HOROVOD_DEPS_DIR=${SCRIPT_WORK_DIR}/horovod_deps
    MODEL_TF_SRC_DIR=${SCRIPT_WORK_DIR}/model
    RN50_MODEL_TF_DIR=${MODEL_TF_SRC_DIR}/models/image_recognition/tensorflow/resnet50v1_5/training
    RN50_OUTPUT_DIR=${MODEL_TF_SRC_DIR}/resnet50_chk
    MODEL_PT_SRC=${SCRIPT_WORK_DIR}/${MODEL_PT_FILE}
    CONDA_INSTALL_DIR="${SCRIPT_WORK_DIR}/conda"

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        FULL_PT="1"
        FULL_TF="1"
    fi

    if [[ ${FULL_PT} = "1" ]]
    then
        DOWNLOAD_PT="1"
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"

        DOWNLOAD_MODEL_PT="1"
        RUN_MODEL_PT="1"
        CHECK_HVD_PT="1"
        CHECK_PT="1"
    fi

    if [[ ${FULL_TF} = "1" ]]
    then
        DOWNLOAD_TF="1"
        INSTALL_TF="1"
        DOWNLOAD_ITEX="1"
        INSTALL_ITEX="1"

        DOWNLOAD_MODEL_TF="1"
        RUN_MODEL_TF="1"
        CHECK_HVD_TF="1"
        CHECK_TF="1"
    fi

    if [[ ${FULL_SCOPE} = "1" || ${FULL_PT} = "1" || ${FULL_TF} = "1" ]]
    then
        DOWNLOAD_CCL="1"
        INSTALL_CCL="1"

        DOWNLOAD_CONDA="1"
        CREATE_CONDA="1"

        DOWNLOAD_HVD="1"
        INSTALL_HVD="1"

        remove_conda
        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    if [[ ${DOWNLOAD_CONDA} = "1" ]]
    then
        CREATE_CONDA="1"
    fi

    if [[ ${DOWNLOAD_CCL} = "1" ]]
    then
        INSTALL_CCL="1"
    fi

    if [[ ${DOWNLOAD_TF} = "1" ]]
    then
        INSTALL_TF="1"
        DOWNLOAD_ITEX="1"
        INSTALL_ITEX="1"
        CHECK_TF="1"
    fi

    if [[ ${DOWNLOAD_PT} = "1" ]]
    then
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"
        CHECK_PT="1"
    fi

    if [[ ${DOWNLOAD_HVD} = "1" ]]
    then
        INSTALL_HVD="1"
    fi

    if [[ ${PREPARE_PT} = "1" || ${PREPARE_TF} = "1" ]]
    then
        DOWNLOAD_CCL="1"
        DOWNLOAD_HVD="1"
        DOWNLOAD_CONDA="1"
        CREATE_CONDA="1"
    fi

    if [[ ${PREPARE_PT} = "1" ]]
    then
        DOWNLOAD_PT="1"
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"
        DOWNLOAD_MODEL_PT="1"
    fi

    if [[ ${PREPARE_TF} = "1" ]]
    then
        DOWNLOAD_TF="1"
        INSTALL_TF="1"
        DOWNLOAD_ITEX="1"
        INSTALL_ITEX="1"
        DOWNLOAD_MODEL_TF="1"
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR    = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE         = ${FULL_SCOPE}"

    echo_log "DOWNLOAD_CCL       = ${DOWNLOAD_CCL}"
    echo_log "INSTALL_CCL        = ${INSTALL_CCL}"

    echo_log "TRANSPORT          = ${TRANSPORT}"
    echo_log "PROVIDER           = ${PROVIDER}"

    echo_log "DOWNLOAD_CONDA     = ${DOWNLOAD_CONDA}"
    echo_log "CREATE_CONDA       = ${CREATE_CONDA}"
    echo_log "REMOVE_CONDA       = ${REMOVE_CONDA}"
    echo_log "CONDA_ENV_NAME     = ${CONDA_ENV_NAME}"

    echo_log "PREPARE_TF         = ${PREPARE_TF}"
    echo_log "CHECK_TF           = ${CHECK_TF}"
    echo_log "FULL_TF            = ${FULL_TF}"
    echo_log "DOWNLOAD_TF        = ${DOWNLOAD_TF}"
    echo_log "INSTALL_TF         = ${INSTALL_TF}"
    echo_log "TF_PATH            = ${TF_PATH}"
    echo_log "DOWNLOAD_ITEX      = ${DOWNLOAD_ITEX}"
    echo_log "INSTALL_ITEX       = ${INSTALL_ITEX}"
    echo_log "ITEX_PATH          = ${ITEX_PATH}"

    echo_log "PREPARE_PT         = ${PREPARE_PT}"
    echo_log "CHECK_PT           = ${CHECK_PT}"
    echo_log "FULL_PT            = ${FULL_PT}"
    echo_log "DOWNLOAD_PT        = ${DOWNLOAD_PT}"
    echo_log "INSTALL_PT         = ${INSTALL_PT}"
    echo_log "PT_PATH            = ${PT_PATH}"
    echo_log "DOWNLOAD_IPEX      = ${DOWNLOAD_IPEX}"
    echo_log "INSTALL_IPEX       = ${INSTALL_IPEX}"
    echo_log "IPEX_PATH          = ${IPEX_PATH}"

    echo_log "DOWNLOAD_HVD       = ${DOWNLOAD_HVD}"
    echo_log "INSTALL_HVD        = ${INSTALL_HVD}"
    echo_log "CHECK_HVD_TF       = ${CHECK_HVD_TF}"
    echo_log "CHECK_HVD_PT       = ${CHECK_HVD_PT}"
    echo_log "HVD_BRANCH         = ${HVD_BRANCH}"

    echo_log "DOWNLOAD_MODEL_TF  = ${DOWNLOAD_MODEL_TF}"
    echo_log "RUN_MODEL_TF       = ${RUN_MODEL_TF}"
    echo_log "DOWNLOAD_MODEL_PT  = ${DOWNLOAD_MODEL_PT}"
    echo_log "RUN_MODEL_PT       = ${RUN_MODEL_PT}"
    echo_log "BATCH_SIZE         = ${BATCH_SIZE}"
    echo_log "ITER_COUNT         = ${ITER_COUNT}"

    echo_log "PROC_MAPS          = ${PROC_MAPS}"
    echo_log "USERNAME_1S        = ${USERNAME_1S}"
    echo_log "PROXY              = ${PROXY}"
    echo_log "SET_EXTRA_PROXY    = ${SET_EXTRA_PROXY}"
}

hvd_test() {
    if [[ ${CHECK_HVD_TF} = "1" ]]
    then
        # WA for HVD+TF 4-ranks hang: issue TBD
        PROC_MAPS=${DEFAULT_PROC_MAPS}

        echo_log "============================= Basic Horovod/TF test =================================="
        cmd="python -c \"import horovod.tensorflow as hvd; hvd.init(); print(hvd.local_rank())\""
        echo_log ${cmd}
        eval ${cmd}
        CheckCommandExitCode $? "Basic Horovod/TF test failed"
        echo_log "============================= ****************** =================================="

        if [[ ${CCL_CONFIGURATION} = "" ]]
        then
            echo_log "============================= Horovod/TF allreduce bench =================================="
            mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${HVD_SRC_DIR}/benchmark/hvd_allreduce_tf.py" \
                app_args="--iters 10" \
                proc_map_iterator
            CheckCommandExitCode $? "HVD allreduce test failed"
            echo_log "============================= ****************** =================================="
            echo_log "============================= Horovod/TF bench =================================="\
            mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${HVD_SRC_DIR}/benchmark/hvd_bench.py" \
                app_args="--xpu gpu --framework tf" \
                proc_map_iterator
            CheckCommandExitCode $? "Basic Horovod/TF test failed"
            echo_log "============================= ****************** =================================="
        fi
    fi

    if [[ ${CHECK_HVD_PT} = "1" ]]
    then
        echo_log "============================= Basic Horovod/PT test =================================="
        cmd="python -c \"import horovod.torch as hvd; hvd.init(); print(hvd.local_rank())\""
        echo_log ${cmd}
        eval ${cmd}
        CheckCommandExitCode $? "Basic Horovod/PT test failed"
        echo_log "============================= ****************** =================================="

        if [[ ${CCL_CONFIGURATION} = "" ]]
        then
            echo_log "============================= Horovod/PT broadcast bench =================================="
            mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${HVD_SRC_DIR}/benchmark/hvd_allreduce_pt.py" \
                app_args="--iter 10 --no-cuda --broadcast" \
                proc_map_iterator
            CheckCommandExitCode $? "HVD broadcast test failed"
            echo_log "============================= ****************** =================================="
            echo_log "============================= Horovod/PT allreduce bench =================================="
            mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${HVD_SRC_DIR}/benchmark/hvd_allreduce_pt.py" \
                app_args="--iter 10 --no-cuda" \
                proc_map_iterator
            CheckCommandExitCode $? "HVD allreduce test failed"
            echo_log "============================= ****************** =================================="
            echo_log "============================= Horovod/PT bench =================================="
            mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${HVD_SRC_DIR}/benchmark/hvd_bench.py" \
                app_args="--xpu gpu --framework pt" \
                proc_map_iterator
            CheckCommandExitCode $? "Basic Horovod/PT test failed"
            echo_log "============================= ****************** =================================="
        fi
    fi
}

tf_test() {
    echo_log "===================================== Basic TF test ==========================================="
    echo_log "python -c \"import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())\""
    python -c "import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())"
    CheckCommandExitCode $? "Basic TF test failed"
    echo_log "===================================== ************* ==========================================="
}

pt_test() {
    echo_log "===================================== Basic PT test ==========================================="
    echo_log "python -c \"import torch; import intel_extension_for_pytorch; print(torch.__version__)\""
    python -c "import torch; import intel_extension_for_pytorch; print(torch.__version__)"
    CheckCommandExitCode $? "Basic PT test failed"
    echo_log "===================================== ************* ==========================================="
}

check_base_env() {
    echo_log "which DPCPP:"
    which dpcpp
    CheckCommandExitCode $? "DPCPP was not found"

    echo_log "DPCPP version:"
    echo_log `dpcpp -v`
}

check_horovod_env() {
    echo_log "which mpicxx:"
    which mpicxx
    CheckCommandExitCode $? "MPICXX was not found"
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
    clone_repo "CCL" "https://${USERNAME_1S}${CCL_BASE_LINK}" "master" \
        ${CCL_SRC_DIR} "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"

    if [[ ! -z "${CCL_PR_NUMBER}" ]]; then
        cd ${CCL_SRC_DIR}
        GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git fetch origin pull/${CCL_PR_NUMBER}/head:pull_${CCL_PR_NUMBER}
        git checkout pull_${CCL_PR_NUMBER}
        git log -1
    fi
}

install_ccl() {
    if [[ ${INSTALL_CCL} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-download_ccl 1\""
        CheckCommandExitCode 1 "Install CCL failed"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build
    cmake .. -DBUILD_FT=0 -DBUILD_EXAMPLES=0 \
        -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp
    CheckCommandExitCode $? "Configure CCL failed"

    make -j install
    CheckCommandExitCode $? "Install CCL failed"

    vars_file="${CCL_SRC_DIR}/build/_install/env/setvars.sh"
    source ${vars_file}
}

download_fw() {

    cd ${SCRIPT_WORK_DIR}

    if [[ ${DOWNLOAD_TF} = "1" ]]
    then
        echo_log "\n=== download TF ===\n"
        wget -nv -e use_proxy=no ${TF_LINK}
    fi

    if [[ ${DOWNLOAD_ITEX} = "1" ]]
    then
        echo_log "\n=== download ITEX ===\n"
        wget -nv -e use_proxy=no ${ITEX_LINK}
    fi

    if [[ ${DOWNLOAD_PT} = "1" ]]
    then
        echo_log "\n=== download PT ===\n"
        wget -nv -e use_proxy=no ${PT_LINK}
    fi

    if [[ ${DOWNLOAD_IPEX} = "1" ]]
    then
        echo_log "\n=== download IPEX ===\n"
        wget -nv -e use_proxy=no ${IPEX_LINK}
    fi
}

install_fw() {

    cd ${SCRIPT_WORK_DIR}

    if [[ ${INSTALL_TF} = "1" ]]
    then
        echo_log "\n=== install TF ===\n"
        if [[ -f ${TF_PATH} ]]
        then
            install_name=$(realpath ${TF_PATH})
        else
            install_name=$(realpath ${TF_NAME})
        fi
        pip install ${install_name}
        CheckCommandExitCode $? "TF install failed"
    fi

    if [[ ${INSTALL_ITEX} = "1" ]]
    then
        echo_log "\n=== install ITEX ===\n"
        if [[ -f ${ITEX_PATH} ]]
        then
            install_name=$(realpath ${ITEX_PATH})
        else
            install_name=$(realpath ${ITEX_NAME})
        fi
        pip install ${install_name}
        CheckCommandExitCode $? "ITEX install failed"
    fi

    if [[ ${INSTALL_PT} = "1" ]]
    then
        echo_log "\n=== install pybind11 ===\n"
        pip install pybind11

        echo_log "\n=== install torchvision==0.8.2 ===\n"
        pip install torchvision==0.8.2 --no-deps

        echo_log "\n=== install pillow ===\n"
        pip install --no-cache-dir -I pillow

        echo_log "\n=== upgrade numpy ===\n"
        pip install numpy --upgrade

        echo_log "\n=== install mpi4py ===\n"
        pip install mpi4py --no-deps

        echo_log "\n=== install PT ===\n"
        if [[ -f ${PT_PATH} ]]
        then
            install_name=$(realpath ${PT_PATH})
        else
            install_name=$(realpath ${PT_NAME})
        fi
        pip install ${install_name}
        CheckCommandExitCode $? "PT install failed"
    fi

    if [[ ${INSTALL_IPEX} = "1" ]]
    then
        echo_log "\n=== install IPEX ===\n"
        if [[ -f ${IPEX_PATH} ]]
        then
            install_name=$(realpath ${IPEX_PATH})
        else
            install_name=$(realpath ${IPEX_NAME})
        fi
        pip install ${install_name}
        CheckCommandExitCode $? "IPEX install failed"
    fi
}

check_fw() {
    cd ${SCRIPT_WORK_DIR}

    if [[ ${CHECK_TF} = "1" ]]
    then
        tf_test
        CheckCommandExitCode $? "TF test failed"
    fi

    if [[ ${CHECK_PT} = "1" ]]
    then
        pt_test
        CheckCommandExitCode $? "PT test failed"
    fi
}

function set_mpich_ofi_env()
{
    if [[ ${WITH_MPICH} != "1" ]]
    then
        return
    fi

    set_ats_mpich_ofi_env
}

download_hvd() {
    if [[ ${DOWNLOAD_HVD} != "1" ]]
    then
        return
    fi

    if [[ -d ${HVD_SRC_DIR} ]]
    then
        rm -rf ${HVD_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}
    clone_repo "Horovod" "https://${USERNAME_1S}${HOROVOD_BASE_LINK}" "${HVD_BRANCH} --recursive" \
        ${HVD_SRC_DIR} "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"

    cd ${HVD_SRC_DIR}

    mkdir benchmark
    if [[ ${PREPARE_PT} == "1" ]]
    then
        wget -O benchmark/hvd_allreduce_pt.py \
            https://raw.githubusercontent.com/intel-sandbox/dlutils/master/hvd_allreduce_pt.py?token=GHSAT0AAAAAABORQUKA55P64PJMGP3TFKLWYRZ3AJA
    fi
    if [[ ${PREPARE_TF} == "1" ]]
    then
        # using private token as the repo is private
        https_proxy="" curl --header "PRIVATE-TOKEN: Ayxmiwc_C6TGd4H1Cso8" \
            "https://gitlab.devtools.intel.com/api/v4/projects/101266/repository/files/tests%2Fhvd_bench%2Fhvd_allreduce_tf.py/raw?ref=master" \
            > benchmark/hvd_allreduce_tf.py
    fi

    if [[ ${PREPARE_PT} == "1" || ${PREPARE_TF} == "1" ]]
    then
        pip download -d ${HOROVOD_DEPS_DIR} .
    fi
}

install_hvd() {
    if [[ ${INSTALL_HVD} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${HVD_SRC_DIR} ]]
    then
        echo "ERROR: HVD_SRC_DIR (${HVD_SRC_DIR}) is not directory, try to run script with \"-download_hvd 1\""
        CheckCommandExitCode 1 "Install Horovod failed"
    fi

    if [[ -d ${HOROVOD_DEPS_DIR} ]]
    then
        for package in $(ls ${HOROVOD_DEPS_DIR})
        do
            pip install --no-index --find-links=${HOROVOD_DEPS_DIR} ${HOROVOD_DEPS_DIR}/${package}
        done
    fi

    check_horovod_env

    cmd="I_MPI_CXX=dpcpp"
    cmd="$cmd MPICH_CXX=dpcpp"
    cmd="$cmd CXX=mpicxx"
    cmd="$cmd LDSHARED=\"dpcpp -shared -fPIC\""
    cmd="$cmd CC=icx"
    cmd="$cmd HOROVOD_GPU=DPCPP"
    cmd="$cmd HOROVOD_GPU_OPERATIONS=CCL"

    cd ${HVD_SRC_DIR}

    # rm -rf build

    cmd="$cmd HOROVOD_CPU_OPERATIONS=CCL"
    cmd="$cmd HOROVOD_WITH_MPI=1"
    cmd="$cmd HOROVOD_WITHOUT_MXNET=1"
    cmd="$cmd HOROVOD_WITHOUT_GLOO=1"

    if [[ ${INSTALL_TF} = "1" ]]
    then
        cmd="$cmd HOROVOD_WITH_TENSORFLOW=1"
    else
        cmd="$cmd HOROVOD_WITHOUT_TENSORFLOW=1"
    fi

    if [[ ${INSTALL_PT} = "1" ]]
    then
        cmd="$cmd HOROVOD_WITH_PYTORCH=1"
    else
        cmd="$cmd HOROVOD_WITHOUT_PYTORCH=1"
    fi

    cmd="$cmd python setup.py install"
    echo_log "\nhvd build cmd:\n${cmd}\n"
    eval ${cmd}
    CheckCommandExitCode $? "Install Horovod failed"
}

check_hvd() {
    if [[ ${CHECK_HVD_TF} != "1" && ${CHECK_HVD_PT} != "1" ]]
    then
        return
    fi
    cd ${SCRIPT_WORK_DIR}
    hvd_test
    CheckCommandExitCode $? "Horovod test failed"
}

download_model_tf() {
    if [[ ${DOWNLOAD_MODEL_TF} != "1" ]]
    then
        return
    fi

    if [[ -f ${MODEL_TF_SRC_DIR} ]]
    then
        rm -rf ${MODEL_TF_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}
    clone_repo "TF_MODEL" "https://${USERNAME_1S}${MODEL_TF_BASE_LINK}" ${MODEL_TF_SRC_BRANCH} \
        ${MODEL_TF_SRC_DIR} "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"
    cd ${MODEL_TF_SRC_DIR}
    git checkout ${MODEL_TF_SRC_COMMIT}
}

download_model_pt() {
    if [[ ${DOWNLOAD_MODEL_PT} != "1" ]]
    then
        return
    fi

    if [[ -f ${MODEL_PT_SRC} ]]
    then
        rm ${MODEL_PT_SRC}
    fi

    cd ${SCRIPT_WORK_DIR}
    wget -O bench_pt.py ${MODEL_PT_BASE_LINK}/${MODEL_PT_FILE}?${MODEL_PT_TOKEN}
}

run_model_tf() {
    if [[ ${RUN_MODEL_TF} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${MODEL_TF_SRC_DIR} ]]
    then
        echo "ERROR: MODEL_TF_SRC_DIR (${MODEL_TF_SRC_DIR}) is not directory, try to run script with \"-download_model_tf 1\""
        CheckCommandExitCode 1 "Run model failed"
    fi

    cd ${SCRIPT_WORK_DIR}

    current_date=`date "+%Y%m%d%H%M%S"`
    rn50_log_file="rn50_"$current_date".txt"

    rm -rf ${MODEL_TF_SRC_DIR}/resnet50_chk/*
    rm -r ${RN50_MODEL_TF_DIR}/mlperf_resnet/__pycache__

    # ForceLocalMemoryAccessMode=3 is a workaround for NaN failure MLSL-1189: see NEO-6700
    mpiexec_env="ForceLocalMemoryAccessMode=3 GPU=1" mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${RN50_MODEL_TF_DIR}/mlperf_resnet/imagenet_main.py" \
        app_args="2 --max_train_steps=${ITER_COUNT} --train_epochs=10 --epochs_between_evals=10 \
          --inter_op_parallelism_threads 1 --intra_op_parallelism_threads 24 \
          --version 1 --resnet_size 50 --model_dir=${RN50_OUTPUT_DIR} \
          --use_synthetic_data --batch_size=${BATCH_SIZE} --use_bfloat16" \
        logging="2>&1 | tee ${rn50_log_file}" \
        proc_map_iterator
    CheckCommandExitCode $? "Model test failed"

    # calculate avg performance
    python ${SCRIPT_WORK_DIR}/../get_perf.py ${rn50_log_file}
    CheckCommandExitCode $? "Model perf test failed"
}

run_model_pt() {
    if [[ ${RUN_MODEL_PT} != "1" ]]
    then
        return
    fi

    if [[ ! -f ${MODEL_PT_SRC} ]]
    then
        echo "ERROR: MODEL_PT_SRC (${MODEL_PT_SRC}) does not exist, try to run script with \"-download_model_pt 1\""
        CheckCommandExitCode 1 "Run model failed"
    fi

    cd ${SCRIPT_WORK_DIR}

    current_date=`date "+%Y%m%d%H%M%S"`
    rn50_log_file="rn50_"$current_date".txt"
    rn50_log_file_pt="pt_rn50_"$current_date".txt"

    mpiexec_args="${BOOTSTRAP_OPTIONS}" app="python ${MODEL_PT_FILE}" \
        app_args="--iter=${ITER_COUNT} --warm=20 --bs ${BATCH_SIZE} --arch resnet50 --sycl --horovod" \
        logging="2>&1 | tee ${rn50_log_file_pt}" \
        proc_map_iterator
    CheckCommandExitCode $? "IPEX Model test failed"
}

parse_arguments $@

set_extra_proxy

check_base_env

download_ccl
install_ccl

download_conda
create_conda ${CONDA_PACKAGES}
activate_conda

download_fw
install_fw

set_run_env

check_fw

set_mpich_ofi_env

download_hvd
install_hvd
check_hvd

download_model_tf
run_model_tf

download_model_pt
run_model_pt

deactivate_conda
remove_conda

echo_log "TEST PASSED"
