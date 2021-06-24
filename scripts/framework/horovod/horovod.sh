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

set_run_env() {
    # model
    export PYTHONPATH=${RN50_MODEL_DIR}:${PYTHONPATH}

    # Tensorflow
    export TF_ENABLE_LAYOUT_OPT=0
    export ITEX_ENABLE_TILE_AS_DEVICE=1
    export ITEX_ENABLE_ONEDNN_LAYOUT_OPT=1

    # PyTorch
    export IPEX_TILE_AS_DEVICE=1

    # HVD
    export HOROVOD_LOG_LEVEL=INFO
    export HOROVOD_THREAD_AFFINITY=0,1
    #export HOROVOD_FUSION_THRESHOLD=150000000
    #export HOROVOD_DISABLE_GROUP_FUSION=1
    #export HOROVOD_CYCLE_TIME=0.1

    # CCL
    export CCL_LOG_LEVEL=INFO
    export CCL_COMM_KERNELS=0
    export CCL_WORKER_COUNT=1
    export CCL_WORKER_AFFINITY=4-19
    export CCL_ATL_TRANSPORT=mpi
    vars_file="${CCL_SRC_DIR}/build/_install/env/setvars.sh"
    if [[ -f ${vars_file} ]]
    then
        source ${vars_file}
        unset CCL_CONFIGURATION
    fi

    # IMPI
    export I_MPI_DEBUG=12
    export I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=${HOROVOD_THREAD_AFFINITY}
    export I_MPI_PIN_PROCESSOR_LIST=2,3

    # OFI
    export FI_PROVIDER=tcp
    export FI_LOG_LEVEL=debug

    # SYCL
    export SYCL_PI_LEVEL_ZERO_BATCH_SIZE=1
    export SYCL_DEVICE_FILTER={level_zero:gpu:0}
    export ZE_ROOT=/usr/local

    if [ "${CCL_COMM_KERNELS}" == "1" ]
    then
        ulimit -n 1048576
        export CCL_WORKER_OFFLOAD=0
        export SYCL_PI_LEVEL_ZERO_DISABLE_USM_ALLOCATOR=1
    else
        export SYCL_PI_LEVEL_ZERO_TRACK_INDIRECT_ACCESS_MEMORY=1
    fi

    # conda
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CONDA_PREFIX}/lib"
}

CONDA_LINK="https://repo.anaconda.com/miniconda/Miniconda3-py37_4.9.2-Linux-x86_64.sh"
CONDA_INSTALL_DIR=""

CCL_LINK="https://gitlab.devtools.intel.com/ict/ccl-team/ccl.git"
HOROVOD_BASE_LINK="github.com/intel-innersource/frameworks.ai.horovod.git"
MODEL_BASE_LINK="github.com/intel-innersource/frameworks.ai.models.intel-models"
MODEL_SRC_BRANCH="yang/resnet50-training-tmp"
MODEL_SRC_COMMIT="3713f6d531685b509a0fbb5eee7f7836dd37f8b8"

TF_LINK="http://mlpc.intel.com/downloads/gpu/acceptance/ww23/compiler-20210527/itex/ubuntu/tensorflow-2.5.0-cp37-cp37m-linux_x86_64.whl"
ITEX_LINK="http://mlpc.intel.com/downloads/gpu/acceptance/ww23/compiler-20210527/itex/ubuntu/intel_extension_for_tensorflow-0.1.0-cp37-cp37m-linux_x86_64.whl"
TF_NAME=`basename $TF_LINK`
ITEX_NAME=`basename $ITEX_LINK`

PT_LINK="http://10.165.58.120:8080/ipexgpu/latest/torch-1.7.0a0-cp37-cp37m-linux_x86_64.whl"
IPEX_LINK="http://10.165.58.120:8080/ipexgpu/latest/torch_ipex-0.1+7d6e91e-cp37-cp37m-linux_x86_64.whl"
PT_NAME=`basename $PT_LINK`
IPEX_NAME=`basename $IPEX_LINK`

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"

DEFAULT_DOWNLOAD_CCL="0"
DEFAULT_INSTALL_CCL="0"

DEFAULT_DOWNLOAD_CONDA="0"
DEFAULT_CREATE_CONDA="0"
DEFAULT_REMOVE_CONDA="0"
DEFAULT_CONDA_ENV_NAME="test_horovod"

