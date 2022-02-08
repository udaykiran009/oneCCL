#!/bin/bash

BASENAME=$(basename $0 .sh)
SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)

current_date=$(date "+%Y%m%d%H%M%S")
log_file="${SCRIPT_DIR}/log_${current_date}.txt"
if [ ! -e "${log_file}" ]; then
    touch "${log_file}"
else
    echo -e "[ WARN ] ${log_file} exists, will be overwrite"
    rm "${log_file}" && touch "${log_file}"
fi

SCRIPTS_ROOT_DIR=$(realpath ${SCRIPT_DIR}/../../../)
source ${SCRIPTS_ROOT_DIR}/utils/common.sh

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"

TORCH_CCL_BRANCH="chengjun/xpu_torch_ccl_1.7"
TORCH_CCL_BASE_LINK="github.com/intel-innersource/frameworks.ai.pytorch.torch-ccl.git"
TORCH_LINK="http://mlpc.intel.com/downloads/gpu-new/releases/v0.3.0/IPEX/v0.3.0-rc3_py39_pvc/rc3_new/torch-1.7.0a0+0ecf0b5-cp39-cp39-linux_x86_64.whl"
IPEX_LINK="http://mlpc.intel.com/downloads/gpu-new/releases/v0.3.0/IPEX/v0.3.0-rc3_py39_pvc/rc3_new/ipex-0.3.0+b7b201da-cp39-cp39-linux_x86_64.whl"

TORCH_NAME=$(basename ${TORCH_LINK})
IPEX_NAME=$(basename ${IPEX_LINK})

MODELS_BRANCH="master"
MODELS_BASE_LINK="github.com/intel-innersource/frameworks.ai.pytorch.gpu-models.git"

DEFAULT_IMAGENET="${HOME}/datasets/imagenet"

ONECCL_BRANCH="master"
ONECCL_BASE_LINK="github.com/intel-innersource/libraries.performance.communication.oneccl.git"

CONDA_LINK="https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh"
CONDA_PACKAGES="cmake python=3.9"

DEFAULT_DOWNLOAD_CONDA="0"
DEFAULT_CREATE_CONDA="0"
DEFAULT_REMOVE_CONDA="0"
DEFAULT_CONDA_ENV_NAME="torch_ccl_test"

DEFAULT_DOWNLOAD_PT="0"
DEFAULT_INSTALL_PT="0"
DEFAULT_TORCH_PATH=""
DEFAULT_DOWNLOAD_IPEX="0"
DEFAULT_INSTALL_IPEX="0"
DEFAULT_IPEX_PATH=""
DEFAULT_DOWNLOAD_TORCH_CCL="0"
DEFAULT_INSTALL_TORCH_CCL="0"

DEFAULT_RUN_UT="0"
DEFAULT_RUN_DEMO="0"
DEFAULT_RUN_RN50="0"

DEFAULT_PATH_TO_TOKEN_FILE_1S=""
DEFAULT_USERNAME_1S=""

DEFAULT_PROXY="http://proxy-us.intel.com:912"
DEFAULT_EXTRA_PROXY=0

set_run_env() {
    # GPU SW
    export EnableDirectSubmission=1

    # CCL
    export CCL_LOG_LEVEL=INFO
    export CCL_SYCL_OUTPUT_EVENT=1
    source "$(python -c 'import torch_ccl; print(torch_ccl.cwd)')/env/setvars.sh"

    # OFI
    export FI_PROVIDER=tcp

    # IMPI
    export I_MPI_DEBUG=12
}

