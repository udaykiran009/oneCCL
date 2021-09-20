install_oneccl_package()
{
    DEST_DIR=${1}

    if [[ ! -d ${DEST_DIR} ]]
    then
        echo "ERROR: missing argument"
        exit 1
    fi

    while true
    do
        if [[ -f ${DEST_DIR}/install.lock ]]
        then
            echo "INFO: The package is already in the installation process..."
            sleep 30
        else
            break
        fi
    done

    if [[ $(ls -d ${DEST_DIR}/*/ | grep l_ccl_${build_type} | wc -l) = 0 ]]
    then
        touch ${DEST_DIR}/install.lock
        scp -i ~/.ssh/id_rsa ${USERNAME}@${REMOTE_HOST}:${ARTEFACT_DIR}/l_ccl_${build_type}*.tgz ${DEST_DIR}

        PACKAGE_NAME=$(ls ${DEST_DIR}/ | grep l_ccl_${build_type})
        PACKAGE_NAME=${PACKAGE_NAME%.*}

        mkdir ${DEST_DIR}/tmp
        tar xfz ${DEST_DIR}/${PACKAGE_NAME}.tgz --directory ${DEST_DIR}/tmp
        pushd ${DEST_DIR}/tmp/${PACKAGE_NAME}
        unzip -P accept *.zip
        ${DEST_DIR}/tmp/${PACKAGE_NAME}/install.sh -s -d ${DEST_DIR}/${PACKAGE_NAME}
        popd

        rm -f ${DEST_DIR}/l_ccl_${build_type}*.tgz ${DEST_DIR}/install.lock
        rm -rf ${DEST_DIR}/tmp
    else
        echo "INFO: The package is already installed"
    fi

}
