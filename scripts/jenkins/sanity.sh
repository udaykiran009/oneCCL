#!/bin/bash

echo "DEBUG: GIT_BRANCH = ${GIT_BRANCH}"
echo "DEBUG: MLSL_BUILD_ID = ${MLSL_BUILD_ID}"
echo "DEBUG: coverage = ${coverage}"

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

. ${SCRIPT_DIR}/settings.sh

BASENAME=`basename $0 .sh`
WORK_DIR=`cd ${SCRIPT_DIR}/../.. && pwd -P`
MLSL_INSTALL_DIR="${WORK_DIR}/_install"
HOSTNAME=`hostname -s`

#TODO: Fix it
if [ "$os" = "rhel6.7" ]
then
    export PATH_TO_QUANT_LIB=""
else
    export PATH_TO_QUANT_LIB="/p/pdsd/scratch/Software/DL_COMPRESS_LIB/libdlcomp.so"
fi

if [ "$coverage" = "true" ] && [ "$compiler" != "gcc" ]
then
    export CODECOV=1
    
    if [ -z "${MLSL_BUILD_ID}" ]
    then
        export TMP_COVERAGE_DIR=${WORK_DIR}/coverage
    else
        export TMP_COVERAGE_DIR=${MLSL_REPO_DIR}/${MLSL_BUILD_ID}/coverage
        export CODECOV_SRCROOT=${WORK_DIR}
    fi
fi

CURRENT_MLSL_BUILD_DIR="${MLSL_REPO_DIR}/${MLSL_BUILD_ID}"
CURRENT_MLSL_BUILD=`cd ${CURRENT_MLSL_BUILD_DIR} && ls -1 *.tgz 2>/dev/null | head -1`
CURRENT_MLSL_BUILD_BASENAME=`basename ${CURRENT_MLSL_BUILD} .tgz`

echo "DEBUG: CURRENT_MLSL_BUILD_DIR = $CURRENT_MLSL_BUILD_DIR"
echo "DEBUG: CURRENT_MLSL_BUILD = $CURRENT_MLSL_BUILD"
echo "DEBUG: CURRENT_MLSL_BUILD_BASENAME = $CURRENT_MLSL_BUILD_BASENAME"

if [ -f ${CURRENT_MLSL_BUILD_DIR}/${CURRENT_MLSL_BUILD} ]
then
    tar xfmz ${CURRENT_MLSL_BUILD_DIR}/${CURRENT_MLSL_BUILD}
    if [ $? -ne 0 ]
    then
        echo "ERROR: tar failed"
        exit 1
    fi

    cd ${WORK_DIR}/${CURRENT_MLSL_BUILD_BASENAME} && ./install.sh -s -d ${MLSL_INSTALL_DIR}
    if [ $? -ne 0 ]
    then
        echo "ERROR: install.sh failed"
        exit 1
    fi
    
    cd - > /dev/null
fi

# 'compiler' is set by Jenkins
case $compiler in
    icc16.0.4)
        . /p/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2016.4.258/linux/bin/compilervars.sh intel64
        COMPILER=intel
        ;;
    icc17.0.4)
        . /p/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2017.4.196/linux/bin/compilervars.sh intel64
        COMPILER=intel
        ;;
    gcc)
        . /p/pdsd/opt/EM64T-LIN/intel/mkl/bin/mklvars.sh intel64
        COMPILER=gnu
        ;;
    *)
        echo "WARNING: compiler isn't specified - icc17.0.4 will be used"
        . /p/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2017.4.196/linux/bin/compilervars.sh intel64
        COMPILER=intel
esac

if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
then
    if [ -f ${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts ]
    then
        export I_MPI_HYDRA_HOST_FILE=${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
    else
        echo "WARNING: hostfile (${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts) isn't available"
        echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set"
    fi
fi

if [ -z "${MLSL_ROOT}" ]
then
    echo "WARNING: MLSL_ROOT isn't set"
    if [ -f ${MLSL_INSTALL_DIR}/intel64/bin/mlslvars.sh ]
    then
        . ${MLSL_INSTALL_DIR}/intel64/bin/mlslvars.sh
    else
        echo "ERROR: ${MLSL_INSTALL_DIR}/intel64/bin/mlslvars.sh doesn't exist"
        exit 1
    fi
fi

#export I_MPI_JOB_TIMEOUT=400

if [ -n "${TESTING_ENVIRONMENT}" ]
then
    for line in ${TESTING_ENVIRONMENT}
    do
	export "$line"
    done
fi

pwd
rm -rf ${WORK_DIR}/_log
mkdir -p ${WORK_DIR}/_log
echo "make testing..."
make COMPILER=${COMPILER} testing > >(tee -a ${WORK_DIR}/_log/out.log) 2> >(tee -a ${WORK_DIR}/_log/err.log >&2)
testing_status=${PIPESTATUS[0]}
grep -r "FAILED" ${WORK_DIR}/tests/examples/*/*.log > /dev/null 2>&1
grep_status_fail=${PIPESTATUS[0]}
grep -r 'Run FAILED\|Make FAILED' ${WORK_DIR}/_log/*.log > /dev/null 2>&1
log_status_fail=${PIPESTATUS[0]}

if [ "$testing_status" -eq 0 ] && [ "$grep_status_fail" -ne 0 ] && [ "$log_status_fail" -ne 0 ]
then
    echo "make testing... OK"
else
    echo "make testing... NOK"
    exit 1
fi
cd ${WORK_DIR}
if [ "$coverage" = "true" ] && [ "$compiler" != "gcc" ]
then
    # Do not create ep_server processes in case of single node
    export MLSL_CHECK_SINGLE_NODE=1

    echo "Code Coverage"
    make COMPILER=intel codecov
    if [ $? -ne 0 ]
    then
        echo "make codecov... NOK"
        exit 1
    else
        echo "make codecov... OK"
    fi
fi
