#!/bin/bash -xe

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

ARTEFACT_DIR="${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
PVC_WORKSPACE_DIR="/home/sys_ctlab/workspace/workspace/"
PVC_ARTEFACT_DIR="${PVC_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"

ATS_WORKSPACE_DIR="/home/sys_ctlab/jenkins/workspace/workspace/"
ATS_ARTEFACT_DIR="${ATS_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"

echo "DEBUG: ARTEFACT_DIR          = ${ARTEFACT_DIR}"
echo "DEBUG: ENABLE_PRE_DROP_STAGE = ${ENABLE_PRE_DROP_STAGE}"
echo "DEBUG: ENABLE_NIGHTLY_DROP   = ${ENABLE_NIGHTLY_DROP}"
echo "DEBUG: CCL_PACKAGE_PHASE     = ${CCL_PACKAGE_PHASE}"
echo "DEBUG: ARTEFACT_DIR          = ${ARTEFACT_DIR}"
echo "DEBUG: PVC_ARTEFACT_DIR      = ${PVC_ARTEFACT_DIR}"
echo "DEBUG: ATS_ARTEFACT_DIR      = ${ATS_ARTEFACT_DIR}"


#debug
export CCL_PACKAGE_PREFIX=l_ccl_debug_
tar xf ${ARTEFACT_DIR}/build_gpu_debug.tgz
tar xf ${ARTEFACT_DIR}/build_cpu_icx_debug.tgz

source ${WORKSPACE}/scripts/jenkins/fake_container.sh

run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install
CheckCommandExitCode $? "post build and pack for debug failed"
mv ${CCL_PACKAGE_PREFIX}*.tgz ${ARTEFACT_DIR}
mv ${CCL_PACKAGE_PREFIX}* ${ARTEFACT_DIR}


rm -rf ${WORKSPACE}/*

#release
tar xfz ${ARTEFACT_DIR}/ccl_src.tgz

export CCL_PACKAGE_PREFIX=l_ccl_release_
tar xf ${ARTEFACT_DIR}/build_gpu_release.tgz
tar xf ${ARTEFACT_DIR}/build_cpu_icx_release.tgz

if [ "${ENABLE_PRE_DROP_STAGE}" == "true" ]
then
    run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install --test-pre-drop --swf-pre-drop
    CheckCommandExitCode $? "post build and pack failed"
else
    run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install --test-pre-drop
    CheckCommandExitCode $? "build.sh failed"
fi

mv ${CCL_PACKAGE_PREFIX}* ${ARTEFACT_DIR}
cp -r ${WORKSPACE}/_log ${ARTEFACT_DIR}
cp -r ${WORKSPACE}/_predrop ${ARTEFACT_DIR}

if [[ ${BUILDER_NAME} == "ccl-nightly" ]] || [[ ${BUILDER_NAME} == "ccl-weekly" ]]
then
    if [ -d ${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/last ]
    then
        rm -d ${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/last
    fi
    ln -s ${ARTEFACT_DIR} ${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/last
fi

if [[ ${TEST_CONFIGURATIONS} == *"mpich_pvc"* ]]
then
    source ~/.aurora_pvc
    scp ${ARTEFACT_DIR}/l_ccl_release*.tgz ${AURORA_PVC}:${PVC_ARTEFACT_DIR}
    CheckCommandExitCode $? "copying release packages failed"
    scp ${ARTEFACT_DIR}/l_ccl_debug*.tgz ${AURORA_PVC}:${PVC_ARTEFACT_DIR}
    CheckCommandExitCode $? "copying debug packages failed"
else
    source ~/.jfcst-xe
    scp ${ARTEFACT_DIR}/l_ccl_release*.tgz ${JFCST_ATS}:${ATS_ARTEFACT_DIR}
    CheckCommandExitCode $? "copying release packages failed"
    scp ${ARTEFACT_DIR}/l_ccl_debug*.tgz ${JFCST_ATS}:${ATS_ARTEFACT_DIR}
    CheckCommandExitCode $? "copying debug packages failed"
fi
