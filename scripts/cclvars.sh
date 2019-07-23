#!/bin/bash

WORK_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd)"
export CCL_ROOT="$(cd ${WORK_DIR}/../../; pwd -P)"

INTERMEDIATE_DIR="intel64"

if [ -z "${PATH}" ]
then
    PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/bin"; export PATH
else
    PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/bin:${PATH}"; export PATH
fi

if [ -z "${LD_LIBRARY_PATH}" ]
then
    LD_LIBRARY_PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/lib64"; export LD_LIBRARY_PATH
else
    LD_LIBRARY_PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/lib64:${LD_LIBRARY_PATH}"; export LD_LIBRARY_PATH
fi

CCL_ATL_TRANSPORT_PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/lib64"; export CCL_ATL_TRANSPORT_PATH
FI_PROVIDER_PATH="${CCL_ROOT}/${INTERMEDIATE_DIR}/lib64/prov"; export FI_PROVIDER_PATH