DEFAULT_DOWNLOAD_TF="0"
DEFAULT_INSTALL_TF="0"
DEFAULT_TF_PATH=""
DEFAULT_DOWNLOAD_ITEX="0"
DEFAULT_INSTALL_ITEX="0"
DEFAULT_ITEX_PATH=""
DEFAULT_DOWNLOAD_PT="0"
DEFAULT_INSTALL_PT="0"
DEFAULT_PT_PATH=""
DEFAULT_DOWNLOAD_IPEX="0"
DEFAULT_INSTALL_IPEX="0"
DEFAULT_IPEX_PATH=""

DEFAULT_DOWNLOAD_HVD="0"
DEFAULT_INSTALL_HVD="0"

DEFAULT_DOWNLOAD_MODEL="0"
DEFAULT_RUN_MODEL="0"
DEFAULT_BATCH_SIZE="128"
DEFAULT_ITER_COUNT="200"

DEFAULT_PATH_TO_TOKEN_FILE_1S=""
DEFAULT_USERNAME_1S=""
DEFAULT_PROXY="http://proxy-us.intel.com:912"

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
    echo_log "  -install_tf <bool_flag>"
    echo_log "      Install TensorFlow"
    echo_log "  -tf_path <path>"
    echo_log "      Install TensorFlow from path (*.whl)"
    echo_log "  -download_itex <bool_flag>"
    echo_log "      Download ITEX"
    echo_log "  -install_itex <bool_flag>"
    echo_log "      Install ITEX"
    echo_log "  -itex_path <path>"
    echo_log "      Install ITEX from path (*.whl)"
    echo_log "  -download_pt <bool_flag>"
    echo_log "      Download PyTorch"
    echo_log "  -install_pt <bool_flag>"
    echo_log "      Install PyTorch"
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
    echo_log "      Install Horovod"
    echo_log "  -download_model <bool_flag>"
    echo_log "      Download repository with models"
    echo_log "  -run_model <bool_flag>"
    echo_log "      Run few iterations of RN50 training on 2 ranks"
    echo_log "  -batch_size <size>"
    echo_log "      Batch size for RN50 training"
    echo_log "  -iter_count <count>"
    echo_log "      Iteration count for RN50 training"
    echo_log "  -token <path>"
    echo_log "      Path to file with github credentials"
    echo_log "  -username <name>"
    echo_log "      Github username with access to Horovod repo"
    echo_log "  -proxy <url>"
    echo_log "      https proxy"
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

    DOWNLOAD_CONDA=${DEFAULT_DOWNLOAD_CONDA}
    CREATE_CONDA=${DEFAULT_CREATE_CONDA}
    REMOVE_CONDA=${DEFAULT_REMOVE_CONDA}
    CONDA_ENV_NAME=${DEFAULT_CONDA_ENV_NAME}

    DOWNLOAD_TF=${DEFAULT_DOWNLOAD_TF}
    INSTALL_TF=${DEFAULT_INSTALL_TF}
    TF_PATH=${DEFAULT_TF_PATH}
    DOWNLOAD_ITEX=${DEFAULT_DOWNLOAD_ITEX}
    INSTALL_ITEX=${DEFAULT_INSTALL_ITEX}
    ITEX_PATH=${DEFAULT_ITEX_PATH}
    DOWNLOAD_PT=${DEFAULT_DOWNLOAD_PT}
    INSTALL_PT=${DEFAULT_INSTALL_PT}
    PT_PATH=${DEFAULT_PT_PATH}
    DOWNLOAD_IPEX=${DEFAULT_DOWNLOAD_IPEX}
    INSTALL_IPEX=${DEFAULT_INSTALL_IPEX}
    IPEX_PATH=${DEFAULT_IPEX_PATH}

    DOWNLOAD_HVD=${DEFAULT_DOWNLOAD_HVD}
    INSTALL_HVD=${DEFAULT_INSTALL_HVD}

    DOWNLOAD_MODEL=${DEFAULT_DOWNLOAD_MODEL}
    RUN_MODEL=${DEFAULT_RUN_MODEL}
    BATCH_SIZE=${DEFAULT_BATCH_SIZE}
    ITER_COUNT=${DEFAULT_ITER_COUNT}

    PATH_TO_TOKEN_FILE_1S=${DEFAULT_PATH_TO_TOKEN_FILE_1S}
    USERNAME_1S=${DEFAULT_USERNAME_1S}
    PROXY=${DEFAULT_PROXY}

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
            "-download_model")
                DOWNLOAD_MODEL=${2}
                shift
                ;;
            "-run_model")
                RUN_MODEL=${2}
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
            "-token")
                PATH_TO_TOKEN_FILE_1S=${2}
                shift
                ;;
            "-username")
                USERNAME_1S="${2}@"
                shift
                ;;
            "-proxy")
                PROXY="${2}@"
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
    MODEL_SRC_DIR=${SCRIPT_WORK_DIR}/model
    RN50_MODEL_DIR=${MODEL_SRC_DIR}/models/image_recognition/tensorflow/resnet50v1_5/training
    RN50_OUTPUT_DIR=${MODEL_SRC_DIR}/resnet50_chk

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        DOWNLOAD_CCL="1"
        INSTALL_CCL="1"

        DOWNLOAD_CONDA="1"
        CREATE_CONDA="1"

        DOWNLOAD_TF="1"
        INSTALL_TF="1"
        DOWNLOAD_ITEX="1"
        INSTALL_ITEX="1"

        # TODO: enable PT tests
        # DOWNLOAD_PT="1"
        # INSTALL_PT="1"
        # DOWNLOAD_IPEX="1"
        # INSTALL_IPEX="1"

        DOWNLOAD_HVD="1"
        INSTALL_HVD="1"

        DOWNLOAD_MODEL="1"
        RUN_MODEL="1"

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
    fi

    if [[ ${DOWNLOAD_PT} = "1" ]]
    then
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"
    fi

    if [[ ${DOWNLOAD_HVD} = "1" ]]
    then
        INSTALL_HVD="1"
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR    = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE         = ${FULL_SCOPE}"

    echo_log "DOWNLOAD_CCL       = ${DOWNLOAD_CCL}"
    echo_log "INSTALL_CCL        = ${INSTALL_CCL}"

    echo_log "DOWNLOAD_CONDA     = ${DOWNLOAD_CONDA}"
    echo_log "CREATE_CONDA       = ${CREATE_CONDA}"
    echo_log "REMOVE_CONDA       = ${REMOVE_CONDA}"
    echo_log "CONDA_ENV_NAME     = ${CONDA_ENV_NAME}"

    echo_log "DOWNLOAD_TF        = ${DOWNLOAD_TF}"
    echo_log "INSTALL_TF         = ${INSTALL_TF}"
    echo_log "TF_PATH            = ${TF_PATH}"
    echo_log "DOWNLOAD_ITEX      = ${DOWNLOAD_ITEX}"
    echo_log "INSTALL_ITEX       = ${INSTALL_ITEX}"
    echo_log "ITEX_PATH          = ${ITEX_PATH}"

    echo_log "DOWNLOAD_PT        = ${DOWNLOAD_PT}"
    echo_log "INSTALL_PT         = ${INSTALL_PT}"
    echo_log "PT_PATH            = ${PT_PATH}"
    echo_log "DOWNLOAD_IPEX      = ${DOWNLOAD_IPEX}"
    echo_log "INSTALL_IPEX       = ${INSTALL_IPEX}"
    echo_log "IPEX_PATH          = ${IPEX_PATH}"

    echo_log "DOWNLOAD_HVD       = ${DOWNLOAD_HVD}"
    echo_log "INSTALL_HVD        = ${INSTALL_HVD}"

    echo_log "DOWNLOAD_MODEL     = ${DOWNLOAD_MODEL}"
    echo_log "RUN_MODEL          = ${RUN_MODEL}"
    echo_log "BATCH_SIZE         = ${BATCH_SIZE}"
    echo_log "ITER_COUNT         = ${ITER_COUNT}"

    echo_log "USERNAME_1S        = ${USERNAME_1S}"
    echo_log "PROXY              = ${PROXY}"
}

echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

hvd_test() {
    # TODO: fix OFI env in CCL jenkins
    return

    if [[ ${INSTALL_TF} = "1" ]]
    then
        echo_log "============================= Basic Horovod/TF test =================================="
        cmd="python -c \"import horovod.tensorflow as hvd; hvd.init(); print(hvd.local_rank())\""
        echo_log ${cmd}
        eval ${cmd}
        CheckCommandExitCode $? "Basic Horovod/TF test failed"
        echo_log "============================= ****************** =================================="

        # TODO: remove after comm kernels correctness fix
        if [[ ${CCL_COMM_KERNELS} = "0" ]] || [[ ${CCL_CONFIGURATION} != "" ]]
        then
            echo_log "============================= Horovod/TF bench =================================="
            cmd="mpiexec -n 2 -l python ${HVD_SRC_DIR}/benchmark/hvd_tf_bench.py --xpu gpu"
            echo_log ${cmd}
            eval ${cmd}
            CheckCommandExitCode $? "Basic Horovod/TF test failed"
            echo_log "============================= ****************** =================================="
        fi
    fi

    if [[ ${INSTALL_PT} = "1" ]]
    then
        echo_log "============================= Basic Horovod/PT test =================================="
        cmd="python -c \"import horovod.torch as hvd; hvd.init(); print(hvd.local_rank())\""
        echo_log ${cmd}
        eval ${cmd}
        CheckCommandExitCode $? "Basic Horovod/PT test failed"
        echo_log "============================= ****************** =================================="
    fi
}

