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
TESTS_LOG_FILE="${SCRIPT_DIR}/tests_log_${current_date}.txt"
touch ${LOG_FILE}
touch ${TESTS_LOG_FILE}

set_run_env() {
    # UCC
    export UCC_CL_BASIC_USE_CCL=1 # enables CCL transport layer
    export UCC_LOG_LEVEL=INFO
    export UCC_CL_BASIC_LOG_LEVEL=INFO
    export UCC_TL_UCP_LOG_LEVEL=INFO
    export UCC_TL_CCL_LOG_LEVEL=INFO
    export LD_LIBRARY_PATH=${UCX_INSTALL_DIR}/lib:${UCC_INSTALL_DIR}/lib:${LD_LIBRARY_PATH}

    # UCX
    export UCX_LOG_LEVEL=INFO
    export UCX_TLS=tcp,shm,self

    # CCL
    export CCL_LOG_LEVEL=INFO
    export CCL_ATL_TRANSPORT=ofi
    ccl_vars_file="${CCL_INSTALL_DIR}/env/setvars.sh"
    if [[ -f ${ccl_vars_file} ]]
    then
        source ${ccl_vars_file}
    fi

    # IMPI
    export I_MPI_DEBUG=12

    # OFI
    #export FI_LOG_LEVEL=INFO
    export FI_PROVIDER=psm3
    export PSM3_MULTI_EP=1 # to handle case when MPI initializes PSM3 before CCL
    export PSM3_RDMA=2 # enables user space RC QP RDMA
    export PSM3_MR_CACHE_MODE=2 # near term workaround until a MR cache solution completed
    export FI_PROVIDER_PATH=${PSM3_INSTALL_DIR}/lib/libfabric
    export LD_LIBRARY_PATH=${OFI_INSTALL_DIR}/lib:${OFI_INSTALL_DIR}/lib64:${LD_LIBRARY_PATH}
}

PSM3_BASE_LINK="github.com/otcshare/psm3.git"
PSM3_BRANCH="v11.1.0.0.84"

OFI_LINK="https://github.com/ofiwg/libfabric.git"
OFI_BRANCH="v1.12.1"

CCL_LINK="https://github.com/otcshare/oneccl.git"
CCL_BRANCH="main"

UCX_LINK="https://github.com/openucx/ucx.git"
UCX_BRANCH="master"

UCC_BASE_LINK="github.com/otcshare/ucc.git"
UCC_BRANCH="ccl-master"

CONDA_LINK="https://repo.anaconda.com/miniconda/Miniconda3-py37_4.9.2-Linux-x86_64.sh"
CONDA_INSTALL_DIR=""

TORCH_UCC_LINK="https://github.com/facebookresearch/torch_ucc.git"
TORCH_UCC_BRANCH="main"

PARAM_LINK="https://github.com/facebookresearch/param.git"
PARAM_BRANCH="master"


DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"

DEFAULT_DOWNLOAD_OFI="0"
DEFAULT_INSTALL_OFI="0"

DEFAULT_DOWNLOAD_CCL="0"
DEFAULT_INSTALL_CCL="0"

DEFAULT_DOWNLOAD_UCC="0"
DEFAULT_INSTALL_UCC="0"

DEFAULT_DOWNLOAD_CONDA="0"
DEFAULT_CREATE_CONDA="0"
DEFAULT_REMOVE_CONDA="0"
DEFAULT_CONDA_ENV_NAME="test_ucc"

DEFAULT_INSTALL_PT="0"

DEFAULT_DOWNLOAD_TORCH_UCC="0"
DEFAULT_INSTALL_TORCH_UCC="0"

DEFAULT_RUN_TESTS="0"

DEFAULT_PATH_TO_TOKEN_FILE_1S=""
DEFAULT_USERNAME_1S=""
DEFAULT_PROXY=""

