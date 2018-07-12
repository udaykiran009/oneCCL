#!/bin/sh

echo "DEBUG: GIT_BRANCH = ${GIT_BRANCH}"
echo "DEBUG: BUILD_ID = ${BUILD_ID}"
echo "DEBUG: coverage = ${coverage}"
echo "DEBUG: archive = ${archive}"
echo "DEBUG: rpm = ${rpm}"

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

. ${SCRIPT_DIR}/settings.sh
. ${COMPILER_PATH}/linux/bin/compilervars.sh intel64

BASENAME=`basename $0 .sh`
WORK_DIR=`cd ${SCRIPT_DIR}/../.. && pwd -P`
export COMPILER_OPTION="COMPILER=intel"
export $MAKE_OPTION
echo $MAKE_OPTION
# 'release' is set by Jenkins
if [ -z "${release}" ]
then
    release="false"
fi

# 'archive' is set by Jenkins
if [ "$archive" = "true" ] && [ "${release}" != "true" ]
then
    export MLSL_ARCHIVE_SUFFIX=_`date +%Y%m%d.%H%M%S`
fi

cd ${WORK_DIR}

echo "Building DL_MSL..."
MAKE_STAGES="clean"

# 'coverage' is set by Jenkins
if [ "$coverage" = "true" ]
then
    export CODECOV=1
    
    if [ -z "${BUILD_ID}" ]
    then
        TMP_COVERAGE_DIR=${WORK_DIR}/coverage
    else
        TMP_COVERAGE_DIR=${MLSL_REPO_DIR}/${BUILD_ID}/coverage
    fi
    
    rm -rf ${TMP_COVERAGE_DIR}
    mkdir -p ${TMP_COVERAGE_DIR}
fi
MAKE_STAGES=$MAKE_STAGES" all"

# 'rpm' is set by Jenkins
if [ "$rpm" = "true" ]
then
    MAKE_STAGES=$MAKE_STAGES" rpm"
fi

# 'deb' is set by Jenkins
if [ "$deb" = "true" ]
then
    MAKE_STAGES=$MAKE_STAGES" deb"
fi

# 'archive' is set by Jenkins
if [ "$archive" = "true" ]
then
    MAKE_STAGES=$MAKE_STAGES" archive"
fi

# 'release' is set by Jenkins
if [ "$release" = "true" ]
then
    MAKE_STAGES=$MAKE_STAGES" publish_to_swf"
fi

for STAGE in $MAKE_STAGES
do
    echo "make "$STAGE"..."
    make $COMPILER_OPTION $MAKE_OPTION $STAGE
    if [ $? -ne 0 ]
    then
        echo "make "$STAGE"... NOK"
        exit 1
    else
        echo "make "$STAGE"... OK"
    fi
done

# 'coverage' is set by Jenkins
if [ "$coverage" = "true" ]
then
    # Move temporary files necessary for coverage measurements
    cp ${WORK_DIR}/pgopti.spi ${TMP_COVERAGE_DIR}/
fi

if [ -n "${BUILD_ID}" ]
then
    MLSL_ARCHIVE_NAME=`cd ${WORK_DIR} && ls -1 *.tgz 2>/dev/null`
    mkdir -p ${MLSL_REPO_DIR}/${BUILD_ID}
    
    if [ -n "${MLSL_ARCHIVE_NAME}" ]
    then
        cp ${WORK_DIR}/${MLSL_ARCHIVE_NAME} ${MLSL_REPO_DIR}/${BUILD_ID}/
        echo "DEBUG: MLSL build = ${MLSL_REPO_DIR}/${BUILD_ID}/${MLSL_ARCHIVE_NAME}"
    else
        echo "WARNING: no tgz files in ${WORK_DIR}"
    fi
fi
echo "Building DL_MSL... OK"
