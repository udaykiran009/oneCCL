#!/bin/bash
##########################################################################################
#IMPI# Title         : Simple shell support check for OneAPI scripts                     #
#IMPI#                                                                                   #
#IMPI# Implemented in: Intel(R) Collective Communication Library 2021.4 (Gold) Update  4 #
##########################################################################################

TOOLS_DIR="/nfs/inn/proj/mpi/pdsd/opt/tools"
SUPPORTED_SHELLS="bash ${TOOLS_DIR}/dash/bin/dash ${TOOLS_DIR}/ksh/bin/ksh ${TOOLS_DIR}/mksh/bin/mksh"

if [[ $(lsb_release -d) = *"Ubuntu"* ]]; then
    ub_majver=$(lsb_release -sr | grep -Eo '^[0-9]+')
    if [[ ${ub_majver} = "20" ]]; then
        SUPPORTED_SHELLS="${SUPPORTED_SHELLS} ${TOOLS_DIR}/zsh/u20/bin/zsh"
    else
        SUPPORTED_SHELLS="${SUPPORTED_SHELLS} ${TOOLS_DIR}/zsh/bin/zsh"
    fi
else
    SUPPORTED_SHELLS="${SUPPORTED_SHELLS} ${TOOLS_DIR}/zsh/bin/zsh"
fi

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

SCRIPT_NAME="vars_test.sh"

cleanup_and_exit()
{
    result=$1
    if [ "${result}" = "Pass" ]
    then
        rm ${TEST_LOG}
        echo "Pass"
        exit 0
    else
        shell=$2
        echo "Some errors occured in ${shell}"
        rm "${SCRIPT_NAME}"
        echo "Fail"
        exit 1
    fi
}

CCL_ROOT_TMP=${CCL_ROOT}

for shell in ${SUPPORTED_SHELLS}; do
    cat << EOF > ${SCRIPT_DIR}/${SCRIPT_NAME}
    . ${CCL_ROOT_TMP}/env/vars.sh
    echo SHELL=\$(ps -p "\$\$" -o comm=)
    echo EXP_ROOT=${CCL_ROOT_TMP}
    echo REAL_ROOT=\${CCL_ROOT}
    if [ \${CCL_ROOT} != ${CCL_ROOT_TMP} ]; then
        exit 1
    fi
    exit 0
EOF
    chmod +x ${SCRIPT_DIR}/${SCRIPT_NAME}
    export CCL_ROOT="" && ${shell} -c "${SCRIPT_DIR}/vars_test.sh" >> "${TEST_LOG}" 2>&1
    if [ $? -ne 0 ]; then
        cleanup_and_exit "Fail" "${shell}"
    fi 
done

rm "${SCRIPT_DIR}/${SCRIPT_NAME}"
cleanup_and_exit "Pass"