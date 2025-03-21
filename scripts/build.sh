#!/bin/bash
set -o pipefail

BASENAME=`basename $0 .sh`

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
RELEASED_BUILD_DIR="/p/pdsd/scratch/Uploads/RegularTesting/CCL/"
if [ -z "${SCRIPT_DIR}" ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

# Echo with logging. Always echo and always log
echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

# Cleanup temp dir
cleanup()
{
    if [[ -d "${TMP_MAKE_DIR}" ]]
    then
        rm -rf "${TMP_MAKE_DIR}"
    fi
}

trap cleanup EXIT

DATE=`date +%Y%m%d`
TIME=`date +%H%M%S`
ARCH=`uname -m`


WORKSPACE=`cd ${SCRIPT_DIR}/../ && pwd -P`

if [ "${ENABLE_CODECOV}" = "yes" ]
then
    CODECOV_FLAGS="true"
else
    CODECOV_FLAGS=""
fi

BOM_DIR="${WORKSPACE}/boms"
IMPI_DIR="${WORKSPACE}/deps/mpi"
HWLOC_DIR="${WORKSPACE}/deps/hwloc"
ITT_DIR="${WORKSPACE}/deps/itt"
BOM_LIST_NAME="bom_lists.txt"
TMP_DIR="${WORKSPACE}/_tmp"
TMP_MAKE_DIR=$(mktemp -d -t dpcpp-XXXXXXXXXX)
PACKAGE_DIR="${WORKSPACE}/_package"
# Files ready to be packaged as an engineering archive
PACKAGE_ENG_DIR="${PACKAGE_DIR}/eng"
LOG_DIR="${WORKSPACE}/_log"
INSTALL_DIR="${WORKSPACE}/_install"
LOG_FILE="${LOG_DIR}/${BASENAME}_${DATE}_${TIME}.log"

PACKAGE_LOG=${TMP_DIR}/package.log

mkdir -p ${LOG_DIR}
rm -f ${LOG_FILE}

HOSTNAME=`hostname -s`

CCL_VERSION_FORMAT="2021.8.0"

if [ -z "${CCL_PACKAGE_PREFIX}" ]
then
    CCL_PACKAGE_PREFIX="l_ccl_"
fi

if [ -z "${CCL_PACKAGE_PHASE}" ]
then
    CCL_PACKAGE_PHASE="ENG"
fi

if [[ $(date -d "$(date +%Y)-01-01" +%W) -gt 0 ]]
then
    CCL_PACKAGE_SUFFIX="_${CCL_PACKAGE_PHASE}_ww$(date +%W).${DATE}.${TIME}"
else
    CCL_PACKAGE_SUFFIX="_${CCL_PACKAGE_PHASE}_ww$(printf '%02d' $(($(date +%-W) + 1))).${DATE}.${TIME}"
fi

CCL_PACKAGE_NAME="${CCL_PACKAGE_PREFIX}_${CCL_VERSION_FORMAT}${CCL_PACKAGE_SUFFIX}"
SWF_PRE_DROP_ROOT_DIR="/p/pdsd/scratch/Drops/CCL/"
SWF_PRE_DROP_DIR="${SWF_PRE_DROP_ROOT_DIR}/${CCL_VERSION_FORMAT}/Linux"
PRE_DROP_DIR="${WORKSPACE}/_predrop/"

if [ -z "${LIBFABRIC_INSTALL_DIR}" ]
then
    # OFI ABI 1.3 + FI_HMEM_ZE
    LIBFABRIC_INSTALL_DIR="/p/pdsd/scratch/Uploads/IMPI/other/software/libfabric/linux/v1.12.0"
fi

LIBCCL_SO_VERSION="1.0"
LIBCCL_MAJOR_VERSION=`echo ${LIBCCL_SO_VERSION}|cut -d"." -f1`
LIBCCL_SONAME="libccl.so.${LIBCCL_MAJOR_VERSION}"

#==============================================================================
#                                Defaults
#==============================================================================
set_build_type()
{
    if [ "${ENABLE_DEBUG_BUILD}" == "yes" ]
    then
        BUILD_TYPE="Debug"
    else
        BUILD_TYPE="Release"
    fi
}

set_default_values()
{
    ENABLE_PACK="no"
    if [ -z "${ENABLE_PRE_DROP}" ]
    then
        ENABLE_PRE_DROP="false"
    fi
    if [ -z "${ENABLE_NIGHTLY_DROP}" ]
    then
        ENABLE_NIGHTLY_DROP="false"
    fi
    ENABLE_VERBOSE="yes"
    ENABLE_SILENT_INSTALLATION="no"
    ENABLE_BUILD_CPU="no"
    ENABLE_BUILD_GPU="no"
    ENABLE_INSTALL="no"
    CORE_COUNT=$(( $(lscpu | grep "^Socket(s):" | awk '{print $2}' ) * $(lscpu | grep "^Core(s) per socket:" | awk '{print $4}') ))
    # 4 because of nuc has 4 core only
    MAKE_JOB_COUNT=$(( CORE_COUNT / 3 > 4 ? CORE_COUNT / 3 : 4 ))
    CMAKE_ADDITIONAL_OPTIONS=""
    set_build_type
}
#==============================================================================
#                                Functions
#==============================================================================

# Logging. Always log. Echo if ENABLE_SILENT_INSTALLATION is "yes".
log()
{
    if [ "${ENABLE_SILENT_INSTALLATION}" = "no" ]
    then
        eval $* 2>&1 | tee -a ${LOG_FILE}
    else
        eval $* >> ${LOG_FILE} 2>&1
    fi
}

# Print usage and help information
print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh <options>"
    echo_log "The most appropriate scenario is running on nnlmpinuc0*: ./build.sh -eng-package"
    echo_log "If you want to build separate CPU and GPU, "
    echo_log "recommended nodes for building CPU part is nnlmpibldl10, for GPU part - nnlmpibdw05-06"
    echo_log ""
    echo_log "Please set compiler=gnu|intel|icx|dpcpp. Default compilers - icx (host build), dpcpp (dpcpp build)"
    echo_log ""
    echo_log "<options>:"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -eng-package|--eng-package"
    echo_log "        Prepare CCL eng-package (gzipped tar archive)"
    echo_log "    -build-cpu|--build-cpu"
    echo_log "        Prepare CCL build with icx/icpx"
    echo_log "    -build-gpu|--build-gpu"
    echo_log "        Prepare CCL build with icx/dpcpp with SYCL"
    echo_log "    -conf|--conf"
    echo_log "        Run configure the library"
    echo_log "    -post-build|--post-build"
    echo_log "        Make post-build actions"
    echo_log "    -pack|--pack"
    echo_log "        Prepare CCL package (gzipped tar archive)"
    echo_log "    -swf-pre-drop|--swf-pre-drop"
    echo_log "        Enable SWF pre-drop procedure"
    echo_log "    -build-deb-conf|--build-deb-conf"
    echo_log "        Build debug configuration"
    echo_log "    -install|--install"
    echo_log "        Install package"
    echo_log "    -verbose|--verbose"
    echo_log "        Enable verbose output"
    echo_log "    -job-count <num>|--job-count <num>"
    echo_log "        Specify number of parallel make threads"
    echo_log "        The value 'max' will set the maximum available number of threads"
    echo_log "    -cmake-add-opt '-DOPT1=0 -DOPT2=0'|--cmake-add-opt '-DOPT1=0 -DOPT2=0'"
    echo_log "        Specify additional options for cmake"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -h|-H|-help|--help"
    echo_log "        Print help information"
    echo_log ""
}