print_run_env() {
    echo_log "I_MPI_ROOT: ${I_MPI_ROOT}"
    echo_log "CCL_ROOT: ${CCL_ROOT}"
}

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
    echo_log "- source oneAPI Base Toolkit before script launch"
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
    echo_log "  -download_torch_ccl <bool_flag>"
    echo_log "      Download torch-ccl"
    echo_log "  -install_torch_ccl <bool_flag>"
    echo_log "      Install torch-ccl"
    echo_log "  -run_ut <bool_flag>"
    echo_log "      Run unit tests"
    echo_log "  -run_demo <bool_flag>"
    echo_log "      Run demo test"
    echo_log "  -run_rn50 <bool_flag>"
    echo_log "      Run ResNet50"
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
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh "
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}

    IMAGENET="${DEFAULT_IMAGENET}"

    DOWNLOAD_CONDA=${DEFAULT_DOWNLOAD_CONDA}
    CREATE_CONDA=${DEFAULT_CREATE_CONDA}
    REMOVE_CONDA=${DEFAULT_REMOVE_CONDA}
    CONDA_ENV_NAME=${DEFAULT_CONDA_ENV_NAME}

    DOWNLOAD_PT=${DEFAULT_DOWNLOAD_PT}
    INSTALL_PT=${DEFAULT_INSTALL_PT}
    TORCH_PATH=${DEFAULT_TORCH_PATH}
    DOWNLOAD_IPEX=${DEFAULT_DOWNLOAD_IPEX}
    INSTALL_IPEX=${DEFAULT_INSTALL_IPEX}
    IPEX_PATH=${DEFAULT_IPEX_PATH}
    DOWNLOAD_TORCH_CCL=${DEFAULT_DOWNLOAD_TORCH_CCL}
    INSTALL_TORCH_CCL=${DEFAULT_INSTALL_TORCH_CCL}

    RUN_UT=${DEFAULT_RUN_UT}
    RUN_DEMO=${DEFAULT_RUN_DEMO}
    RUN_RN50=${DEFAULT_RUN_RN50}

    PATH_TO_TOKEN_FILE_1S=${DEFAULT_PATH_TO_TOKEN_FILE_1S}
    USERNAME_1S=${DEFAULT_USERNAME_1S}

    PROXY=${DEFAULT_PROXY}
    SET_EXTRA_PROXY=${DEFAULT_EXTRA_PROXY}

    if [[ $# -eq 0 ]]; then
        print_help
        exit 1
    fi

    while [[ $# -ne 0 ]]
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
            "-download_pt")
                DOWNLOAD_PT=${2}
                shift
                ;;
            "-install_pt")
                INSTALL_PT=${2}
                shift
                ;;
            "-pt_path")
                TORCH_PATH=${2}
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
            "-download_torch_ccl")
                DOWNLOAD_TORCH_CCL=${2}
                shift
                ;;
            "-install_torch_ccl")
                INSTALL_TORCH_CCL=${2}
                shift
                ;;
            "-run_ut")
                RUN_UT=${2}
                shift
                ;;
            "-run_demo")
                RUN_DEMO=${2}
                shift
                ;;
            "-run_rn50")
                RUN_RN50=${2}
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
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        DOWNLOAD_CONDA="1"
        CREATE_CONDA="1"

        DOWNLOAD_PT="1"
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"
        DOWNLOAD_TORCH_CCL="1"
        INSTALL_TORCH_CCL="1"

        RUN_UT="1"
        RUN_DEMO="1"
        RUN_RN50="1"

        remove_conda
        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
        fi
    fi

    if [[ ${DOWNLOAD_PT} = "1" ]]
    then
        INSTALL_PT="1"
        DOWNLOAD_IPEX="1"
        INSTALL_IPEX="1"
        DOWNLOAD_TORCH_CCL="1"
        INSTALL_TORCH_CCL="1"
    fi

    mkdir -p ${SCRIPT_WORK_DIR}
    CONDA_INSTALL_DIR="${SCRIPT_WORK_DIR}/conda"

    if [[ ${DOWNLOAD_CONDA} = "1" ]]
    then
        CREATE_CONDA="1"
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR    = ${SCRIPT_WORK_DIR}"

    echo_log "DOWNLOAD_CONDA     = ${DOWNLOAD_CONDA}"
    echo_log "CREATE_CONDA       = ${CREATE_CONDA}"
    echo_log "REMOVE_CONDA       = ${REMOVE_CONDA}"
    echo_log "CONDA_ENV_NAME     = ${CONDA_ENV_NAME}"

    echo_log "DOWNLOAD_PT        = ${DOWNLOAD_PT}"
    echo_log "INSTALL_PT         = ${INSTALL_PT}"
    echo_log "TORCH_PATH         = ${TORCH_PATH}"
    echo_log "DOWNLOAD_IPEX      = ${DOWNLOAD_IPEX}"
    echo_log "INSTALL_IPEX       = ${INSTALL_IPEX}"
    echo_log "IPEX_PATH          = ${IPEX_PATH}"
    echo_log "DOWNLOAD_TORCH_CCL = ${DOWNLOAD_TORCH_CCL}"
    echo_log "INSTALL_TORCH_CCL  = ${INSTALL_TORCH_CCL}"

    echo_log "RUN_UT             = ${RUN_UT}"
    echo_log "RUN_DEMO           = ${RUN_DEMO}"
    echo_log "RUN_RN50           = ${RUN_RN50}"

    echo_log "USERNAME_1S        = ${USERNAME_1S}"
    echo_log "PROXY              = ${PROXY}"
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
    if [ -d "${CONDA_INSTALL_DIR}" ]; then
        rm -rf ${CONDA_INSTALL_DIR}
    fi
}

download_torch_ccl() {
    if [ -d "${SCRIPT_WORK_DIR}/torch_ccl" ]; then
        rm -rf "${SCRIPT_WORK_DIR}/torch_ccl"
    fi

    clone_repo "Torch-ccl" "https://${USERNAME_1S}${TORCH_CCL_BASE_LINK}" "${TORCH_CCL_BRANCH}" \
        "${SCRIPT_WORK_DIR}/torch_ccl" "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"

    pushd "${SCRIPT_WORK_DIR}/torch_ccl" || exit 1
    rm -rf third_party/oneCCL
    clone_repo "oneCCL" "https://${USERNAME_1S}${ONECCL_BASE_LINK}" "${ONECCL_BRANCH}" \
        "third_party/oneCCL" "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"

    if [[ ! -z "${CCL_PR_NUMBER}" ]]; then
        pushd "third_party/oneCCL"
        GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git fetch origin pull/${CCL_PR_NUMBER}/head:pull_${CCL_PR_NUMBER}
        git checkout pull_${CCL_PR_NUMBER}
        git log -1
        popd
    fi
    popd
}

