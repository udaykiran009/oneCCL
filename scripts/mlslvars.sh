#!/bin/sh

export MLSL_ROOT=MLSL_SUBSTITUTE_INSTALLDIR

if [ -z "${I_MPI_ROOT}" ]
then
    export I_MPI_ROOT="${MLSL_ROOT}"
fi

if [ -z "${PATH}" ]
then
    export PATH="${MLSL_ROOT}/intel64/bin"
else
    export PATH="${MLSL_ROOT}/intel64/bin:${PATH}"
fi

if [ -z "${LD_LIBRARY_PATH}" ]
then
    export LD_LIBRARY_PATH="${MLSL_ROOT}/intel64/lib"
else
    export LD_LIBRARY_PATH="${MLSL_ROOT}/intel64/lib:${LD_LIBRARY_PATH}"
fi

if [ -z "${PYTHONPATH}" ]
then
    export PYTHONPATH="${MLSL_ROOT}/intel64/include"
else
    export PYTHONPATH="${MLSL_ROOT}/intel64/include:${PYTHONPATH}"
fi
