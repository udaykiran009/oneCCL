#!/bin/bash
set -o pipefail

BASENAME=`basename $0 .sh`

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
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

DATE=`date +%Y%m%d`
TIME=`date +%H%M%S`
ARCH=`uname -m`


WORKSPACE=`cd ${SCRIPT_DIR}/../ && pwd -P`

if [ -z "${BUILD_COMPILER_TYPE}" ]
then
    BUILD_COMPILER_TYPE="clang"
fi

if [ "${BUILD_COMPILER_TYPE}" = "gnu" ]
then
    BUILD_COMPILER=/usr/bin
    C_COMPILER=${BUILD_COMPILER}/gcc
    CXX_COMPILER=${BUILD_COMPILER}/g++
elif [ "${BUILD_COMPILER_TYPE}" = "intel" ]
then
	BUILD_COMPILER=/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2019.4.243/linux/bin/intel64/
    C_COMPILER=${BUILD_COMPILER}/icc
    CXX_COMPILER=${BUILD_COMPILER}/icpc
else
    if [ -z "${SYCL_BUNDLE_ROOT}" ]
    then
        echo "WARNING: SYCL_BUNDLE_ROOT is not defined, will be used default: /p/pdsd/scratch/Software/oneAPI/inteloneapi_31/compiler/inteloneapi/dpcpp_compiler/latest/env/vars.sh"
        source /p/pdsd/scratch/Software/oneAPI/inteloneapi_31/compiler/inteloneapi/dpcpp_compiler/latest/env/vars.sh
        SYCL_BUNDLE_ROOT="/p/pdsd/scratch/Software/oneAPI/inteloneapi_31/compiler/inteloneapi/dpcpp_compiler/latest"
    fi
    BUILD_COMPILER=${SYCL_BUNDLE_ROOT}/bin
    C_COMPILER=${BUILD_COMPILER}/clang
    CXX_COMPILER=${BUILD_COMPILER}/clang++
fi
if [ "${ENABLE_CODECOV}" = "yes" ]
then
	CODECOV_FLAGS="TRUE"
else
	CODECOV_FLAGS=""
fi


BOM_DIR="${WORKSPACE}/boms"
BOM_LIST_NAME="bom_list.txt"
TMP_DIR="${WORKSPACE}/_tmp"
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

CCL_COPYRIGHT_YEAR="2019"
CCL_VERSION_FORMAT="2021.1-alpha02"

CCL_PACKAGE_PREFIX="l_ccl_"

if [ -z "${CCL_PACKAGE_PHASE}" ]
then
    CCL_PACKAGE_PHASE="ENG"
fi
CCL_PACKAGE_SUFFIX="_${CCL_PACKAGE_PHASE}_ww`date +%V`.${DATE}.${TIME}"
CCL_PACKAGE_NAME="${CCL_PACKAGE_PREFIX}${CCL_VERSION_FORMAT}${CCL_PACKAGE_SUFFIX}"
SWF_PRE_DROP_DIR="/p/pdsd/scratch/Drops/CCL/1.0/"
PRE_DROP_DIR="${WORKSPACE}/_predrop/`date +%Y-%m-%d`/SWF_Drops"

#==============================================================================
#                                Defaults
#==============================================================================
set_default_values()
{
    ENABLE_PACK="no"
    if [ -z "${ENABLE_PRE_DROP}" ]
    then
	ENABLE_PRE_DROP="no"
    fi
    ENABLE_DEBUG="no"
    ENABLE_VERBOSE="yes"
    ENABLE_BUILD="no"
}
#==============================================================================
#                                Functions
#==============================================================================

# Logging. Always log. Echo if ENABLE_VERBOSE is "yes".
log()
{
    if [ "${ENABLE_VERBOSE}" = "yes" ]; then
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
    echo_log ""
    echo_log "<options>:"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -eng-package|--eng-package"
    echo_log "        [Under construction] Prepare CCL eng-package (gzipped tar archive)"
    echo_log "    -build|--build"
    echo_log "        Compile library"
    echo_log "    -pack|--pack"
    echo_log "        Prepare CCL package (gzipped tar archive)"
    echo_log "    -swf-pre-drop|--swf-pre-drop"
    echo_log "        Enable SWF pre-drop procedure"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -h|-H|-help|--help"
    echo_log "        Print help information"
    echo_log ""
}

# echo the debug information
echo_debug()
{
    if [ ${ENABLE_DEBUG} = "yes" ]; then
        echo_log "DEBUG: $*"
    fi
}

echo_log_separator()
{
    echo_log "#=============================================================================="
}

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