check_exit_code() {
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
    echo_log "- GCC 8.3+"
    echo_log ""
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -download_ofi <bool_flag>"
    echo_log "      Download OFI"
    echo_log "  -install_ofi <bool_flag>"
    echo_log "      Install OFI"
    echo_log "  -download_ccl <bool_flag>"
    echo_log "      Download oneCCL"
    echo_log "  -install_ccl <bool_flag>"
    echo_log "      Install oneCCL"
    echo_log "  -download_ucc <bool_flag>"
    echo_log "      Download UCC"
    echo_log "  -install_ucc <bool_flag>"
    echo_log "      Install UCC"
    echo_log "  -download_conda <bool_flag>"
    echo_log "      Download miniconda"
    echo_log "  -create_conda <bool_flag>"
    echo_log "      Create conda environment"
    echo_log "  -remove_conda <bool_flag>"
    echo_log "      Remove conda environment after script completion"
    echo_log "  -env <name>"
    echo_log "      The name for conda environment"
    echo_log "  -install_pt <bool_flag>"
    echo_log "      Install PyTorch"
    echo_log "  -download_torch_ucc <bool_flag>"
    echo_log "      Download torch-ucc"
    echo_log "  -install_torch_ucc <bool_flag>"
    echo_log "      Install torch-ucc"
    echo_log "  -run_tests <bool_flag>"
    echo_log "      Run UCC, torch-ucc and PARAM tests on 2 ranks"
    echo_log "  -token <path>"
    echo_log "      Path to file with github credentials"
    echo_log "  -username <name>"
    echo_log "      Github username with access to Horovod repo"
    echo_log "  -proxy <url>"
    echo_log "      https proxy"
    echo_log ""
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}

    DOWNLOAD_OFI=${DEFAULT_DOWNLOAD_OFI}
    INSTALL_OFI=${DEFAULT_INSTALL_OFI}

    DOWNLOAD_CCL=${DEFAULT_DOWNLOAD_CCL}
    INSTALL_CCL=${DEFAULT_INSTALL_CCL}

    DOWNLOAD_UCC=${DEFAULT_DOWNLOAD_UCC}
    INSTALL_UCC=${DEFAULT_INSTALL_UCC}

    DOWNLOAD_CONDA=${DEFAULT_DOWNLOAD_CONDA}
    CREATE_CONDA=${DEFAULT_CREATE_CONDA}
    REMOVE_CONDA=${DEFAULT_REMOVE_CONDA}
    CONDA_ENV_NAME=${DEFAULT_CONDA_ENV_NAME}

    INSTALL_PT=${DEFAULT_INSTALL_PT}

    DOWNLOAD_TORCH_UCC=${DEFAULT_DOWNLOAD_TORCH_UCC}
    INSTALL_TORCH_UCC=${DEFAULT_INSTALL_TORCH_UCC}

    RUN_TESTS=${DEFAULT_RUN_TESTS}

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
            "-download_ofi")
                DOWNLOAD_OFI=${2}
                shift
                ;;
            "-install_ofi")
                INSTALL_OFI=${2}
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
            "-download_ucc")
                DOWNLOAD_UCC=${2}
                shift
                ;;
            "-install_ucc")
                INSTALL_UCC=${2}
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
            "-install_pt")
                INSTALL_PT=${2}
                shift
                ;;
            "-download_torch_ucc")
                DOWNLOAD_TORCH_UCC=${2}
                shift
                ;;
            "-install_torch_ucc")
                INSTALL_TORCH_UCC=${2}
                shift
                ;;
            "-run_tests")
                RUN_TESTS=${2}
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
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    mkdir -p ${SCRIPT_WORK_DIR}

    PSM3_SRC_DIR=${SCRIPT_WORK_DIR}/psm3
    PSM3_INSTALL_DIR=${PSM3_SRC_DIR}/_install

    OFI_SRC_DIR=${SCRIPT_WORK_DIR}/ofi
    OFI_INSTALL_DIR=${OFI_SRC_DIR}/_install

    CCL_SRC_DIR=${SCRIPT_WORK_DIR}/ccl
    CCL_INSTALL_DIR=${CCL_SRC_DIR}/_install

    UCX_SRC_DIR=${SCRIPT_WORK_DIR}/ucx
    UCX_INSTALL_DIR=${UCX_SRC_DIR}/_install

    UCC_SRC_DIR=${SCRIPT_WORK_DIR}/ucc
    UCC_INSTALL_DIR=${UCC_SRC_DIR}/_install

    TORCH_UCC_SRC_DIR=${SCRIPT_WORK_DIR}/torch_ucc

    PARAM_SRC_DIR=${SCRIPT_WORK_DIR}/param

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        DOWNLOAD_OFI="1"
        INSTALL_OFI="1"

        DOWNLOAD_CCL="1"
        INSTALL_CCL="1"

        DOWNLOAD_UCC="1"
        INSTALL_UCC="1"

        DOWNLOAD_CONDA="1"
        CREATE_CONDA="1"

        INSTALL_PT="1"

        DOWNLOAD_TORCH_UCC="1"
        INSTALL_TORCH_UCC="1"

        RUN_TESTS="1"

        remove_conda
        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR    = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE         = ${FULL_SCOPE}"

    echo_log "DOWNLOAD_OFI       = ${DOWNLOAD_OFI}"
    echo_log "INSTALL_OFI        = ${INSTALL_OFI}"

    echo_log "DOWNLOAD_CCL       = ${DOWNLOAD_CCL}"
    echo_log "INSTALL_CCL        = ${INSTALL_CCL}"

    echo_log "DOWNLOAD_UCC       = ${DOWNLOAD_UCC}"
    echo_log "INSTALL_UCC        = ${INSTALL_UCC}"

    echo_log "DOWNLOAD_CONDA     = ${DOWNLOAD_CONDA}"
    echo_log "CREATE_CONDA       = ${CREATE_CONDA}"
    echo_log "REMOVE_CONDA       = ${REMOVE_CONDA}"
    echo_log "CONDA_ENV_NAME     = ${CONDA_ENV_NAME}"

    echo_log "INSTALL_PT         = ${INSTALL_PT}"

    echo_log "DOWNLOAD_TORCH_UCC = ${DOWNLOAD_TORCH_UCC}"
    echo_log "INSTALL_TORCH_UCC  = ${INSTALL_TORCH_UCC}"

    echo_log "RUN_TESTS          = ${RUN_TESTS}"

    echo_log "PROXY              = ${PROXY}"
}

echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

check_base_env() {
    which gcc
    check_exit_code $? "GCC was not found"

    echo_log "GCC version:"
    echo_log `gcc -v`
}

download_ofi() {
    if [[ ${DOWNLOAD_OFI} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ -d ${PSM3_SRC_DIR} ]]
    then
        rm -rf ${PSM3_SRC_DIR}
    fi
    GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git clone --branch ${PSM3_BRANCH} --single-branch \
        https://${USERNAME_1S}${PSM3_BASE_LINK} ${PSM3_SRC_DIR}
    check_exit_code $? "Download PSM3 failed"

    if [[ -d ${OFI_SRC_DIR} ]]
    then
        rm -rf ${OFI_SRC_DIR}
    fi
    git clone --branch ${OFI_BRANCH} --single-branch ${OFI_LINK} ${OFI_SRC_DIR}
    check_exit_code $? "Download OFI failed"
}

install_ofi() {
    if [[ ${INSTALL_OFI} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${PSM3_SRC_DIR} ]] || [[ ! -d ${OFI_SRC_DIR} ]]
    then
        echo_log "ERROR: PSM3_SRC_DIR (${PSM3_SRC_DIR}) or OFI_SRC_DIR (${OFI_SRC_DIR}) is not directory, try to run script with \"-download_ofi 1\""
        check_exit_code 1 "Install OFI failed"
    fi

    cd ${PSM3_SRC_DIR}

    ./configure --prefix=${PSM3_INSTALL_DIR} --without-psm3-rv
    check_exit_code $? "Configure PSM3 failed"

    make -j && make install
    check_exit_code $? "Install PSM3 failed"


    cd ${OFI_SRC_DIR}

    ./autogen.sh
    check_exit_code $? "Autogen OFI failed"

    ./configure --prefix=${OFI_INSTALL_DIR} --enable-atomics=no \
        --enable-rxm --enable-shm --enable-sockets --enable-tcp --enable-verbs \
        --disable-bgq --disable-efa --disable-gni --disable-hook_debug --disable-mrail \
        --disable-perf --disable-psm --disable-psm2 --disable-psm3 --disable-rxd \
        --disable-rstream --disable-udp --disable-usnic --disable-xpmem
    # --enable-debug
    check_exit_code $? "Configure OFI failed"

    make -j all && make install
    check_exit_code $? "Install OFI failed"

    export LD_LIBRARY_PATH=${OFI_INSTALL_DIR}/lib:${OFI_INSTALL_DIR}/lib64:${LD_LIBRARY_PATH}
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
    git clone --branch ${CCL_BRANCH} --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
    check_exit_code $? "Download CCL failed"
}

