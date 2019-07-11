#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z "${SCRIPT_DIR}" ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

SRC_ROOT_DIR=`cd ${SCRIPT_DIR}/ && pwd -P`
INTEL_DIR="${SRC_ROOT_DIR}/"
DOC_DIR="${INTEL_DIR}/doc"

if [ "$1" = "win" ]; then
    # Windows
    # TODO
    :
else
    # Linux
    BOM_COMPONENT="<deliverydir>/l_doc"
    DEST_BOM_FILE=${INTEL_DIR}/l_doc_auto.txt
    SRC_PATH="<iccl_root>/doc"
    INSTALL_PATH="<installdir><l_iccl_install_path><l_iccl_platform>/doc"
    CONTENT=`find ${DOC_DIR} -type f | sort -u`""
fi

echo "DeliveryName	InstallName	FileCheckSum	FileOrigin	Owner	FileFeature	FileInstallCondition	InstalledFilePermission	Redistributable	InstalledFileOwner	SymbolicLink" > ${DEST_BOM_FILE}

for FILE in ${CONTENT}
do
    FILE=`echo ${FILE} | sed -e "s|${DOC_DIR}/||"`
    echo "${BOM_COMPONENT}/${FILE}	${INSTALL_PATH}/${FILE}		internal	Yury Kiryanov	Documentation	1	644	nonredistributable	root	${SRC_PATH}/${FILE}" >> ${DEST_BOM_FILE}
done

echo "#Intel Confidential" >> ${DEST_BOM_FILE}
