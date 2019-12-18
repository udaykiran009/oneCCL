#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
BASENAME=`basename $0`
USERNAME=`whoami`

SILENT_MODE=0

if [ "$USERNAME" = "root" ]
then
    DEFAULT_INSTALL_PATH="/opt/intel/CCL_SUBSTITUTE_FULL_VERSION"
else
    DEFAULT_INSTALL_PATH="${HOME}/intel/CCL_SUBSTITUTE_FULL_VERSION"
fi

print_help()
{
    echo "Usage: $BASENAME [-d <install_directory>][-h][-s]"
    echo " -d <install_directory> : install into the specified directory"
    echo " -s                     : install silently"
    echo " -h                     : print help"
}

while [ $# -ne 0 ]
do
    case $1 in
        '-d')
            INSTALL_PATH="$2"
            shift
            ;;
        '-s')
            SILENT_MODE=1
            ;;
        '-h')
            print_help
            exit 0
            ;;
        *)
            echo "ERROR: unknown option ($1)"
            print_help
            exit 1
            ;;
    esac
    
    shift
done

if [ ${SILENT_MODE} -eq 0 ]
then
    echo "oneCCL CCL_SUBSTITUTE_OFFICIAL_VERSION for Linux* OS will be installed"
    echo "Type 'y' to continue or 'q' to exit and then press Enter"
    CONFIRMED=0
    while [ $CONFIRMED = 0 ]
    do
        read -e ANSWER
        case $ANSWER in
            'q')
                exit 0
                ;;
            'y')
                CONFIRMED=1
                ;;
            *)
                ;;
        esac
    done
fi

if [ -z "${INSTALL_PATH}" ]
then
    echo "Please specify the installation directory or press Enter to install into the default path [${DEFAULT_INSTALL_PATH}]:"
    read -e INSTALL_PATH

    if [ -z "${INSTALL_PATH}" ]
    then
        INSTALL_PATH=${DEFAULT_INSTALL_PATH}
    fi
fi

if [ ${SILENT_MODE} -eq 0 ]
then
    echo "oneCCL CCL_SUBSTITUTE_OFFICIAL_VERSION for Linux* OS will be installed into ${INSTALL_PATH}"
fi

if [ -d ${INSTALL_PATH} ]
then
    if [ ${SILENT_MODE} -eq 0 ]
    then
        echo "WARNING: ${INSTALL_PATH} exists and will be removed"
    fi
    
    rm -rf ${INSTALL_PATH}
    if [ $? -ne 0 ]
    then
        echo "ERROR: unable to clean ${INSTALL_PATH}"
        echo "Please verify the folder permissions and check it isn't in use"
        exit 1
    fi
fi

mkdir -p ${INSTALL_PATH}
if [ $? -ne 0 ]
then
    echo "ERROR: unable to create ${INSTALL_PATH}"
    echo "Please verify the folder permissions"
    exit 1
fi

cd ${INSTALL_PATH} && tar xfzm ${SCRIPT_DIR}/files.tar.gz
if [ $? -ne 0 ]
then
    echo "ERROR: unable to unpack ${SCRIPT_DIR}/files.tar.gz"
    exit 1
fi

