#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define WORK_DIR"
    exit 1
fi

. ${SCRIPT_DIR}/settings.sh

BASENAME=`basename $0 .sh`
LOG_FILE="${LOG_DIR}/${BASENAME}_$$.log"
WORK_DIR=`cd ${SCRIPT_DIR}/../.. && pwd -P`
HOSTNAME=`hostname -s`

if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
then
    export I_MPI_HYDRA_HOST_FILE=${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
fi

if [ -z "${MLSL_ROOT}" ]
then
    MLSL_PATH="${WORK_DIR}/_install"
    . ${MLSL_PATH}/intel64/bin/mlslvars.sh
fi

PASS_COUNTER=0
FAIL_COUNTER=0
TOTAL_COUNTER=0

echo "Log file: ${LOG_FILE}"

rm -f ${LOG_FILE}

cd ${WORK_DIR}

echo "Testing DL_MSL..." | tee -a ${LOG_FILE}

echo "Data parallelism..." | tee -a ${LOG_FILE}
mpiexec.hydra -ppn ${PPN} -n ${N} ${WORK_DIR}/tests/mlsl_test 1 | tee -a ${LOG_FILE} 2>&1
if [ $? -ne 0 ]
then
    echo "Data parallelism... NOK" | tee -a ${LOG_FILE}
    FAIL_COUNTER=`expr ${FAIL_COUNTER} + 1`
else
    echo "Data parallelism... OK" | tee -a ${LOG_FILE}
    PASS_COUNTER=`expr ${PASS_COUNTER} + 1`
fi
TOTAL_COUNTER=`expr ${TOTAL_COUNTER} + 1`

echo "Model parallelism..." | tee -a ${LOG_FILE}
mpiexec.hydra -ppn ${PPN} -n ${N} ${WORK_DIR}/tests/mlsl_test ${N} | tee -a ${LOG_FILE} 2>&1
if [ $? -ne 0 ]
then
    echo "Model parallelism... NOK" | tee -a ${LOG_FILE}
    FAIL_COUNTER=`expr ${FAIL_COUNTER} + 1`
else
    echo "Model parallelism... OK" | tee -a ${LOG_FILE}
    PASS_COUNTER=`expr ${PASS_COUNTER} + 1`
fi
TOTAL_COUNTER=`expr ${TOTAL_COUNTER} + 1`

echo "Hybrid parallelism (N/2) x 2..." | tee -a ${LOG_FILE}
mpiexec.hydra -ppn ${PPN} -n ${N} ${WORK_DIR}/tests/mlsl_test 2 | tee -a ${LOG_FILE} 2>&1
if [ $? -ne 0 ]
then
    echo "Hybrid parallelism (N/2) x 2... NOK" | tee -a ${LOG_FILE}
    FAIL_COUNTER=`expr ${FAIL_COUNTER} + 1`
else
    echo "Hybrid parallelism (N/2) x 2... OK" | tee -a ${LOG_FILE}
    PASS_COUNTER=`expr ${PASS_COUNTER} + 1`
fi
TOTAL_COUNTER=`expr ${TOTAL_COUNTER} + 1`

echo "Testing DL_MSL... DONE" | tee -a ${LOG_FILE}

echo "PASS: ${PASS_COUNTER}"
echo "FAIL: ${FAIL_COUNTER}"
echo "TOTAL: ${TOTAL_COUNTER}"

echo "Log file: ${LOG_FILE}"

if [ ${FAIL_COUNTER} -ne 0 ]
then
    echo "Testing DL_MSL... NOK" | tee -a ${LOG_FILE}
else
    echo "Testing DL_MSL... OK" | tee -a ${LOG_FILE}
fi
