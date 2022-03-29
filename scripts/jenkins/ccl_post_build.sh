#!/bin/bash -xe

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

ARTEFACT_DIR="${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}"

JFCST_XE="jfcst-xe.jf.intel.com /home/sys_ctlab/jenkins/workspace/workspace/"
JFCST_DEV="jfcst-dev.jf.intel.com /home2/sys_ctlab/jenkins_an/workspace/"
AURORA_PVC="a21-surrogate4.hpe.jf.intel.com /home/sys_ctlab/workspace/workspace/"

echo "DEBUG: ARTEFACT_DIR          = ${ARTEFACT_DIR}"
echo "DEBUG: ENABLE_PRE_DROP_STAGE = ${ENABLE_PRE_DROP_STAGE}"
echo "DEBUG: ENABLE_NIGHTLY_DROP   = ${ENABLE_NIGHTLY_DROP}"
echo "DEBUG: CCL_PACKAGE_PHASE     = ${CCL_PACKAGE_PHASE}"
echo "DEBUG: ARTEFACT_DIR          = ${ARTEFACT_DIR}"

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
    host=$(echo ${AURORA_PVC} | awk '{print $1}')
    workspace=$(echo ${AURORA_PVC} | awk '{print $2}')
    artefact_dir="${workspace}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
    for build_type in "release" "debug"
    do
        scp ${ARTEFACT_DIR}/l_ccl_${build_type}*.tgz ${host}:${artefact_dir}
        CheckCommandExitCode $? "ERROR: copying ${build_type} packages to ${host} failed"
    done
else
    for build_type in "release" "debug"
    do
        for cluster in "${JFCST_XE}" "${JFCST_DEV}"
        do
            host=$(echo ${cluster} | awk '{print $1}')
            workspace=$(echo ${cluster} | awk '{print $2}')
            artefact_dir="${workspace}/${BUILDER_NAME}/${MLSL_BUILD_ID}"
            scp ${ARTEFACT_DIR}/l_ccl_${build_type}*.tgz ${host}:${artefact_dir}
            CheckCommandExitCode $? "ERROR: copying ${build_type} packages to ${host} failed"
        done
    done
fi
