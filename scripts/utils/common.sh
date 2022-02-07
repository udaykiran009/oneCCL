function install_oneccl_package()
{
    DEST_DIR=${1}

    if [[ ! -d ${DEST_DIR} ]]
    then
        echo "ERROR: missing argument"
        exit 1
    fi

    while true
    do
        if [[ -f ${DEST_DIR}/install_${build_type}.lock ]]
        then
            echo "INFO: The package is already in the installation process..."
            sleep 30
        else
            break
        fi
    done

    # Download package
    ls ${DEST_DIR} | grep l_ccl_${build_type}.*.tgz
    if [[ $? = 0 ]]
    then
        echo "INFO: Archive is already downloaded"
    else
        scp -i ~/.ssh/id_rsa ${USERNAME}@${REMOTE_HOST}:${ARTEFACT_DIR}/l_ccl_${build_type}*.tgz ${DEST_DIR}
    fi

    # Install package
    if [[ $(ls -d ${DEST_DIR}/*/ | grep l_ccl_${build_type}) ]]
    then
        echo "INFO: Package is already installed"
    else
        touch ${DEST_DIR}/install_${build_type}.lock
        PACKAGE_NAME=$(ls ${DEST_DIR}/ | grep l_ccl_${build_type})
        PACKAGE_NAME=${PACKAGE_NAME%.*}
        mkdir ${DEST_DIR}/tmp
        tar xfz ${DEST_DIR}/${PACKAGE_NAME}.tgz --directory ${DEST_DIR}/tmp
        pushd ${DEST_DIR}/tmp/${PACKAGE_NAME}
        unzip -P accept *.zip
        ${DEST_DIR}/tmp/${PACKAGE_NAME}/install.sh -s -d ${DEST_DIR}/${PACKAGE_NAME}
        popd
        rm -rf ${DEST_DIR}/install_${build_type}.lock ${DEST_DIR}/tmp
    fi
}

function check_command_exit_code() 
{
    local arg=$1
    local err_msg=$2

    if [[ $arg -ne 0 ]]
    then
        echo_log "ERROR: ${err_msg}" 1>&2
        exit $1
    fi
}

function echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

function run_cmd()
{
    echo_log "\n${1}\n"
    eval ${1} 2>&1 | tee -a ${LOG_FILE}
    check_command_exit_code $? "cmd failed"
}

function run_test_cmd()
{
    echo_log "\nTest command: ${1}\n"
    eval ${1}
}

function echo_log_separator()
{
    echo_log "#=============================================================================="
}

function is_gpu_node()
{
    if [[ ${node_label} == "ccl_test_gen9" ]] || [[ ${node_label} == "ccl_test_ats" ]] || [[ ${node_label} == "ccl_test_pvc" ]]
    then
        export IS_GPU_NODE="yes"
    fi
}

function set_tests_option()
{
    local option="${1}"
    local current_scope="${2}"
    local option_name=${option%=*}
    local pattern=$(echo ${current_scope} | grep -oE "${option_name}[^ ]*")
    if [[ -z ${pattern} ]]
    then
        current_scope="${current_scope} ${option}"
    else
        current_scope=${current_scope/${pattern}/${option}}
    fi
    echo ${current_scope}
}

function set_gnu_compiler()
{
    if [[ -z "${GNU_BUNDLE_ROOT}" ]]
    then
        GNU_BUNDLE_ROOT="/usr/bin"
    fi
    C_COMPILER=${GNU_BUNDLE_ROOT}/gcc
    CXX_COMPILER=${GNU_BUNDLE_ROOT}/g++
}

function set_intel_compiler()
{
    if [[ -z "${INTEL_BUNDLE_ROOT}" ]]
    then
        INTEL_BUNDLE_ROOT="/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/"
    fi
    source ${INTEL_BUNDLE_ROOT}/linux/bin/compilervars.sh intel64
    C_COMPILER="${INTEL_BUNDLE_ROOT}/linux/bin/intel64/icc"
    CXX_COMPILER="${INTEL_BUNDLE_ROOT}/linux/bin/intel64/icpc"
}

function set_icx_compiler()
{
    if [[ -z "${ICX_BUNDLE_ROOT}" ]]
    then
        ICX_BUNDLE_ROOT="${CCL_ONEAPI_DIR}/compiler/last/"
    fi
    source ${ICX_BUNDLE_ROOT}/setvars.sh
    C_COMPILER="${ICX_BUNDLE_ROOT}/compiler/latest/linux/bin/icx"
    CXX_COMPILER="${ICX_BUNDLE_ROOT}/compiler/latest/linux/bin/icpx"
}

function set_dpcpp_compiler()
{
    if [[ -z "${SYCL_BUNDLE_ROOT}" ]]
    then
        SYCL_BUNDLE_ROOT="${CCL_ONEAPI_DIR}/compiler/last/"
    fi
    if [[ ${node_label} = "ccl_test_pvc" ]]
    then
        if [[ ! -d ${SYCL_BUNDLE_ROOT}/modulefiles ]]
        then
            ${SYCL_BUNDLE_ROOT}/modulefiles-setup.sh
        fi
        module use ${SYCL_BUNDLE_ROOT}/modulefiles
        module load compiler/latest
    else
        source ${SYCL_BUNDLE_ROOT}/setvars.sh
    fi
    C_COMPILER="${SYCL_BUNDLE_ROOT}/compiler/latest/linux/bin/icx"
    CXX_COMPILER="${SYCL_BUNDLE_ROOT}/compiler/latest/linux/bin/dpcpp"
    COMPUTE_BACKEND="dpcpp"
}

function set_impi_environment()
{
    if [[ ${ENABLE_MPICH_OFI_TESTS} = "yes" ]]
    then
        return
    fi

    if [[ -z "${I_MPI_HYDRA_HOST_FILE}" ]]
    then
        if [ -f ${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts ]
        then
            export I_MPI_HYDRA_HOST_FILE=${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
        else
            echo "WARNING: hostfile (${CURRENT_WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts) isn't available"
            echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set"
        fi
    fi

    if [[ -z "${IMPI_PATH}" ]]
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

function clone_repo()
{
    name=$1
    link=$2
    branch=$3
    output_dir=$4
    extra_env=$5

    echo_log "\nclone repo"
    echo_log "name: ${name}"
    echo_log "link: ${link}"
    echo_log "branch: ${branch}"
    echo_log "output_dir: ${output_dir}"
    echo_log "extra_env: ${extra_env}\n"

    cmd="${extra_env} git clone --single-branch --branch ${branch} ${link} ${output_dir}"
    echo_log ${cmd}
    eval ${cmd}
    CheckCommandExitCode $? "git clone for repo ${name} (${repo}) failed"

    echo_log "\nrepo: ${name} (${link})\n"
    pushd ${output_dir}
    echo_log "`git log -1`\n"
    popd
}

download_conda() {
    if [[ ${DOWNLOAD_CONDA} != "1" ]]
    then
        return
    fi

    pushd "${SCRIPT_WORK_DIR}"
    CONDA_FILENAME="conda.sh"
    wget -O ${CONDA_FILENAME} ${CONDA_LINK}
    chmod +x ${CONDA_FILENAME}
    ./${CONDA_FILENAME} -b -p ${CONDA_INSTALL_DIR}
    export CONDA_ENVS_PATH="${CONDA_INSTALL_DIR}/envs"
    export CONDA_PKGS_DIRS="${CONDA_INSTALL_DIR}/pkgs"
    export PIP_CACHE_DIR="${CONDA_INSTALL_DIR}/pip"
    mkdir -p "${PIP_CACHE_DIR}"
    popd 
}

create_conda() {
    if [[ ${CREATE_CONDA} != "1" ]]
    then
        return
    fi

    conda_packages=${1}
    echo_log "conda_packages: ${conda_packages}"

    pushd ${SCRIPT_WORK_DIR}

    if [ -d "${CONDA_INSTALL_DIR}" ]
    then
        if [ -f "${CONDA_INSTALL_DIR}/etc/profile.d/conda.sh" ]
        then
            source "${CONDA_INSTALL_DIR}/etc/profile.d/conda.sh"
        else
            export PATH="${CONDA_INSTALL_DIR}/bin:$PATH"
        fi
    fi

    echo_log "which conda: `which conda`"
    echo_log "\n=== create conda env ===\n"
    env_count=`conda env list | grep "${CONDA_ENV_NAME} " | wc -l`
    if [[ $env_count -ne 0 ]]
    then
        echo_log "found conda env ${CONDA_ENV_NAME}"
    else
        echo_log "didn't find conda env ${CONDA_ENV_NAME}, create new one"
        https_proxy=${PROXY} conda create -y -n ${CONDA_ENV_NAME} ${conda_packages}
    fi

    activate_conda

    echo_log "\n=== install packages in conda env ${CONDA_ENV_NAME} ===\n"
    python -m pip install --upgrade pip

    echo_log "\n=== CONDA_PREFIX ${CONDA_PREFIX} ===\n"

    deactivate_conda

    popd || exit 1
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

    echo_log "CONDA_BIN_DIR = ${CONDA_BIN_DIR}"

    if [[ ! -f ${activate_script} ]]
    then
        echo "ERROR: activate_script (${activate_script}) is not a file, try to run script with \"-download_conda 1 -create_conda 1\""
        CheckCommandExitCode 1 "Install CCL failed"
    fi

    source ${activate_script} ${CONDA_ENV_NAME}
    https_proxy=${PROXY} conda install -y pip
    
    echo_log "python = $(which python)"
    echo_log "pip    = $(which pip)"
}

deactivate_conda() {
    echo_log "\n=== deactivate conda env ${CONDA_ENV_NAME} ===\n"
    conda deactivate
}

set_extra_proxy() {
    if [[ ${SET_EXTRA_PROXY} = "0" ]]
    then
        return
    fi

    export http_proxy=http://proxy-dmz.intel.com:911
    export https_proxy=http://proxy-dmz.intel.com:912
    export ftp_proxy=http://proxy-dmz.intel.com:911
    export socks_proxy=http://proxy-dmz.intel.com:1080
    export no_proxy=intel.com,.intel.com,localhost,127.0.0.1
}
