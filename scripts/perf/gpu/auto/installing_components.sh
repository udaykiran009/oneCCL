#!/bin/bash

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

set_default_values()
{
    PERF_WORK_DIR="/panfs/users/dsazanov/performance"
    PRODUCT_ABBREVIATION="mpi_oneapi"
}

# parse_parameters() {
#     while [ $# -gt 0 ]
#     do
#         key="$1"
#         case $key in
#             mpi)
#             export PRODUCT_ABBREVIATION="mpi_oneapi"; shift;
#             ;;
#             compiler)
#             export PRODUCT_ABBREVIATION="compiler"; shift;
#             ;;
#         esac
#     done
# }

info() {
    echo ""
    echo "   INFO: PRODUCT_ABBREVIATION   = ${PRODUCT_ABBREVIATION}"
    echo "   INFO: PERF_WORK_DIR          = ${PERF_WORK_DIR}"
    echo ""
}



installing_components() {
    INSTALL_DIR=${PERF_WORK_DIR}/oneapi/${PRODUCT_ABBREVIATION}
    echo "MESSAGE: ${PRODUCT_ABBREVIATION} installation begins"
    mkdir -p ${INSTALL_DIR}/tmp
    export HOME=${INSTALL_DIR}/tmp
    ${INSTALL_DIR}/*${PRODUCT_ABBREVIATION}*.sh -x -f ${INSTALL_DIR}/tmp
    ${INSTALL_DIR}/tmp/*${PRODUCT_ABBREVIATION}*/install.sh -s --eula=accept --action=install --install-dir ${INSTALL_DIR}/_install --log-dir ${INSTALL_DIR}/tmp/log_dir_${PRODUCT_ABBREVIATION}
    CheckCommandExitCode $? "  ERROR: Installation components"
    rm -rf ${INSTALL_DIR}/tmp
    echo "MESSAGE: ${PRODUCT_ABBREVIATION} installation: SUCCESS"
}

set_default_values

    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            mpi_oneapi)
            export PRODUCT_ABBREVIATION="mpi_oneapi"; shift;
            ;;
            ATS)
            export PERF_WORK_DIR="/home/gta/dsazanov/performance"; shift;
            ;;
            DG1)
            export PERF_WORK_DIR="/panfs/users/dsazanov/performance"; shift;
            ;;
            compiler)
            export PRODUCT_ABBREVIATION="compiler"; shift;
            ;;
        esac
    done

#parse_parameters
info
installing_components