install_torch_ccl() {
    pushd "${SCRIPT_WORK_DIR}/torch_ccl" || exit 1
    COMPUTE_BACKEND=dpcpp_level_zero python setup.py install
    popd
}

download_fw() {
    if [[ ${DOWNLOAD_PT} == "1" ]]
    then
        wget -nv -e use_proxy=no ${TORCH_LINK}
    fi

    if [[ ${DOWNLOAD_IPEX} == "1" ]]
    then
        wget -nv -e use_proxy=no ${IPEX_LINK}
    fi

    if [[ ${DOWNLOAD_TORCH_CCL} == "1" ]]
    then
        download_torch_ccl
    fi
}

install_fw() {
    if [[ ${INSTALL_PT} == "1" ]]
    then
        if [[ -f ${TORCH_PATH} ]]
        then
            install_name=$(realpath ${TORCH_PATH})
        else
            install_name=$(realpath ${TORCH_NAME})
        fi
        python -m pip install ${TORCH_NAME}
    fi

    if [[ ${INSTALL_IPEX} == "1" ]]
    then
        if [[ -f ${IPEX_PATH} ]]
        then
            install_name=$(realpath ${IPEX_PATH})
        else
            install_name=$(realpath ${IPEX_NAME})
        fi
        python -m pip install ${IPEX_NAME}
    fi

    if [[ ${INSTALL_TORCH_CCL} == "1" ]]
    then
        install_torch_ccl
    fi
}

check_fw() {
    if [[ ${INSTALL_PT} == "1" ]]
    then
        echo_log "Torch Version: $(python -c 'import torch; print(torch.__version__)')"
        echo_log "Torch has MKL: $(python -c 'import torch; print(torch.has_mkl)')"
    fi

    if [[ ${INSTALL_IPEX} == "1" ]]
    then
        echo_log "IPEX Version:"
        echo_log "$(python -c 'import ipex; ipex.version()')"
        echo_log "IPEX oneMKL:  $(python -c 'import ipex; import torch; print(torch.xpu.settings.has_onemkl())')"
    fi
}

install_rn50_requirements() {
    python -m pip install --no-cache-dir -I pillow
    python -m pip install torchvision==0.8.2 --no-deps
}

download_models() {
    if [ -d "${SCRIPT_WORK_DIR}/models" ]; then
        rm -rf "${SCRIPT_WORK_DIR}/models"
    fi
    clone_repo "GPU models" "https://${USERNAME_1S}${MODELS_BASE_LINK}" "${MODELS_BRANCH}" \
        "${SCRIPT_WORK_DIR}/models" "GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S}"
}

run_torch_ccl_ut() {
    if [[ ${RUN_UT} != "1" ]]
    then
        return
    fi

    pushd "${SCRIPT_WORK_DIR}/torch_ccl/tests" || exit 1

    python -m pip install hypothesis pytest

    print_run_env
    python -m pytest -s -v ./test_c10d_ccl.py

    if [[ "$?" == "0" ]]; then
        echo -e "[PASS] torch ccl ut pass"
    else
        echo -e "[FAIL] torch ccl ut fail"
        exit 1
    fi
    popd
}

run_torch_ccl_demo() {
    if [[ ${RUN_DEMO} != "1" ]]
    then
        return
    fi

    pushd "${SCRIPT_WORK_DIR}/torch_ccl" || exit 1

    print_run_env
    mpiexec -n 2 python ./demo/demo.py

    if [ "$?" == "0" ]; then
        echo -e "[PASS] torch ccl gpu demo pass"
    else
        echo -e "[FAIL] torch ccl gpu demo fail"
        exit 1
    fi
    popd || exit
}

run_torch_ccl_rn50() {
    if [[ ${RUN_RN50} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${IMAGENET}/val ]] || [[ ! -d ${IMAGENET}/train ]]; then
        echo -e "[FAIL] ${IMAGENET}/val or ${IMAGENET}/train doens't exists!"
        exit 1
    fi

    install_rn50_requirements
    download_models

    pushd "${SCRIPT_WORK_DIR}/models/resnet50" || exit 1

    print_run_env
    mpiexec -n 2 python main.py -a resnet50 -b 128 --xpu 0 --epochs 1 --dry-run 5 --num-iterations 20 --bf16 1 --seed 1 "${IMAGENET}"
    if [ "$?" == "0" ]; then
        echo -e "[PASS] torch ccl gpu demo pass"
    else
        echo -e "[FAIL] torch ccl gpu demo fail"
        exit 1
    fi
    popd || exit
}

main() {
    parse_arguments $@

    set_extra_proxy

    download_conda
    create_conda ${CONDA_PACKAGES}
    activate_conda

    download_fw
    install_fw
    check_fw

    set_run_env

    run_torch_ccl_ut
    run_torch_ccl_demo
    run_torch_ccl_rn50
    
}

main $@
