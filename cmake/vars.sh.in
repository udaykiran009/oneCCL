#!/bin/bash

WORK_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd)"
export CCL_ROOT="$(cd ${WORK_DIR}/../; pwd -P)"

args=$*
for arg in $args
do
    case "$arg" in
        --ccl-configuration=*)
            ccl_configuration="${arg#*=}"
            ;;
    esac
done

case "$ccl_configuration" in
    cpu_gpu_dpcpp|cpu_icc)
        ;;
    *)
        ccl_configuration="cpu_gpu_dpcpp"
        ;;
esac

if [ -z "${LD_LIBRARY_PATH}" ]
then
    LD_LIBRARY_PATH="${CCL_ROOT}/lib/${ccl_configuration}"; export LD_LIBRARY_PATH
else
    LD_LIBRARY_PATH="${CCL_ROOT}/lib/${ccl_configuration}:${LD_LIBRARY_PATH}"; export LD_LIBRARY_PATH
fi

CCL_ATL_TRANSPORT_PATH="${CCL_ROOT}/lib/${ccl_configuration}"; export CCL_ATL_TRANSPORT_PATH
