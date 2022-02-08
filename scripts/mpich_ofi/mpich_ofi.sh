#!/bin/bash

set_default_values()
{
    BASENAME=$(basename $0 .sh)
    SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
    USE_DEFAULT_MPICH_OFI="yes"
    MPICH_OFI_DEFAULT_LINK="https://af02p-or.devtools.intel.com/artifactory/list/mpich-aurora-or-local/"
    PACKAGE_NAME=""
}

print_help()
{
    echo "Usage:"
    echo "    ./${BASENAME}.sh <options>"
    echo ""
    echo "<options>:"
    echo "   ------------------------------------------------------------"
    echo "    -l|--link <url>"
    echo "        Specific package URL"
    echo "    -p|--package <path>"
    echo "        Specific package path"
    echo "   ------------------------------------------------------------"
    echo "    -h|-H|-help|--help"
    echo "        Print help information"
    echo ""
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "-help"|"--help"|"-h"|"-H")
                print_help
                exit 0
                ;;
            "-l"|"--link")
                PACKAGE_LINK=${2}
                USE_DEFAULT_MPICH_OFI="no"
                shift
                ;;
            "-p"|"--package")
                PACKAGE_PATH=${2}
                USE_DEFAULT_MPICH_OFI="no"
                shift
                ;;
            *)
                echo "ERROR: unknown option ($1)"
                echo "See ./update_runtimes.sh -help"
                exit 1
                ;;
        esac

        shift
    done

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "USE_DEFAULT_MPICH_OFI = ${USE_DEFAULT_MPICH_OFI}"
    echo "PACKAGE_LINK          = ${PACKAGE_LINK}"
    echo "PACKAGE_PATH          = ${PACKAGE_PATH}"
    echo "-----------------------------------------------------------"
}

load_package() {
    pushd ${SCRIPT_DIR}
    if [[ ${USE_DEFAULT_MPICH_OFI} = "yes" ]]
    then
        LINE="$(curl -s ${MPICH_OFI_DEFAULT_LINK} | grep "drop" | tail -1)"
        LATEST_DROP="$(echo "${LINE}" | grep -o 'drop[0-9]*.[0-9]' | tail -1)"
        PACKAGE_NAME="$(curl -s "${MPICH_OFI_DEFAULT_LINK}${LATEST_DROP}/ats/" | grep -o ">mpich-ofi.*icc-debug.*\.rpm" | cut -c 2-)"
        PACKAGE_LINK="${MPICH_OFI_DEFAULT_LINK}${LATEST_DROP}/ats/${PACKAGE_NAME}"

        echo "DEBUG: LINE         = ${LINE}"
        echo "DEBUG: LATEST_DROP  = ${LATEST_DROP}"
        echo "DEBUG: PACKAGE_NAME = ${PACKAGE_NAME}"
        echo "DEBUG: PACKAGE_LINK = ${PACKAGE_LINK}"
    fi
    if [[ ! -z ${PACKAGE_LINK} ]]
    then
        wget -nv ${PACKAGE_LINK}
        rc=$?
        if [[ ${rc} != 0 ]]
        then
            echo "ERROR: package loading failed"
            exit 1
        fi
    fi
    popd
}

install_package() {
    mkdir ${SCRIPT_DIR}/install
    pushd ${SCRIPT_DIR}/install
    if [[ -z ${PACKAGE_PATH} ]]
    then
        PACKAGE_PATH=${SCRIPT_DIR}/${PACKAGE_NAME}
    fi
    echo "DEBUG: PACKAGE_PATH = ${PACKAGE_PATH}"
    rpm2cpio ${PACKAGE_PATH} | cpio -idv
    popd
}

set_default_values
parse_arguments "$@"
load_package
install_package
