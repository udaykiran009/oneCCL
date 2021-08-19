#!/bin/bash

#########################################################################################
#CCL# Title         : Simple pkg-config support checker                                 #
#CCL# Tracker ID    : INFRA-1459                                                        #
#CCL#                                                                                   #
#CCL# Implemented in: Intel(R) Collective Communication Library 2021.4 (Gold) Update  4 #
#########################################################################################

BASENAME="`basename $0 .sh`"
TEST_LOG="${BASENAME}.log"
TESTFILE="${BASENAME}.cpp"
BINFILE="${BASENAME}"

${BINFILE} > ${TEST_LOG} 2>&1
rc=$?

if [[ $rc -ne 0 ]]; then
    echo "fail"
    exit 1
else
    rm -rf ${BINFILE} ${TEST_LOG}
    echo "Pass"
fi

