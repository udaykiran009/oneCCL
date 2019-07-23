#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
else
    echo "SCRIPT_DIR: ${SCRIPT_DIR}"
fi

. ${SCRIPT_DIR}/settings.sh
. ${COMPILER_PATH}/linux/bin/compilervars.sh intel64

BASENAME=`basename $0 .sh`
LOG_FILE="${LOG_DIR}/${BASENAME}_$$.log"
WORK_DIR=`cd ${SCRIPT_DIR}/../.. && pwd -P`
KW_DIR="${WORK_DIR}"
KW_TBL_DIR="${KW_DIR}/kwtables"
KW_BUILD_SPEC_FILE="${WORK_DIR}/build.out"
SRC_DIR="${KW_DIR}"

function Print() {
    echo $* | tee -a ${LOG_FILE}
}

function PrinUsage() {
    echo "Usage:"
    echo "$0 -project=[impi|itac|mps|ccl|spark]"
}

while [ $# -ne 0 ]
do
    case $1 in
        -project=*)
            PROJECT=${1#*=}
            ;;
        *)
            echo "ERROR: unrecognized option ($1)"
            PrinUsage
            exit 1
            ;;
    esac
    
    shift
done

case "$PROJECT" in
    "impi")
        BUILD_DIR="${SRC_DIR}/build"
        # BUILD_CMD="./build_product_locally -build all"
        BUILD_CMD="./build_product_locally -build release_mt,release"
        # BUILD_CMD="./build_product_locally -build release_mt"
        # BUILD_CMD="./build_product_locally -build product"
        KW_PROJ_NAME="${PROJECT}_lin"
        ;;
    "itac")
        export PATH="/p/pdsd/opt/EM64T-LIN/software/python2.7.5/bin:$PATH"
        BUILD_DIR="${SRC_DIR}"
        BUILD_CMD="scons config=release -j1 collector.VT_shared"
        # BUILD_CMD="scons ENABLE_FLEXLM=yes config=release -j8 all"
        KW_PROJ_NAME="${PROJECT}_lin"
        ;;
    "mps")
        export PATH="/p/pdsd/opt/EM64T-LIN/software/python2.7.5/bin:$PATH"
        BUILD_DIR="${SRC_DIR}"
        BUILD_CMD="scons config=release -j1 mps::"
        KW_PROJ_NAME="${PROJECT}"
        ;;
    "ccl")
        BUILD_DIR="${SRC_DIR}/scripts/jenkins"
        BUILD_CMD="./build.sh"
        KW_PROJ_NAME="dlmsl"
        ;;
    "spark")
        BUILD_DIR="${SRC_DIR}/jni/ofi"
        BUILD_CMD="make JAVA_HOME=/p/pdsd/Intel_MPI/Software/java/jdk/latest OFI_HOME=${LIBFABRIC_DIR}"
        KW_PROJ_NAME="${PROJECT}"
        ;;
    *)
        echo "ERROR: unrecognized or empty project name ($1)"
        PrinUsage
        exit 1
        ;;
esac

# preliminary operations
rm -f ${LOG_FILE}
# mkdir -p ${WORK_DIR}
mkdir -p ${LOG_DIR}
mkdir -p ${TMP_DIR}
mkdir -p ${SRC_DIR}
mkdir -p ${KW_TBL_DIR}

echo "###############################################################################"
echo "#                           KW Scanning                                       #"
echo "###############################################################################"

Print -n "Generating KW build specification... "
rm -f ${KW_BUILD_SPEC_FILE}
cd ${BUILD_DIR}

# W/A for MPS
if [ "$PROJECT" = "itac" ]
then
    mkdir -p _install/intel64/include
fi

kwinject --output ${KW_BUILD_SPEC_FILE} ${BUILD_CMD} | tee -a ${LOG_FILE} 2>&1
if [ $? -ne 0 ]
then
    Print "NOK"
    exit 1
else
    Print "OK"
fi

Print -n "Running integration build analysis... "
rm -rf ${KW_TBL_DIR}/${KW_PROJ_NAME}_tbl
kwbuildproject --add-analysis-options "--lef-planner-host localhost --lef-planner-port 55555" ${KW_BUILD_SPEC_FILE} -tables-directory ${KW_TBL_DIR}/${KW_PROJ_NAME}_tbl --url https://${KW_SERVER_HOST}:${KW_SERVER_PORT}/${KW_PROJ_NAME} | tee -a ${LOG_FILE} 2>&1
if [ $? -ne 0 ]
then
    Print "NOK"
    exit 1
else
    Print "OK"

    if [ "$NEED_UPLOAD_RES_TO_KW_SERVER" = "yes" ]
    then
        Print -n "Unloading scanning results to the KW server... "
        kwadmin --url https://${KW_SERVER_HOST}:${KW_SERVER_PORT} load ${KW_PROJ_NAME} ${KW_TBL_DIR}/${KW_PROJ_NAME}_tbl | tee -a ${LOG_FILE} 2>&1
        if [ $? -ne 0 ]
        then
            Print "NOK"
            exit 1
        else
            Print "OK"
        fi
    fi
fi
