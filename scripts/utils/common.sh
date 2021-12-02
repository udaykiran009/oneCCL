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
