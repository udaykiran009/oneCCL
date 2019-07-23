#!/bin/bash

COMPILER_PATH="/p/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2017.4.196"

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

TMP_DIR="/tmp"
LOG_DIR="${TMP_DIR}"

# Building
if [ -z "$BUILD_TYPE" ]
then
    BUILD_TYPE="sanity"
fi

CCL_REPO_DIR="/p/pdsd/scratch/jenkins/builds/${BUILD_TYPE}"

# Testing
#N=4
N=3
PPN=1
HOSTS="nnlmpihsw05,nnlmpihsw06,nnlmpihsw07,nnlmpihsw08"
#HOSTS="nnlmpiwsm02,nnlmpiwsm03"
#HOSTS="nnlmpibdw07,nnlmpibdw08"

# Klocwork
NEED_UPLOAD_RES_TO_KW_SERVER="yes"
# export PATH="$PATH:/localdisk/sys_ct/kw/10.4/bin"
export PATH="$PATH:/localdisk/sys_ct/kw/2016.3_server/bin"
KW_SERVER_HOST="klocwork-igk5.devtools.intel.com"
KW_SERVER_PORT="8095"
