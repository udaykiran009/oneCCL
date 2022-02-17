#!/bin/bash

set_default_values()
{
    BASENAME=$(basename $0 .sh)
    SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
    LIBFABRIC_LINK="https://github.com/ofiwg/libfabric.git"
    CURRENT_WORK_DIR=$(realpath ${SCRIPT_DIR}/../../)
    source ${CURRENT_WORK_DIR}/scripts/utils/common.sh
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
    echo "    -h|-H|-help|--help"
    echo "        Print help information"
    echo ""
}

parse_arguments()
{
    PACKAGE_LINK=${LIBFABRIC_LINK}
    while [ $# -ne 0 ]
    do
        case $1 in
            "-help"|"--help"|"-h"|"-H")
                print_help
                exit 0
                ;;
            "-l"|"--link")
                PACKAGE_LINK=${2}
                shift
                ;;
            *)
                echo "ERROR: unknown option ($1)"
                echo "See ./${BASENAME}.sh -help"
                exit 1
                ;;
        esac

        shift
    done

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "PACKAGE_LINK          = ${PACKAGE_LINK}"
    echo "-----------------------------------------------------------"
}

install_package() {
    pushd ${SCRIPT_DIR}
    git clone ${PACKAGE_LINK} libfabric_src
    check_command_exit_code $? "git clone failed"
    cd libfabric_src
    ./autogen.sh
    check_command_exit_code $? "autogen.sh script failed"
    ./configure --prefix=${SCRIPT_DIR}/libfabric --enable-dmabuf_peer_mem=dl --enable-verbs=dl --with-ze=yes --enable-ze-dlopen=yes
    check_command_exit_code $? "configure script failed"
    make -j install
    check_command_exit_code $? "installation failed"
    popd
}

set_default_values
parse_arguments "$@"
install_package