# echo the debug information
echo_debug()
{
    if [ ${ENABLE_VERBOSE} = "yes" ]
    then
        echo_log "DEBUG: $*"
    fi
}

echo_log_separator()
{
    echo_log "#=============================================================================="
}

CheckCommandExitCode() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

check_dpcpp_path()
{
    if [ -z "${SYCL_BUNDLE_ROOT}" ]
    then
        SYCL_BUNDLE_ROOT="/p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/linux/"
        echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: ${SYCL_BUNDLE_ROOT}"
    fi
    source ${SYCL_BUNDLE_ROOT}/../env/vars.sh intel64
}

check_gcc_path()
{
    if [ -z "${GNU_BUNDLE_ROOT}" ]
    then
        GNU_BUNDLE_ROOT="/usr/bin/"
        echo "WARNING: GNU_BUNDLE_ROOT is not defined, will be used default: $GNU_BUNDLE_ROOT"
    fi
}

check_icc_path()
{
    if [ -z "${ICC_BUNDLE_ROOT}" ]
    then
        ICC_BUNDLE_ROOT="/p/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/"
        echo "WARNING: ICC_BUNDLE_ROOT is not defined, will be used default: ${ICC_BUNDLE_ROOT}"
    fi
    source ${ICC_BUNDLE_ROOT}/bin/compilervars.sh intel64
}

check_icx_path()
{
    if [ -z "${ICX_BUNDLE_ROOT}" ]
    then
        ICX_BUNDLE_ROOT="/p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/linux/"
        echo "WARNING: ICX_BUNDLE_ROOT is not defined, will be used default: ${ICX_BUNDLE_ROOT}"
    fi
    source ${ICX_BUNDLE_ROOT}/../env/vars.sh intel64
}

define_compiler()
{
    if [ -z "${compiler}" ] && [ -z "${compute_backend}" ]
    then
        compiler="icx"
    fi

    if [[ "${compute_backend}" == "dpcpp"* ]]
    then
        compiler="dpcpp"
    fi

    if [ "${compiler}" == "gnu" ]
    then
        check_gcc_path
        C_COMPILER=${GNU_BUNDLE_ROOT}/gcc
        CXX_COMPILER=${GNU_BUNDLE_ROOT}/g++
    elif [ "${compiler}" == "intel" ]
    then
        check_icc_path
        C_COMPILER=${ICC_BUNDLE_ROOT}/bin/intel64/icc
        CXX_COMPILER=${ICC_BUNDLE_ROOT}/bin/intel64/icpc
    elif [ "${compiler}" == "icx" ]
    then
        check_icx_path
        C_COMPILER=${ICX_BUNDLE_ROOT}/bin/icx
        CXX_COMPILER=${ICX_BUNDLE_ROOT}/bin/icpx
    elif [ "${compiler}" == "dpcpp" ]
    then
        check_dpcpp_path
        C_COMPILER=${SYCL_BUNDLE_ROOT}/bin/icx
        CXX_COMPILER=${SYCL_BUNDLE_ROOT}/bin/dpcpp
    fi
}

build()
{
    define_compiler
    export I_MPI_ROOT=$IMPI_DIR

    BUILD_FOLDER="build"
    OFI_HMEM="0"

    # TODO: check by dpcpp* mask
    # if [[ "${compute_backend}" == "dpcpp"* ]]
    if [ "${compute_backend}" == "dpcpp" ]
    then
        BUILD_FOLDER="build_gpu"
        OFI_HMEM="1"
    fi

    echo "compute_backend =" ${compute_backend}
    echo "compiler =" ${compiler}
    echo "BUILD_FOLDER =" ${BUILD_FOLDER}

    log mkdir -p ${WORKSPACE}/${BUILD_FOLDER} && cd ${WORKSPACE}/${BUILD_FOLDER} && echo ${PWD}

    if [[ ${ENABLE_CONF} = "yes" ]]
    then
        log cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DCMAKE_C_COMPILER="${C_COMPILER}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
        -DUSE_CODECOV_FLAGS="${CODECOV_FLAGS}" -DENABLE_OFI_HMEM="${OFI_HMEM}" \
        -DENABLE_LINKER_RUNPATH=0 -DLIBFABRIC_DIR="${LIBFABRIC_INSTALL_DIR}" \
        -DLIB_SO_VERSION="${LIBCCL_SO_VERSION}" -DLIB_MAJOR_VERSION="${LIBCCL_MAJOR_VERSION}" \
        "${CMAKE_ADDITIONAL_OPTIONS}"
    fi

    log TEMP=${TMP_MAKE_DIR} make -j$MAKE_JOB_COUNT VERBOSE=1 install
    CheckCommandExitCode $? "build failed"
}

