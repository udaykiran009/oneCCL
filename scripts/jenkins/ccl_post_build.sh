#!/bin/bash -xe

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

echo "DEBUG: BUILD_USER = ${BUILD_USER}"
echo "DEBUG: BUILD_USER_FIRST_NAME = ${BUILD_USER_FIRST_NAME}"
echo "DEBUG: BUILD_USER_LAST_NAME = ${BUILD_USER_LAST_NAME}"
echo "DEBUG: BUILD_USER_ID = ${BUILD_USER_ID}"
echo "DEBUG: BUILD_USER_EMAIL = ${BUILD_USER_EMAIL}"
echo "DEBUG: MAKE_OPTION = ${MAKE_OPTION}"
echo "DEBUG: TESTING_ENVIRONMENT = ${TESTING_ENVIRONMENT}"

ARTEFACT_DIR=$ARTEFACT_ROOT_DIR/$BUILDER_NAME/$MLSL_BUILD_ID
echo "DEBUG: ARTEFACT_DIR = ${ARTEFACT_DIR}"

#debug
export CCL_PACKAGE_PREFIX=l_ccl_debug_
tar xf ${ARTEFACT_DIR}/build_gpu_debug.tar
tar xf ${ARTEFACT_DIR}/build_debug.tar

source ${WORKSPACE}/scripts/jenkins/fake_container.sh

run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install
CheckCommandExitCode $? "post build and pack for debug failed"
mv ${CCL_PACKAGE_PREFIX}*.tgz ${ARTEFACT_DIR}
mv ${CCL_PACKAGE_PREFIX}* ${ARTEFACT_DIR}


rm -rf ${WORKSPACE}/*

#release
tar xfz ${ARTEFACT_DIR}/ccl_src.tgz

export CCL_PACKAGE_PREFIX=l_ccl_release_
tar xf ${ARTEFACT_DIR}/build_gpu_release.tar
tar xf ${ARTEFACT_DIR}/build_release.tar

if [ "$ENABLE_PRE_DROP_STAGE" == "true" ]
then
    run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install --test-pre-drop --swf-pre-drop
    CheckCommandExitCode $? "post build and pack failed"
else
    run_in_fake_container /build/ccl/scripts/build.sh ${BUILD_OPTIONS} --post-build --pack --install --test-pre-drop
    CheckCommandExitCode $? "build.sh failed"
fi

tar -zcf buildspace_$CCL_PACKAGE_PREFIX.tgz --exclude buildspace_$CCL_PACKAGE_PREFIX.tgz ./*

mv buildspace_$CCL_PACKAGE_PREFIX.tgz ${ARTEFACT_DIR}

cd ${ARTEFACT_DIR}
tar -xzf buildspace_$CCL_PACKAGE_PREFIX.tgz
CheckCommandExitCode $? "post build failed"

if [ ${BUILDER_NAME} == "ccl-nightly" ]
then
    if [ -d $ARTEFACT_ROOT_DIR/$BUILDER_NAME/last ]
    then
        rm -d $ARTEFACT_ROOT_DIR/$BUILDER_NAME/last
    fi
    ln -s ${ARTEFACT_DIR} $ARTEFACT_ROOT_DIR/$BUILDER_NAME/last
fi