build()
{

    log mkdir ${WORKSPACE}/build && cd ${WORKSPACE}/build && echo ${PWD}
    log cmake ..  -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER="${C_COMPILER}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" -DUSE_CODECOV_FLAGS="${CODECOV_FLAGS}"
    log make -j4 VERBOSE=1 install
    if [ $? -ne 0 ]
    then
        echo "ERROR: build failed"
        exit 1
    fi
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

    echo_debug "PRE_DROP_DIR = ${SWF_PRE_DROP_DIR}"

    rm -rf ${TMP_DIR} ${TARGET_DIR}
    mkdir -p ${TMP_DIR}
    mkdir -p ${TARGET_DIR}

    if [ "$MODE" = "swf_pre_drop" ]
    then
        mkdir -p ${TARGET_DIR}/BOMS
        cp ${BOM_DIR}/${BOM_LIST_NAME} ${TARGET_DIR}/BOMS/
        mkdir -p ${TARGET_DIR}/bom_lists
    fi
   # Result of BOM-files processing (if there're missed files RES=1)
    RES=0

    cp ${BOM_DIR}/${BOM_LIST_NAME} ${TMP_DIR}/

    while read CUR_BOM_FILE
    do
        echo_debug "CUR_BOM_FILE = $CUR_BOM_FILE"
        echo "CUR_BOM_FILE = $CUR_BOM_FILE" >> ${PACKAGE_LOG}
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
                            #$(replace_tags $BL_INSTALL)
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
                                    ;;
                                "swf_pre_drop")
                                    DROP_DEST_FILE=`echo ${BL_SOURCE} | sed -e "s/<deliverydir>\///"`""
                                    ;;
                            esac

                            echo_debug "DROP_DEST_FILE = $DROP_DEST_FILE"

                            case "${BL_LINK}" in
                                "<installroot>"*)
                                    DROP_SRC_FILE=`echo $BL_LINK | sed -e "s|<installroot>/||"`
                                    DROP_SRC_FILE="${WORKSPACE}/build/_install/${DROP_SRC_FILE}"
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
                                echo " NOK" >> ${PACKAGE_LOG}
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
#
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
    done < ${TMP_DIR}/${BOM_LIST_NAME}

    if [ "${ENABLE_VERBOSE}" = "yes" ]
    then
        if [ -f ${PACKAGE_LOG} ]
        then
            cat ${PACKAGE_LOG} | column -t | sed 's|^|\t|' | tee -a ${LOG_FILE}
            rm -f ${PACKAGE_LOG}
        fi
    fi
    echo_log "${BOM_SOURCE}... DONE"


    if [ $RES -eq 1 ]
    then
        echo_log "ERROR: packaging failed"
        exit 1
    fi
}

# Make archive package
make_package()
{
        echo_log "Create eng package..."
        rm -rf ${TMP_DIR}/${CCL_PACKAGE_NAME}
        rm -f ${TMP_DIR}/${CCL_PACKAGE_NAME}.tgz
        mkdir -p ${TMP_DIR}/${CCL_PACKAGE_NAME}
        cp -r ${PACKAGE_ENG_DIR}/* ${TMP_DIR}/${CCL_PACKAGE_NAME}

        cd ${TMP_DIR} && tar czf ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz ${CCL_PACKAGE_NAME} --owner=root --group=root
        echo_log "Package: ${WORKSPACE}/${CCL_PACKAGE_NAME}.tgz"
        echo_log "Create package... DONE"
}


#==============================================================================
#                              Parse arguments
#==============================================================================
parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "--eng-package"|"-eng-package")
                ENABLE_BUILD="yes"
                ENABLE_PACK="yes"
                ;;
            "-pack"|"--pack")
                ENABLE_PACK="yes"
                ;;
            "-build"|"--build")
                ENABLE_BUILD="yes"
                ;;
            "-swf-pre-drop"|"--swf-pre-drop")
                ENABLE_PRE_DROP="yes"
                ;;
            "-help")
                print_help
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
    echo_log "ENABLE_BUILD              = ${ENABLE_BUILD}"
    echo_log "ENABLE_PACK               = ${ENABLE_PACK}"
    echo_log "ENABLE_PRE_DROP           = ${ENABLE_PRE_DROP}"
    echo_log "-----------------------------------------------------------"

    TIMESTAMP_START=`date +%s`

}
#==============================================================================
#                             Preparing eng-package
#==============================================================================
run_build()
{
    if [ "${ENABLE_BUILD}" = "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPreparing eng-package..."
        echo_log_separator
        build
        prepare_staging package
        make_package
        echo_log_separator
        echo_log "#\t\t\teng-package... DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                             Building
#==============================================================================
run_build()
{
    if [ "${ENABLE_BUILD}" = "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tBuilding..."
        echo_log_separator
        build
        echo_log_separator
        echo_log "#\t\t\tBuilding... DONE"
        echo_log_separator
    fi
}
#==============================================================================
#                             Packaging
#==============================================================================
run_pack()
{
    if [ "${ENABLE_PACK}" = "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\tPackaging..."
        echo_log_separator
        prepare_staging package
        make_package
        echo_log_separator
        echo_log "#\t\t\tPackaging... DONE"
        echo_log_separator
    fi
}

#==============================================================================
#                              Performing pre-drop
#==============================================================================
run_pre_drop()
{
	echo_log_separator
	echo_log "#\t\t\tPerforming pre-drop..."
	echo_log_separator
	prepare_staging swf_pre_drop
	echo_log_separator
	echo_log "#\t\t\tPerforming pre-drop... DONE"
	echo_log_separator
}

#==============================================================================
#                              SWF pre-drop
#==============================================================================
run_swf_pre_drop()
{	
    if [ "${ENABLE_PRE_DROP}" == "yes" ]
    then
		echo_log_separator
		echo_log "#\t\t\tSWF pre-drop..."
		echo_log_separator
        cp -r ${PRE_DROP_DIR}/../../ ${SWF_PRE_DROP_DIR}
        rm -rf ${SWF_PRE_DROP_DIR}/SWF_Drops
        cp -R ${PRE_DROP_DIR}/../ ${SWF_PRE_DROP_DIR}
		echo_log_separator
		echo_log "#\t\t\tSWF pre-drop... DONE"
		echo_log_separator
    fi
}



#==============================================================================
#                              MAIN
#==============================================================================
set_default_values
parse_arguments $@
run_build
run_pack
run_pre_drop
run_swf_pre_drop