install_ccl() {
    if [[ ${INSTALL_CCL} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo_log "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-download_ccl 1\""
        check_exit_code 1 "Install CCL failed"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build

    # intentionally build with MPI support
    # to get MPI launcher in CCL install dir to be used for test launches below
    cmake .. -DBUILD_FT=0 -DBUILD_EXAMPLES=0 -DBUILD_CONFIG=0 -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR} \
        -DENABLE_MPI=1 -DENABLE_MPI_TESTS=0 -DLIBFABRIC_DIR=${OFI_INSTALL_DIR}
    check_exit_code $? "Configure CCL failed"

    make -j install
    check_exit_code $? "Install CCL failed"
}

download_ucc() {
    if [[ ${DOWNLOAD_UCC} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ -d ${UCX_SRC_DIR} ]]
    then
        rm -rf ${UCX_SRC_DIR}
    fi
    git clone --branch ${UCX_BRANCH} --single-branch ${UCX_LINK} ${UCX_SRC_DIR}
    check_exit_code $? "Download UCX failed"

    if [[ -d ${UCC_SRC_DIR} ]]
    then
        rm -rf ${UCC_SRC_DIR}
    fi
    GIT_ASKPASS=${PATH_TO_TOKEN_FILE_1S} git clone --branch ${UCC_BRANCH} --single-branch \
        https://${USERNAME_1S}${UCC_BASE_LINK} ${UCC_SRC_DIR}
    check_exit_code $? "Download UCC failed"
}

install_ucc() {
    if [[ ${INSTALL_UCC} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${UCX_SRC_DIR} ]] || [[ ! -d ${UCC_SRC_DIR} ]]
    then
        echo_log "ERROR: UCX_SRC_DIR (${UCX_SRC_DIR}) or UCC_SRC_DIR (${UCC_SRC_DIR}) is not directory, try to run script with \"-download_ucc 1\""
        check_exit_code 1 "Install UCC failed"
    fi

    cd ${UCX_SRC_DIR}

    ./autogen.sh
    check_exit_code $? "Autogen UCX failed"

    ./configure --prefix=${UCX_INSTALL_DIR} --enable-mt=yes --enable-gtest=no
    check_exit_code $? "Configure UCX failed"

    make -j install
    check_exit_code $? "Install UCX failed"


    cd ${UCC_SRC_DIR}

    ./autogen.sh
    check_exit_code $? "Autogen UCC failed"

    ccl_vars_file="${CCL_INSTALL_DIR}/env/setvars.sh"
    source ${ccl_vars_file}

    ./configure --prefix=${UCC_INSTALL_DIR} --with-ucx=${UCX_INSTALL_DIR} \
        --with-ccl=${CCL_INSTALL_DIR} --with-mpi=${CCL_INSTALL_DIR}
    check_exit_code $? "Configure UCC failed"

    make -j install
    check_exit_code $? "Install UCC failed"
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

    if [[ -d "${CONDA_INSTALL_DIR}" ]]
    then
        if [[ -f "${CONDA_INSTALL_DIR}/etc/profile.d/conda.sh" ]]
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
    https_proxy=${PROXY} python -m pip install --upgrade pip

    echo_log "\n=== CONDA_PREFIX ${CONDA_PREFIX} ===\n"

    deactivate_conda
}

activate_conda() {
    echo_log "\n=== activate conda env ${CONDA_ENV_NAME} ===\n"

    if [[ -d "${CONDA_INSTALL_DIR}" ]]
    then
        CONDA_BIN_DIR="${CONDA_INSTALL_DIR}/bin"
    else
        CONDA_BIN_DIR=$(dirname $(which python))
    fi

    activate_script="${CONDA_BIN_DIR}/activate"

    echo_log "CONDA_BIN_DIR = ${CONDA_BIN_DIR}"

    if [[ ! -f ${activate_script} ]]
    then
        echo_log "ERROR: activate_script (${activate_script}) is not a file, try to run script with \"-download_conda 1 -create_conda 1\""
        check_exit_code 1 "Install CCL failed"
    fi

    source ${activate_script} ${CONDA_ENV_NAME}
    https_proxy=${PROXY} conda install -y pip
    conda config --set offline false
    check_exit_code $? "Install pip failed"

    echo_log "PYTHON = $(which python)"
    echo_log "PIP = $(which pip)"
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

    if [[ -d "${CONDA_INSTALL_DIR}" ]]
    then
        rm -rf ${CONDA_INSTALL_DIR}
    fi
}

install_pt() {
    if [[ ${INSTALL_PT} != "1" ]]
    then
        return
    fi

    https_proxy=${PROXY} conda install -y pytorch torchvision torchaudio cpuonly -c pytorch
    check_exit_code $? "PT install failed"
}

check_pt() {
    echo_log "================ Basic PT test ================"
    cmd="python -c \"import torch; print(torch.__version__)\""
    echo_log ${cmd}
    eval ${cmd}
    check_exit_code $? "Basic PT test failed"
    echo_log "================ ************* ================"
}

download_torch_ucc() {
    if [[ ${DOWNLOAD_TORCH_UCC} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}

    if [[ -d ${TORCH_UCC_SRC_DIR} ]]
    then
        rm -rf ${TORCH_UCC_SRC_DIR}
    fi
    git clone --branch ${TORCH_UCC_BRANCH} --single-branch ${TORCH_UCC_LINK} ${TORCH_UCC_SRC_DIR}
    check_exit_code $? "Download torch-ucc failed"
}

install_torch_ucc() {
    if [[ ${INSTALL_TORCH_UCC} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${TORCH_UCC_SRC_DIR} ]]
    then
        echo_log "ERROR: TORCH_UCC_SRC_DIR (${TORCH_UCC_SRC_DIR}) is not directory, try to run script with \"-download_torch_ucc 1\""
        check_exit_code 1 "Install torch-ucc failed"
    fi

    cd ${TORCH_UCC_SRC_DIR}

    UCX_HOME=${UCX_INSTALL_DIR} UCC_HOME=${UCC_INSTALL_DIR} python setup.py clean --all install
    check_exit_code $? "Install torch-ucc failed"
}

check_torch_ucc() {
    echo_log "================ Basic torch-ucc test ================"
    export LD_LIBRARY_PATH=${UCX_INSTALL_DIR}/lib:${UCC_INSTALL_DIR}/lib:${LD_LIBRARY_PATH}
    cmd="python -c \"import torch; import torch_ucc;\""
    echo_log ${cmd}
    eval ${cmd}
    check_exit_code $? "Basic torch-ucc test failed"
    echo_log "================ ****************** ================"
}

run_tests() {
    if [[ ${RUN_TESTS} != "1" ]]
    then
        return
    fi

    cd ${SCRIPT_WORK_DIR}

    echo "================ UCC test ================" | tee -a ${TESTS_LOG_FILE}
    cd ${UCC_SRC_DIR}/test/mpi
    make
    check_exit_code $? "Build UCC test failed"
    cmd="mpirun -n 2 ./ucc_test_mpi --colls allgather,allgatherv,allreduce,alltoall,alltoallv,bcast \
        --teams world --dtypes float32 --ops sum --inplace 2 --msgsize 64:1024 2>&1 | tee -a ${TESTS_LOG_FILE}"
    echo_log "exec cmd: $cmd"
    eval ${cmd}
    check_exit_code $? "Run UCC test failed"
    echo "================ ****************** ================" | tee -a ${TESTS_LOG_FILE}

    echo "================ torch-ucc test ================" | tee -a ${TESTS_LOG_FILE}
    cd ${TORCH_UCC_SRC_DIR}/test
    chmod +x start_test.sh
    cmd="./start_test.sh torch_alltoall_bench.py --backend ucc --max-size 1024 --iter 2 2>&1 | tee -a ${TESTS_LOG_FILE}"
    echo_log "exec cmd: $cmd"
    eval ${cmd}
    check_exit_code $? "Run torch-ucc test failed"
    echo "================ ****************** ================" | tee -a ${TESTS_LOG_FILE}

    echo "================ PARAM test ================" | tee -a ${TESTS_LOG_FILE}
    if [[ -d ${PARAM_SRC_DIR} ]]
    then
        rm -rf ${PARAM_SRC_DIR}
    fi
    cmd="git clone --branch ${PARAM_BRANCH} --single-branch ${PARAM_LINK} ${PARAM_SRC_DIR}"
    echo_log "exec cmd: $cmd"
    eval ${cmd}
    check_exit_code $? "Download PARAM failed"

    cd ${PARAM_SRC_DIR}/train/comms/pt
    echo "I_MPI_HYDRA_HOST_FILE ${I_MPI_HYDRA_HOST_FILE}"
    first_host="localhost"
    if [[ -f ${I_MPI_HYDRA_HOST_FILE} ]]
    then
        first_host=`cat ${I_MPI_HYDRA_HOST_FILE} | head -n 1`
    fi
    echo "first_host: ${first_host}"

    master_ip_value=`ssh ${first_host} ifconfig | grep "inet " | head -n 1 | awk '{print $2}'`
    echo_log "master_ip: ${master_ip_value}"

    MASTER_IP="--master-ip ${master_ip_value}"
    N="-n 2"
    PPN="-ppn 1"

    cmd="mpirun ${N} ${PPN} python ./comms.py ${MASTER_IP} \
        --b 8 --e 8M --n 64 --f 2 --z 1 \
        --collective all_reduce --backend ucc --log INFO 2>&1 | tee -a ${TESTS_LOG_FILE}"
    echo_log "exec cmd: $cmd"
    eval ${cmd}
    check_exit_code $? "Run PARAM failed"
    echo "================ ****************** ================" | tee -a ${TESTS_LOG_FILE}

    fail_pattern="abort|^bad$|corrupt|^fault$|invalid|kill|runtime_error|terminate|timed|unexpected"
    ignore_fail_pattern="UCX  DEBUG"

    fail_strings=`grep -E -i "${fail_pattern}" ${TESTS_LOG_FILE}`
    if [[ ! -z ${ignore_fail_pattern} ]]
    then
        fail_strings=`echo "${fail_strings}" | grep -E -i -v "${ignore_fail_pattern}"`
    fi
    fail_count=`echo "${fail_strings}" | sed '/^$/d' | wc -l`

    if [[ ${fail_count} -ne 0 ]]
    then
        echo_log ""
        echo_log "fail strings:"
        echo_log ""
        echo_log "${fail_strings}"
        echo_log ""
        echo_log "see full log ${TESTS_LOG_FILE} for details"
        echo_log ""
        echo_log "TEST FAILED"
        check_exit_code ${fail_count} "TEST FAILED"
    else
        echo_log "TEST PASSED"
    fi
}

parse_arguments $@

check_base_env

download_ofi
install_ofi

download_ccl
install_ccl

download_ucc
install_ucc

download_conda
create_conda
activate_conda

install_pt
check_pt

download_torch_ucc
install_torch_ucc
check_torch_ucc

set_run_env

run_tests

deactivate_conda
remove_conda