tf_test() {
    if [[ ${INSTALL_TF} != "1" ]]
    then
        return
    fi

    echo_log "===================================== Basic TF test ==========================================="
    echo_log "python -c \"import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())\""
    python -c "import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())"
    CheckCommandExitCode $? "Basic TF test failed"
    echo_log "===================================== ************* ==========================================="
}

pt_test() {
    if [[ ${INSTALL_PT} != "1" ]]
    then
        return
    fi

    echo_log "===================================== Basic PT test ==========================================="
    echo_log "python -c \"import torch; import torch_ipex; print(torch.__version__)\""
    python -c "import torch; import torch_ipex; print(torch.__version__)"
    CheckCommandExitCode $? "Basic PT test failed"
    echo_log "===================================== ************* ==========================================="
}

check_base_env() {
    which dpcpp
    CheckCommandExitCode $? "DPCPP was not found"

    echo_log "DPCPP version:"
    echo_log `dpcpp -v`
}

check_horovod_env() {
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
    https_proxy="" git clone --branch master --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
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
        -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp_level_zero
    CheckCommandExitCode $? "Configure CCL failed"

    make -j install
    CheckCommandExitCode $? "Install CCL failed"

    vars_file="${CCL_SRC_DIR}/build/_install/env/setvars.sh"
    source ${vars_file}
}

