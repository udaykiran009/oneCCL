#!/bin/bash

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

echo_log()
{
    msg=$1
    date=`date`
    echo "[$date] > $msg"
}


set_default_values()
{
    WDIR="/panfs/users/dsazanov/performance"
    PRODUCT_ABBREVIATION="mpi_oneapi"
}

info() {
    echo ""
    echo "   INFO: PRODUCT_ABBREVIATION   = ${PRODUCT_ABBREVIATION}"
    echo "   INFO: WDIR                   = ${WDIR}"
    echo ""
}

installing_components() {
    INSTALL_DIR=${WDIR}/oneapi/${PRODUCT_ABBREVIATION}
    echo_log "MESSAGE: ${PRODUCT_ABBREVIATION} installation begins"
    mkdir -p ${INSTALL_DIR}/tmp
    export HOME=${INSTALL_DIR}/tmp
    ${INSTALL_DIR}/*${PRODUCT_ABBREVIATION}*.sh -x -f ${INSTALL_DIR}/tmp
    ${INSTALL_DIR}/tmp/*${PRODUCT_ABBREVIATION}*/install.sh -s --eula=accept --action=install --install-dir ${INSTALL_DIR}/_install --log-dir ${INSTALL_DIR}/tmp/log_dir_${PRODUCT_ABBREVIATION}
    CheckCommandExitCode $? "  ERROR: Installation components"
    rm -rf ${INSTALL_DIR}/tmp
    echo_log "MESSAGE: ${PRODUCT_ABBREVIATION} installation: SUCCESS"
}

parse_parameters()
{
    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            mpi_oneapi)
            export PRODUCT_ABBREVIATION="mpi_oneapi"; shift;
            ;;
            compiler)
            export PRODUCT_ABBREVIATION="compiler"; shift;
            ;;
            -WDIR)
            export WDIR="$2"; shift; shift;
            ;;
        esac
    done
}

#==============================================
#==============================================
#==============================================

set_default_values
parse_parameters $@
info
installing_components
