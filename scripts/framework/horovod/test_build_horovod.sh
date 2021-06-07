#!/bin/bash -x

CONDA_DEFAULT_ENV_NAME="test_horovod"
TF_LINK="http://mlpc.intel.com/downloads/gpu/acceptance/ww23/compiler-20210527/itex/ubuntu/tensorflow-2.5.0-cp37-cp37m-linux_x86_64.whl"
ITEX_LINK="http://mlpc.intel.com/downloads/gpu/acceptance/ww23/compiler-20210527/itex/ubuntu/intel_extension_for_tensorflow-0.1.0-cp37-cp37m-linux_x86_64.whl"

TF_NAME=`basename $TF_LINK`
ITEX_NAME=`basename $ITEX_LINK`

BASENAME=`basename $0 .sh`

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

CheckCommandExitCode()
{
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        clean_env
        echo_log "TEST FAILED"
        exit $1
    fi
}

print_help()
{
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  --tf <path>"
    echo_log "  --itex <path>"
    echo_log ""
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh "
    echo_log "  ${BASENAME}.sh --tf /nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Software/Tensorflow/latest"
    echo_log "  ${BASENAME}.sh --itex /nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Software/ITEX/latest"
    echo_log ""
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")                  
                print_help
                exit 0 
                ;;
            "--tf" | "-tf")                         
                TF_NAME=${2}
                DOWNLOAD_TF="no"
                shift 
                ;;
            "--itex" | "-itex")                       
                ITEX_NAME=${2}
                DOWNLOAD_ITEX="no"
                shift
                ;;
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done


    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "ITEX_NAME          = ${ITEX_NAME}"
    echo_log "TF_NAME            = ${TF_NAME}"
}

echo_log()
{
    echo -e "$*"
}

tf_test() {
    echo_log "===================================== Basic TF test ==========================================="
    echo_log "python -c \"import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())\""
    python -c "import tensorflow as tf; print(tf.__version__); print(tf.sysconfig.get_include())"
    CheckCommandExitCode $? "Basic TF test failed"
    echo_log "===================================== ************* ==========================================="
}

hvd_test() {
    echo_log "============================= Basic Horovod test =================================="
    echo_log "python -c \"import horovod.tensorflow as hvd; hvd.init(); print(hvd.local_rank())\""
    python -c "import horovod.tensorflow as hvd; hvd.init(); print(hvd.local_rank())"
    CheckCommandExitCode $? "Basic Horovod test failed"
    echo_log "============================= ****************** =================================="
}

prepare_env() {
    mkdir -p tmp
    pushd tmp

    if [[ ${DOWNLOAD_TF} != "no" ]]; then
        wget -e use_proxy=no ${TF_LINK}
    fi
    if [[ ${DOWNLOAD_ITEX} != "no" ]]; then
        wget -e use_proxy=no ${ITEX_LINK}
    fi

    conda env list | grep ${CONDA_DEFAULT_ENV_NAME} || rc=$?
    if [[ $rc -ne 0 ]]; then
        CONDA_ENV_NAME="${CONDA_DEFAULT_ENV_NAME}_0"
        conda create -y -n "${CONDA_ENV_NAME}" python=3.7
    else
        id=$(conda env list | grep ${CONDA_DEFAULT_ENV_NAME} | tail -1 | cut -d "_" -f3 | cut -d " " -f1)
        new_id=$((id+1))
        CONDA_ENV_NAME="${CONDA_DEFAULT_ENV_NAME}_${new_id}"
        conda create -y -n ${CONDA_ENV_NAME} python=3.7
    fi

    CONDA_BIN_DIR=$(dirname $(which python))
    source ${CONDA_BIN_DIR}/activate ${CONDA_ENV_NAME}

    python3 -m pip install --upgrade pip

    pip install $(realpath ${TF_NAME}) $(realpath ${ITEX_NAME})

    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CONDA_PREFIX}/lib"

    tf_test
    CheckCommandExitCode $? "TF test failed"
    popd
    rm -rf tmp
}

clean_env() {
    conda deactivate
    conda env remove -y --name ${CONDA_ENV_NAME}
}

build_horovod()
{
    if [[ ! -d ${HVD_DPCPP_DIR} ]]; then
        echo "ERROR: the value of HVD_DPCPP_DIR variable is missing"
        CheckCommandExitCode 1 "Build Horovod failed"
    fi

    export I_MPI_CXX=dpcpp
    export CXX=mpicxx
    export LDSHARED="dpcpp -shared -fPIC"
    export CC=clang
    export HOROVOD_GPU=DPCPP
    export HOROVOD_GPU_OPERATIONS=CCL

    pushd ${HVD_DPCPP_DIR}

    export HOROVOD_WITH_MPI=1
    export HOROVOD_CPU_OPERATIONS=CCL
    export HOROVOD_WITHOUT_MXNET=1
    export HOROVOD_WITHOUT_PYTORCH=1
    export HOROVOD_WITH_TENSORFLOW=1
    export HOROVOD_WITHOUT_GLOO=1

    python setup.py install
    CheckCommandExitCode $? "Build Horovod failed"

    popd

    hvd_test
    CheckCommandExitCode $? "Horovod test failed"
}

parse_arguments $@
prepare_env
build_horovod
clean_env

echo_log "TEST PASSED"