post_build()
{
    define_compiler
    rm -rf ${TMP_DIR}/lib
    mkdir -p ${TMP_DIR}/lib
    cd ${TMP_DIR}/lib

    if [[ ${compiler} = "intel" ]]
    then
        COMPILER_DIR=${ICC_BUNDLE_ROOT}
    else
        COMPILER_DIR=${ICX_BUNDLE_ROOT}
    fi

    # libccl.so
    cp ${COMPILER_DIR}/compiler/lib/intel64/libirc.a ./
    cp ${COMPILER_DIR}/compiler/lib/intel64/libsvml.a ./
    cp ${COMPILER_DIR}/compiler/lib/intel64/libimf.a ./
    cp ${WORKSPACE}/build/_install/lib/libccl.a ./

    if [ "${ENABLE_CODECOV}" = "yes" ]
    then
        cp ${COMPILER_DIR}/compiler/lib/intel64/libipgo.a ./
        ar x libipgo.a
    fi

    ar x libccl.a
    ar x libirc.a
    ar x libsvml.a svml_i_div4_iface_la.o svml_d_feature_flag_.o \
        svml_i_div4_core_e7la.o svml_i_div4_core_exla.o svml_i_div4_core_h9la.o \
        svml_i_div4_core_y8la.o
    ar x libimf.a float16.o

    if [ ! -z "${LIBFABRIC_INSTALL_DIR}" ]
    then
        LDFLAGS="-L${LIBFABRIC_INSTALL_DIR}/lib/"
    fi
    LDFLAGS="${LDFLAGS} ${HWLOC_DIR}/lib/libhwloc.a"

    # check whether we have a dependency on itt framework and
    # link with the library if we have
    nm libccl.a | grep __itt.* >/dev/null 2>&1
    if [[ $? == 0 ]]
    then
        LDFLAGS="${LDFLAGS} ${ITT_DIR}/lib64/libittnotify.a"
    fi

    g++ -fPIE -fPIC -Wl,-z,now -Wl,-z,relro -Wl,-z,noexecstack \
        -Wl,--version-script=${WORKSPACE}/ccl.map -Wl,--no-undefined -std=gnu99 -Wall -Werror \
        -D_GNU_SOURCE -fvisibility=hidden -O3 -DNDEBUG \
        -std=gnu99 -O3 -shared -Wl,-soname=${LIBCCL_SONAME} \
        -o libccl.so.${LIBCCL_SO_VERSION} *.o ${LDFLAGS} \
        -L${WORKSPACE}/build/_install/lib/ -lm -ldl -pthread

    CheckCommandExitCode $? "post build failed"
    rm -rf *.o

    mkdir -p ${WORKSPACE}/build/_install/lib/cpu_gpu_dpcpp
    mkdir -p ${WORKSPACE}/build/_install/lib/cpu
    mkdir -p ${WORKSPACE}/build/_install/include
    mv ${WORKSPACE}/build/_install/include/oneapi ${WORKSPACE}/build/_install/include
    mv ${TMP_DIR}/lib/lib* ${WORKSPACE}/build/_install/lib/cpu
    mv ${WORKSPACE}/build/_install/lib/prov ${WORKSPACE}/build/_install/lib/cpu
    mv ${WORKSPACE}/build_gpu/_install/lib/* ${WORKSPACE}/build/_install/lib/cpu_gpu_dpcpp
    cp -r ${WORKSPACE}/examples ${WORKSPACE}/build/_install
    prepare_pkgconfig
}

replace_tags()
{
    INITIAL_LINE="$1"
    while read TAG_LINE
    do
        tag=`echo "$TAG_LINE" | awk '{print $1}'`
        tag_sub=`echo "$TAG_LINE" | awk '{print $3}'`
        INITIAL_LINE=`echo $INITIAL_LINE | sed -e "s|<${tag}>|${tag_sub}|"`
    done < ${WORKSPACE}/boms/tags/l_oneapi.vars
    echo "$INITIAL_LINE"
}

# Prepare staging according to the BOM-files
prepare_staging()
{
    # Supported modes: package, swf_pre_drop
    MODE="$1"

    exp_array="tags" #using to exclude boms from package mode

    case $MODE in
        "package")
            TARGET_DIR="${PACKAGE_ENG_DIR}"
            ;;
        "swf_pre_drop")
            TARGET_DIR="${PRE_DROP_DIR}"
            ;;
        *)
            echo_log "ERROR: unsupported or empty MODE ($MODE) for prepare_staging() function"
            exit 1
            ;;
    esac

    echo_debug "PRE_DROP_DIR = ${PRE_DROP_DIR}"
    echo_debug "SWF_PRE_DROP_DIR = ${SWF_PRE_DROP_DIR}"
    echo_debug "ENABLE_NIGHTLY_DROP = ${ENABLE_NIGHTLY_DROP}"
    cd ${WORKSPACE}

    rm -rf ${TMP_DIR} ${TARGET_DIR}
    mkdir -p ${TMP_DIR}
    mkdir -p ${TARGET_DIR}

    if [ "$MODE" = "swf_pre_drop" ]
    then
        mkdir -p ${TARGET_DIR}/BOMS
        cp ${BOM_DIR}/${BOM_LIST_NAME} ${TARGET_DIR}/BOMS/
        mkdir -p ${TARGET_DIR}/bom_lists
        sed -e "/^#/d" -e "/^DeliveryName/d" ${BOM_DIR}/bom_lists.txt > ${TMP_DIR}/bom_lists.txt
    fi
   # Result of BOM-files processing (if there're missed files RES=1)
    RES=0

    cp ${BOM_DIR}/${BOM_LIST_NAME} ${TMP_DIR}/

    while read LINE
    do
        echo_debug "LINE = $LINE"
        BOM_SOURCE=`echo "$LINE" | awk -F $'\t' '{print $1}'`
        BOM_DEST=`echo "$LINE" | awk -F $'\t' '{print $2}'`
        BOM_CHECK=`echo "$LINE" | awk -F $'\t' '{print $3}'`
        BOM_ORIG=`echo "$LINE" | awk -F $'\t' '{print $4}'`
        BOM_OWN=`echo "$LINE" | awk -F $'\t' '{print $5}'`
        BOM_REST=`echo "$LINE" | awk -F $'\t' '{print $6}'`
        echo_debug "BOM_SOURCE = $BOM_SOURCE"
        echo_debug "BOM_DEST = $BOM_DEST"
        echo_debug "BOM_CHECK = $BOM_CHECK"
        echo_debug "BOM_ORIG = $BOM_ORIG"
        echo_debug "BOM_OWN = $BOM_OWN"
        echo_debug "BOM_REST = $BOM_REST"

        if [ -n "${BOM_REST}" ]
        then
            echo_log "ERROR: unexpected extra data in BOM line: ${BOM_REST}"
            exit 1
        fi
        case ${BOM_SOURCE} in
            "<deliverydir>"*)
                BOM_SOURCE=`echo ${BOM_SOURCE} | sed -e "s|<deliverydir>/bom_lists/||"`
                echo_debug "BOM_SOURCE = $BOM_SOURCE"

                if [ -z "${BOM_SOURCE}" ]
                then
                    echo_log "ERROR: ${TMP_DIR}/bom_lists.txt is invalid"
                    exit 1
                fi

                if [ ! -f "${BOM_DIR}/${BOM_SOURCE}" ]
                then
                    echo_log "ERROR: ${BOM_DIR}/${BOM_SOURCE} is unavailable"
                    exit 1
                fi

                echo_log "${BOM_SOURCE}..."

                sed -e "/^#/d" ${BOM_DIR}/${BOM_SOURCE} > ${TMP_DIR}/${BOM_SOURCE}

                if [ "$MODE" = "swf_pre_drop" ]
                then
                    cp ${BOM_DIR}/${BOM_SOURCE} ${TARGET_DIR}/bom_lists/
                fi


                while read CUR_BOM_FILE
                do
                    echo_debug "CUR_BOM_FILE = $CUR_BOM_FILE"
                    echo "CUR_BOM_FILE = $CUR_BOM_FILE" >> ${PACKAGE_LOG}
                    echo "CUR_BOM_FILE = ${CUR_BOM_FILE}"
                    if [ ! -f "${BOM_DIR}/${CUR_BOM_FILE}" ]
                    then
                        echo_log "ERROR: ${BOM_DIR}/${CUR_BOM_FILE} is unavailable"
                        exit 1
                    fi

                    bad_word=""

                    if [ "${MODE}" == "package" ]
                    then
                        for word in $exp_array
                        do
                            if echo $CUR_BOM_FILE | grep -q $word
                            then
                                bad_word=$word
                            fi
                        done
                    fi

                    if [ ! -z $bad_word ]
                    then
                        echo_debug "${CUR_BOM_FILE} is skipped for pack mode"
                    else
                        if [ "$MODE" = "swf_pre_drop" ]
                        then
                            # Remove the last column which is for internal purposes (for engineering package)
                            sed -e "/^<deliverydir\>/ s/\t[^[:space:]]*$//" ${BOM_DIR}/${CUR_BOM_FILE} > ${TARGET_DIR}/BOMS/${CUR_BOM_FILE}
                        fi

                        sed -e "/^#/d" -e "/^DeliveryName/d" ${BOM_DIR}/${CUR_BOM_FILE} > ${TMP_DIR}/${CUR_BOM_FILE}

                        while read LINE
                        do
                            echo_debug "LINE = $LINE"

                            BL_SOURCE=`echo "$LINE" | awk -F $'\t' '{print $1}'`
                            BL_INSTALL=`echo "$LINE" | awk -F $'\t' '{print $2}'`
                            BL_CHECKSUM=`echo "$LINE" | awk -F $'\t' '{print $3}' `
                            BL_ORIGIN=`echo "$LINE" | awk -F $'\t' '{print $4}'`
                            BL_OWNER=`echo "$LINE" | awk -F $'\t' '{print $5}'`
                            BL_FEATURE=`echo "$LINE" | awk -F $'\t' '{print $6}'`
                            BL_ICOND=`echo "$LINE" | awk -F $'\t' '{print $7}'`
                            BL_MODE=`echo "$LINE" | awk -F $'\t' '{print $8}'`
                            BL_IOWNER=`echo "$LINE" | awk -F $'\t' '{print $9}'`
                            BL_REDIST=`echo "$LINE" | awk -F $'\t' '{print $10}'`
                            BL_LINK=`echo "$LINE" | awk -F $'\t' '{print $11}'`
                            BL_REST=`echo "$LINE" | awk -F $'\t' '{print $12}'`

                            echo_debug "BL_SOURCE = $BL_SOURCE"
                            echo_debug "BL_INSTALL = $BL_INSTALL"
                            echo_debug "BL_CHECKSUM = $BL_CHECKSUM"
                            echo_debug "BL_ORIGIN = $BL_ORIGIN"
                            echo_debug "BL_OWNER = $BL_OWNER"
                            echo_debug "BL_FEATURE = $BL_FEATURE"
                            echo_debug "BL_ICOND = $BL_ICOND"
                            echo_debug "BL_MODE = $BL_MODE"
                            echo_debug "BL_IOWNER = $BL_IOWNER"
                            echo_debug "BL_REDIST = $BL_REDIST"
                            echo_debug "BL_LINK = $BL_LINK"
                            echo_debug "BL_REST = $BL_REST"

                            if [ -n "${BL_REST}" ]
                            then
                                echo_log "ERROR: unexpected extra data in BOM line: ${BL_REST}"
                                exit 1
                            fi

                            case ${BL_SOURCE} in
                                "<deliverydir>"*)
                                    if [ "$BL_INSTALL" = "N/A" ] && [ "$MODE" = "package" ]
                                    then
                                        echo_debug "Skipped for MODE=pack"
                                    else
                                        BL_SOURCE=`echo $BL_SOURCE | sed -e "s|<deliverydir>/||"`
                                        #
                                        BL_INSTALL=`echo $BL_INSTALL | sed -e "s|<installdir><l_ccl_install_path><l_ccl_platform>||" | \
                                                                    sed -e "s|<installdir><l_ccl_install_path>||"`
                                        if [ $CUR_BOM_FILE == "l_license_runtime.txt" ]
                                        then
                                            BL_INSTALL=$(replace_tags $BL_INSTALL)
                                        fi
                                        echo_debug "BL_SOURCE = $BL_SOURCE"
                                        echo_debug "BL_INSTALL = $BL_INSTALL"

                                        if [ -z "${BL_SOURCE}" ] || [ -z "${BL_INSTALL}" ]
                                        then
                                            echo_log "ERROR: ${BOM_DIR}/${CUR_BOM_FILE} is invalid"
                                            exit 1
                                        fi

                                        #echo -en "\t${BL_SOURCE}..." >> ${PACKAGE_LOG}
                                        case $MODE in
                                            "package")
                                                DROP_DEST_FILE="${BL_INSTALL}"
                                                DROP_DEST_FILE=$(replace_tags $DROP_DEST_FILE)
                                                ;;
                                            "swf_pre_drop")
                                                DROP_DEST_FILE=`echo ${BL_SOURCE} | sed -e "s/<deliverydir>\///"`""
                                                ;;
                                        esac

                                        echo_debug "DROP_DEST_FILE = $DROP_DEST_FILE"

                                        case "${BL_LINK}" in
                                            "<installroot>"*)
                                                DROP_SRC_FILE=`echo $BL_LINK | sed -e "s|<installroot>/||"`
                                                if [ "$MODE" = "swf_pre_drop" ]
                                                then
                                                    DROP_SRC_FILE="${PACKAGE_ENG_DIR}/${DROP_SRC_FILE}"
                                                else
                                                    DROP_SRC_FILE="${WORKSPACE}/build/_install/${DROP_SRC_FILE}"
                                                fi
                                                echo_debug "DROP_SRC_FILE = ${DROP_SRC_FILE}"
                                                ;;
                                            "<ccl_root>"*)
                                                DROP_SRC_FILE=`echo $BL_LINK | sed -e "s|<ccl_root>/||"`
                                                DROP_SRC_FILE="${WORKSPACE}/${DROP_SRC_FILE}"
                                                echo_debug "DROP_SRC_FILE = ${DROP_SRC_FILE}"
                                                ;;
                                            *)
                                                ;;
                                        esac

                                        DROP_DEST_DIR=`dirname ${TARGET_DIR}/$DROP_DEST_FILE`
                                        mkdir -p ${DROP_DEST_DIR}

                                        if [ ! -f ${DROP_SRC_FILE} ]
                                        then
                                            echo " NOK: ${DROP_SRC_FILE}"  >> ${PACKAGE_LOG}
                                            RES=1
                                            continue
                                        fi

                                        cp ${DROP_SRC_FILE} ${TARGET_DIR}/${DROP_DEST_FILE}
                                        chmod ${BL_MODE} ${TARGET_DIR}/${DROP_DEST_FILE}
                                        echo " OK" >> ${PACKAGE_LOG}
                                    fi
                                    ;;
                                "<N/A>")
                                    if [ "$MODE" = "swf_pre_drop" ]
                                    then
                                        echo_debug "Skipped for MODE=swf_pre_drop"
                                    else
                                        #TODO packing
                                        #echo "packing mode is under construction"
                                        BL_SOURCE=`echo $BL_LINK | sed -e "s|<installdir><l_ccl_install_path><l_ccl_platform>||" `
                                        BL_SOURCE=$(replace_tags $BL_SOURCE)
                                        BL_INSTALL=`echo $BL_INSTALL | sed -e "s|<installdir><l_ccl_install_path><l_ccl_platform>||"`
                                        BL_INSTALL=$(replace_tags $BL_INSTALL)
                                        if [ -z "${BL_SOURCE}" ]
                                        then
                                            echo_log "ERROR: BL_SOURCE is empty"
                                            exit 1
                                        fi
                                        if [ -z "${BL_INSTALL}" ]
                                        then
                                            echo_log "ERROR: BL_INSTALL is empty"
                                            exit 1
                                        fi
                                        if [ -z "${BL_LINK}" ]
                                        then
                                            echo_log "ERROR: BL_LINK is empty"
                                            exit 1
                                        fi
                                        echo_log -en "\t${BL_SOURCE}..." >> ${PACKAGE_LOG}

                                        DROP_DEST_FILE="${BL_INSTALL}"
                                        case "${BL_LINK}" in
                                            "<installdir"*)
                                                PATH_TO_RM=`dirname ${BL_INSTALL}`
                                                if [ "${PATH_TO_RM}" = "." ]
                                                then
                                                    DROP_SRC_FILE="${BL_SOURCE}"
                                                else
                                                    DROP_SRC_FILE=`echo "${BL_SOURCE}" | sed -e "s|${PATH_TO_RM}/||"`
                                                fi
                                                ;;
                                        esac
                                        DROP_DEST_DIR=`dirname ${TARGET_DIR}/${DROP_DEST_FILE}`
                                        DROP_DEST_FILE=`basename ${DROP_DEST_FILE}`
                                        mkdir -p ${DROP_DEST_DIR}
                                        cd ${DROP_DEST_DIR} && ln -s ${DROP_SRC_FILE} ${DROP_DEST_FILE}
                                        echo " OK" >> ${PACKAGE_LOG}
                                    fi
                                    ;;

                            esac
                        done < ${TMP_DIR}/${CUR_BOM_FILE}

                    fi
                echo_log "Done"
                done < ${TMP_DIR}/${BOM_SOURCE}

                if [ "${ENABLE_VERBOSE}" = "yes" ]
                then
                    if [ -f ${PACKAGE_LOG} ]
                    then
                        cat ${PACKAGE_LOG} | column -t | sed 's|^|\t|' | tee -a ${LOG_FILE}
                        rm -f ${PACKAGE_LOG}
                    fi
                fi
                echo_log "${BOM_SOURCE}... DONE"
                ;;
        esac
    done < ${TMP_DIR}/bom_lists.txt


    if [ $RES -eq 1 ]
    then
        echo_log "ERROR: packaging failed"
        exit 1
    fi
}

prepare_pkgconfig()
{
    echo_log "run_prepare_pkgconfig()..."

    declare -a ccl_configurations=("cpu" "cpu_gpu_dpcpp")

    for i in "${ccl_configurations[@]}"
    do
        PKGCONFIG_PATH="${WORKSPACE}/build/_install/lib/pkgconfig"
        mkdir -p "${PKGCONFIG_PATH}"
        cp ${WORKSPACE}/pkgconfig/template.pc ${PKGCONFIG_PATH}/ccl-${i}.pc
        sed -i "s/@BUILD_TYPE@/${i}/" ${PKGCONFIG_PATH}/ccl-${i}.pc
        if [ "$i" ==  "cpu_gpu_dpcpp" ]
        then
            sed -i "s/@OTHER_FLAGS@/-lsycl/" ${PKGCONFIG_PATH}/ccl-${i}.pc
        else
            sed -i "s/@OTHER_FLAGS@//" ${PKGCONFIG_PATH}/ccl-${i}.pc
        fi
    done

    echo_log "run_prepare_pkgconfig()... DONE"
}

# Make archive package
make_package()
{
        echo_log "Create eng package..."
        rm -rf ${TMP_DIR}/${CCL_PACKAGE_NAME}
        rm -f ${TMP_DIR}/${CCL_PACKAGE_NAME}.tgz
        mkdir -p ${TMP_DIR}/${CCL_PACKAGE_NAME}/package
        add_copyrights
        cp -r ${PACKAGE_ENG_DIR}/* ${TMP_DIR}/${CCL_PACKAGE_NAME}/package

        cd ${TMP_DIR}/${CCL_PACKAGE_NAME}/package && tar czf ${TMP_DIR}/${CCL_PACKAGE_NAME}/files.tar.gz * --owner=root --group=root
        cd ${WORKSPACE}
        rm -rf ${TMP_DIR}/${CCL_PACKAGE_NAME}/package
        sed -i -e "s|CCL_SUBSTITUTE_OFFICIAL_VERSION|${CCL_VERSION_FORMAT}|g" ${WORKSPACE}/scripts/install.sh
        sed -i -e "s|CCL_SUBSTITUTE_OFFICIAL_VERSION|${CCL_VERSION_FORMAT}|g" ${PACKAGE_ENG_DIR}/lib/pkgconfig/*.pc
        cp ${WORKSPACE}/scripts/install.sh ${TMP_DIR}/${CCL_PACKAGE_NAME}
        cd ${TMP_DIR}/${CCL_PACKAGE_NAME} && zip -0 -r -m -P accept ${TMP_DIR}/${CCL_PACKAGE_NAME}/package.zip *
        cp ${WORKSPACE}/doc/cclEULA.txt ${TMP_DIR}/${CCL_PACKAGE_NAME}
        cp ${WORKSPACE}/doc/README.txt ${TMP_DIR}/${CCL_PACKAGE_NAME}

        cd ${TMP_DIR} && tar czf ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz ${CCL_PACKAGE_NAME} --owner=root --group=root
        CheckCommandExitCode $? "package failed"
        echo_log "Package: ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz"
        echo_log "Create package... DONE"
}

#==============================================================================
#                              Parse arguments
#==============================================================================
parse_arguments()
{
    if [ $# -eq 0 ]
    then
        print_help
        exit 1
    fi

    while [ $# -ne 0 ]
    do
        case $1 in
            "--eng-package"|"-eng-package")
                ENABLE_BUILD_CPU="yes"
                ENABLE_BUILD_GPU="yes"
                ENABLE_PREPARE_FILES="yes"
                ENABLE_CONF="yes"
                ENABLE_POST_BUILD="yes"
                ENABLE_PACK="yes"
                ENABLE_INSTALL="yes"
                ;;
            "-pack"|"--pack")
                ENABLE_PACK="yes"
                ;;
            "-build-cpu"|"--build-cpu")
                ENABLE_BUILD_CPU="yes"
                ;;
            "-build-gpu"|"--build-gpu")
                ENABLE_BUILD_GPU="yes"
                ;;
            "-conf"|"--conf")
                ENABLE_PREPARE_FILES="yes"
                ENABLE_CONF="yes"
                ;;
            "-post-build"|"--post-build")
                ENABLE_POST_BUILD="yes"
                ;;
            "-test-pre-drop"|"--test-pre-drop")
                ENABLE_TEST_PRE_DROP="yes"
                ;;
            "-swf-pre-drop"|"--swf-pre-drop")
                ENABLE_PRE_DROP="true"
                ;;
            "-build-deb-conf"|"--build-deb-conf")
                ENABLE_DEBUG_BUILD="yes"
                ;;
            "--verbose"|"--verbose")
                ENABLE_VERBOSE="yes"
                ;;
            "-install"| "--install")
                ENABLE_INSTALL="yes"
                ;;
            "-job-count"| "--job-count")
                if [[ $2 == "max" ]]
                then
                    MAKE_JOB_COUNT=$(nproc)
                    shift
                elif [[ $2 == ?(-)+([0-9]) ]]
                then
                    MAKE_JOB_COUNT=$2
                    shift
                fi
                ;;
            "-cmake-add-opt"| "--cmake-add-opt")
                CMAKE_ADDITIONAL_OPTIONS=$2
                shift
                ;;
            "-help"|"--help"|"-h"|"-H")
                print_help
                exit 0
                ;;
            *)
                echo_log "ERROR: unknown option ($1)"
                echo_log "See ./build.sh -help"
                exit 1
                ;;
        esac

        shift
    done
    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "ENABLE_BUILD_CPU          = ${ENABLE_BUILD_CPU}"
    echo_log "ENABLE_BUILD_GPU          = ${ENABLE_BUILD_GPU}"
    echo_log "ENABLE_CONF               = ${ENABLE_CONF}"
    echo_log "ENABLE_POST_BUILD         = ${ENABLE_POST_BUILD}"
    echo_log "ENABLE_PACK               = ${ENABLE_PACK}"
    echo_log "ENABLE_DEBUG_BUILD        = ${ENABLE_DEBUG_BUILD}"
    echo_log "ENABLE_PRE_DROP           = ${ENABLE_PRE_DROP}"
    echo_log "ENABLE_INSTALL            = ${ENABLE_INSTALL}"
    echo_log "MAKE_JOB_COUNT            = ${MAKE_JOB_COUNT}"
    echo_log "CMAKE_ADDITIONAL_OPTIONS  = ${CMAKE_ADDITIONAL_OPTIONS}"
    echo_log "-----------------------------------------------------------"

    TIMESTAMP_START=`date +%s`
    set_build_type
}

#==============================================================================
#                             Preparing
#==============================================================================
preparing_files()
{
    if [[ ${ENABLE_PREPARE_FILES} = "yes" ]]
    then
        echo_log_separator
        echo_log "#\t\t\tPreparing files..."
        echo_log_separator
        ${SCRIPT_DIR}/prepare_files.sh ccl_oneapi
        CheckCommandExitCode $? "ERROR: preparing files failed"
        echo_log_separator
        echo_log "#\t\t\tPreparing files...DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                             Building
#==============================================================================
run_build_cpu()
{
    if [[ "${ENABLE_BUILD_CPU}" = "yes" ]]
    then
        echo_log_separator
        echo_log "#\t\t\tBuilding host..."
        echo_log_separator
        build
        echo_log_separator
        echo_log "#\t\t\tBuilding host... DONE"
        echo_log_separator
    fi
}
run_build_gpu()
{
    if [[ "${ENABLE_BUILD_GPU}" = "yes" ]]
    then
        echo_log_separator
        echo_log "#\t\t\tBuilding dpcpp..."
        echo_log_separator
        if [ -z "${compute_backend}" ]
        then
            compute_backend="dpcpp"
        fi
        build
        echo_log_separator
        echo_log "#\t\t\tBuilding dpcpp... DONE"
        echo_log_separator
    fi
}

run_post_build()
{
    if [ "${ENABLE_POST_BUILD}" = "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPost build..."
        echo_log_separator
        post_build
        echo_log_separator
        echo_log "#\t\t\tPost build... DONE"
        echo_log_separator
    fi
}
#==============================================================================
#                             Packaging
#==============================================================================
run_copy_package_to_common_folder()
{
    if [ "${ENABLE_PRE_DROP}" == "true" ]
    then
        echo_log_separator
        echo_log "#\t\t\tCopy package ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz to ${RELEASED_BUILD_DIR}..."
        echo_log_separator
        cp ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz ${RELEASED_BUILD_DIR}
        echo_log_separator
        echo_log "#\t\t\tCopy package to ${RELEASED_BUILD_DIR}... DONE"
        echo_log_separator
    fi
}
run_pack()
{
    if [ "${ENABLE_PACK}" == "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPackaging..."
        echo_log_separator
        prepare_staging package
        make_package
        run_copy_package_to_common_folder
        echo_log_separator
        echo_log "#\t\t\tPackaging... DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                              Installation
#==============================================================================
install_package()
{
    if  [ "${ENABLE_INSTALL}" == "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPackage installation..."
        echo_log_separator
        tar xfz ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz --directory ${WORKSPACE}
        unzip -P accept ${WORKSPACE}/${CCL_PACKAGE_NAME}/package.zip -d ${WORKSPACE}/${CCL_PACKAGE_NAME}
        ${WORKSPACE}/${CCL_PACKAGE_NAME}/install.sh -s -d ${WORKSPACE}/${CCL_PACKAGE_NAME}/tmp/
        find ${WORKSPACE}/${CCL_PACKAGE_NAME} -mindepth 1 -maxdepth 1 -not -name tmp -exec rm -rf {} \;
        cp -r ${WORKSPACE}/${CCL_PACKAGE_NAME}/tmp/* ${WORKSPACE}/${CCL_PACKAGE_NAME}
        rm -rf ${WORKSPACE}/${CCL_PACKAGE_NAME}/tmp
        echo_log_separator
        echo_log "#\t\t\tPackage installation...DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                              Creating copyright
#==============================================================================

add_copyrights()
{
    echo_log_separator
    echo_log "#\t\t\tAdd copyrights..."
    echo_log_separator

    mkdir ${WORKSPACE}/copyright_tmp
    TMP_COPYRIGHT_DIR="${WORKSPACE}/copyright_tmp"

    COPYRIGHT_TEMPLATE_INTEL="${WORKSPACE}/scripts/copyright/copyright_template_intel.txt"
    COPYRIGHT_INTEL_SH="${TMP_COPYRIGHT_DIR}/copyright_intel.sh"
    COPYRIGHT_INTEL_C="${TMP_COPYRIGHT_DIR}/copyright_intel.c"

    echo "Generate ${COPYRIGHT_INTEL_SH}..."
    echo "1a" > ${COPYRIGHT_INTEL_SH}
    echo "#" >> ${COPYRIGHT_INTEL_SH}
    sed -e "s|^|# |" ${COPYRIGHT_TEMPLATE_INTEL} >> ${COPYRIGHT_INTEL_SH}
    echo "#" >> ${COPYRIGHT_INTEL_SH}
    echo "." >> ${COPYRIGHT_INTEL_SH}
    echo "w" >> ${COPYRIGHT_INTEL_SH}
    echo "Generate ${COPYRIGHT_INTEL_SH}... DONE"

    echo "Generate ${COPYRIGHT_INTEL_C}..."
    echo "1i" > ${COPYRIGHT_INTEL_C}
    echo "/*" >> ${COPYRIGHT_INTEL_C}
    sed -e "s|^|    |" ${COPYRIGHT_TEMPLATE_INTEL} >> ${COPYRIGHT_INTEL_C}
    echo "*/" >> ${COPYRIGHT_INTEL_C}
    echo "." >> ${COPYRIGHT_INTEL_C}
    echo "w" >> ${COPYRIGHT_INTEL_C}
    echo "Generate ${COPYRIGHT_INTEL_C}... DONE"

    for CUR_FILE in `find ${PACKAGE_ENG_DIR}/include/ \( -name "*.h" -or -name "*.hpp" \) -type f`
    do
        ed `realpath ${CUR_FILE}` < ${COPYRIGHT_INTEL_C} >/dev/null 2>&1
    done

    for CUR_FILE in `find ${PACKAGE_ENG_DIR}/examples/ \( -name "*.h" -or -name "*.hpp" -or -name "*.c" -or -name "*.cpp" \) -type f`
    do
        ed `realpath ${CUR_FILE}` < ${COPYRIGHT_INTEL_C} >/dev/null 2>&1
    done

    for CUR_FILE in `find ${PACKAGE_ENG_DIR}/examples/ \( -name "*.sh" \) -type f`
    do
        ed `realpath ${CUR_FILE}` < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    done

    for CUR_FILE in `find ${PACKAGE_ENG_DIR}/examples/ \( -name "CMakeLists.txt" \) -type f`
    do
        ed `realpath ${CUR_FILE}` < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    done

    ed ${PACKAGE_ENG_DIR}/env/vars.sh < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    ed ${PACKAGE_ENG_DIR}/modulefiles/ccl < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    ed ${PACKAGE_ENG_DIR}/lib/cmake/oneCCL/oneCCLConfig.cmake < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    ed ${PACKAGE_ENG_DIR}/lib/cmake/oneCCL/oneCCLConfigVersion.cmake < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    ed ${PACKAGE_ENG_DIR}/lib/pkgconfig/ccl-cpu.pc < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1
    ed ${PACKAGE_ENG_DIR}/lib/pkgconfig/ccl-cpu_gpu_dpcpp.pc < ${COPYRIGHT_INTEL_SH} >/dev/null 2>&1

    rm -rf ${TMP_COPYRIGHT_DIR}
    echo_log_separator
    echo_log "#\t\t\tAdd copyrights...DONE"
    echo_log_separator
}