download_conda() {
    if [[ ${DOWNLOAD_CONDA} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}
    CONDA_FILENAME="conda.sh"
    wget -O ${CONDA_FILENAME} ${CONDA_LINK}
    chmod +x ${CONDA_FILENAME}
    CONDA_INSTALL_DIR="${SCRIPT_WORK_DIR}/conda"
    ./${CONDA_FILENAME} -b -p ${CONDA_INSTALL_DIR}
    export CONDA_ENVS_PATH="${CONDA_INSTALL_DIR}/envs"
    export CONDA_PKGS_DIRS="${CONDA_INSTALL_DIR}/pkgs"
    export PIP_CACHE_DIR="${CONDA_INSTALL_DIR}/pip"
    mkdir -p ${PIP_CACHE_DIR}
}

create_conda() {
    if [[ ${CREATE_CONDA} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [ -d "${CONDA_INSTALL_DIR}" ]
    then
        if [ -f "${CONDA_INSTALL_DIR}/etc/profile.d/conda.sh" ]
        then
            source "${CONDA_INSTALL_DIR}/etc/profile.d/conda.sh"
        else
            export PATH="${CONDA_INSTALL_DIR}/bin:$PATH"
        fi
    fi

    echo_log `which conda`

    echo_log "\n=== create conda env ===\n"
    env_count=`conda env list | grep "${CONDA_ENV_NAME} " | wc -l`
    if [[ $env_count -ne 0 ]]
    then
        echo_log "found conda env ${CONDA_ENV_NAME}"
    else
        echo_log "didn't find conda env ${CONDA_ENV_NAME}, create new one"
        https_proxy=${PROXY} conda create -y -n ${CONDA_ENV_NAME} python=3.7
    fi

    activate_conda

    echo_log "\n=== install packages in conda env ${CONDA_ENV_NAME} ===\n"
    python3 -m pip install --upgrade pip

    echo_log "\n=== CONDA_PREFIX ${CONDA_PREFIX} ===\n"

    deactivate_conda
}

activate_conda() {
    echo_log "\n=== activate conda env ${CONDA_ENV_NAME} ===\n"

    if [ -d "${CONDA_INSTALL_DIR}" ]
    then
        CONDA_BIN_DIR="${CONDA_INSTALL_DIR}/bin"
    else
        CONDA_BIN_DIR=$(dirname $(which python))
    fi

    activate_script="${CONDA_BIN_DIR}/activate"

    if [[ ! -f ${activate_script} ]]
    then
        echo "ERROR: activate_script (${activate_script}) is not a file, try to run script with \"-download_conda 1 -create_conda 1\""
        CheckCommandExitCode 1 "Install CCL failed"
    fi

    source ${activate_script} ${CONDA_ENV_NAME}
}

deactivate_conda() {
    echo_log "\n=== deactivate conda env ${CONDA_ENV_NAME} ===\n"
    conda deactivate
}

remove_conda() {
    if [[ ${REMOVE_CONDA} != "1" ]]
    then
        return
    fi

    conda env remove -y --name ${CONDA_ENV_NAME}
}

download_fw() {

    cd ${SCRIPT_WORK_DIR}

    if [[ ${DOWNLOAD_TF} = "1" ]]
    then
        echo_log "\n=== download TF ===\n"
        wget -e use_proxy=no ${TF_LINK}
    fi

    if [[ ${DOWNLOAD_ITEX} = "1" ]]
    then
        echo_log "\n=== download ITEX ===\n"
        wget -e use_proxy=no ${ITEX_LINK}
    fi

    if [[ ${DOWNLOAD_PT} = "1" ]]
    then
        echo_log "\n=== download PT ===\n"
        wget -e use_proxy=no ${PT_LINK}
    fi

    if [[ ${DOWNLOAD_IPEX} = "1" ]]
    then
        echo_log "\n=== download IPEX ===\n"
        wget -e use_proxy=no ${IPEX_LINK}
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
        echo_log "\n=== uninstall PT ===\n"
        pip uninstall torch

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

    if [[ ${INSTALL_TF} = "1" ]]
    then
        tf_test
        CheckCommandExitCode $? "TF test failed"
    fi

    if [[ ${INSTALL_PT} = "1" ]]
    then
        pt_test
        CheckCommandExitCode $? "PT test failed"
    fi
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
    GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git clone --recursive --branch xpu --single-branch \
        https://${USERNAME_1S}${HOROVOD_BASE_LINK} ${HVD_SRC_DIR}
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

    check_horovod_env

    cmd="I_MPI_CXX=dpcpp"
    cmd="$cmd CXX=mpicxx"
    cmd="$cmd LDSHARED=\"dpcpp -shared -fPIC\""
    cmd="$cmd CC=clang"
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
    if [[ ${INSTALL_HVD} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}
    hvd_test
    CheckCommandExitCode $? "Horovod test failed"
}

download_model() {
    if [[ ${DOWNLOAD_MODEL} != "1" ]]
    then
        return
    fi

    if [[ -d ${MODEL_SRC_DIR} ]]
    then
        rm -rf ${MODEL_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}
    GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git clone --branch ${MODEL_SRC_BRANCH} --single-branch \
        https://${USERNAME_1S}${MODEL_BASE_LINK} ${MODEL_SRC_DIR}
    cd ${MODEL_SRC_DIR}
    git checkout ${MODEL_SRC_COMMIT}
}

run_model() {
    if [[ ${RUN_MODEL} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${MODEL_SRC_DIR} ]]
    then
        echo "ERROR: MODEL_SRC_DIR (${MODEL_SRC_DIR}) is not directory, try to run script with \"-download_model 1\""
        CheckCommandExitCode 1 "Run model failed"
    fi

    cd ${SCRIPT_WORK_DIR}

    current_date=`date "+%Y%m%d%H%M%S"`
    rn50_log_file="rn50_"$current_date".txt"

    rm -rf ${MODEL_SRC_DIR}/resnet50_chk/*
    rm -r ${RN50_MODEL_DIR}/mlperf_resnet/__pycache__

    cmd="GPU=1 mpiexec -n 2 -l python ${RN50_MODEL_DIR}/mlperf_resnet/imagenet_main.py 2 \
      --max_train_steps=${ITER_COUNT} --train_epochs=10 --epochs_between_evals=10 \
      --inter_op_parallelism_threads 1 --intra_op_parallelism_threads 24  \
      --version 1 --resnet_size 50 --model_dir=${RN50_OUTPUT_DIR} \
      --use_synthetic_data --batch_size=${BATCH_SIZE} --use_bfloat16 2>&1 | tee ${rn50_log_file}"

    echo_log "\nrn50 cmd:\n${cmd}\n"
    eval ${cmd}
    CheckCommandExitCode $? "Model test failed"

    # calculate avg performance
    python ${SCRIPT_WORK_DIR}/../get_perf.py ${rn50_log_file}
    CheckCommandExitCode $? "Model perf test failed"
}

parse_arguments $@

check_base_env

download_ccl
install_ccl

download_conda
create_conda
activate_conda

download_fw
install_fw

set_run_env

check_fw

download_hvd
install_hvd
check_hvd

download_model
run_model

deactivate_conda
remove_conda

echo_log "TEST PASSED"