#==============================================================================
#                              Performing pre-drop
#==============================================================================
run_pre_drop()
{
    if  [ "${ENABLE_TEST_PRE_DROP}" == "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPerforming pre-drop..."
        echo_log_separator
        prepare_staging swf_pre_drop
        echo_log_separator
        echo_log "#\t\t\tPerforming pre-drop... DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                              SWF pre-drop
#==============================================================================
run_swf_pre_drop()
{
    if [ "${ENABLE_PRE_DROP}" == "true" ]
    then
        echo_log_separator
        echo_log "#\t\t\tSWF pre-drop..."
        echo_log_separator
        mkdir -p ${SWF_PRE_DROP_DIR}
        if [ "${ENABLE_NIGHTLY_DROP}" == "true" ]
        then
            mkdir -p ${SWF_PRE_DROP_DIR}/0000_`date +%Y-%m-%d`/SWF_Drops/
            cp -R ${PRE_DROP_DIR}/* ${SWF_PRE_DROP_DIR}/0000_`date +%Y-%m-%d`/SWF_Drops/
        else
            mkdir -p ${SWF_PRE_DROP_DIR}/`date +%Y-%m-%d`/SWF_Drops/
            cp -R ${PRE_DROP_DIR}/* ${SWF_PRE_DROP_DIR}/`date +%Y-%m-%d`/SWF_Drops/
        fi
        rm -rf ${SWF_PRE_DROP_DIR}/SWF_Drops
        mkdir -p ${SWF_PRE_DROP_DIR}/SWF_Drops/
        cp -R ${PRE_DROP_DIR}/* ${SWF_PRE_DROP_DIR}/SWF_Drops/
        CheckCommandExitCode $? "predrop failed"
        echo_log_separator
        echo_log "#\t\t\tSWF pre-drop... DONE"
        echo_log_separator
    fi
}



#==============================================================================
#                              MAIN
#==============================================================================
set_default_values
parse_arguments "$@"
echo "BUILD_TYPE=" $BUILD_TYPE
preparing_files
run_build_cpu
run_build_gpu
run_post_build
run_pack
install_package
run_pre_drop
run_swf_pre_drop

TIMESTAMP_FINISH=`date +%s`
DURATION=`expr ${TIMESTAMP_FINISH} - ${TIMESTAMP_START}`

echo_log "#=============================================================================="
echo_log "Building duration     = ${DURATION} sec"
echo_log "Log file              = ${LOG_FILE}"
echo_log "#=============================================================================="
