#!/bin/bash

print_log()
{
    msg=$1
    date=`date`
    echo "[$date] > $msg"
}

set_default_values()
{
    if [ -z ${IS_SLURM} ]
    then
        work_dir="/p/pdsd/Users/dsazanov/ccl_pytorch/PT"
        imagenet_dir="/p/pdsd/scratch/Uploads/ccl_pytorch/pytorch-tiny-imagenet/tiny-imagenet-200"
        anaconda_install_path=${work_dir}/anaconda
        jemalloc_install_path=${work_dir}/jemalloc/_install
        checkpoint_dir=${work_dir}/checkpoint
        PPN="2"
        EPOCHS="1" #90
        WORKERS="2" #4
        BATCH_SIZE="64"

        DOWNLOAD_ANACONDA="0"
        DOWNLOAD_PYTORCH="0"
        DOWNLOAD_IPEX="0"
        DOWNLOAD_TORCHCCL="0"
        DOWNLOAD_JEMALLOC="0"
        DOWNLOAD_VISION="0"
        DOWNLOAD_IMAGENET="0"
        DOWNLOAD_CCL="0"
    fi

    hostfile=${work_dir}/hostfile
    first_host=`cat $hostfile | head -n 1`
    masterip=${work_dir}/masterip; ssh ${first_host} "hostname -I" > $masterip
    master_ip=$(cat $masterip | awk '{print $2}')
    N=`cat $hostfile | wc -l`

    log_path=${work_dir}/log.txt
    ENV_NAME="pytorch_jenkins"

    BUILD_ANACONDA="0"
    BUILD_PYTORCH="0"
    BUILD_IPEX="0"
    BUILD_TORCHCCL="0"
    BUILD_JEMALLOC="0"
    BUILD_VISION="0"
    BUILD_IMAGENET="0"
    BUILD_CCL="0"
}

parse_parameters()
{
    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            -work_dir)
            work_dir="$2"; shift; shift;
            ;;
            -imagenet_dir)
            imagenet_dir="$2"; shift; shift;
            ;;
            -anaconda_install_path)
            anaconda_install_path="$2"; shift; shift;
            ;;
            -jemalloc_install_path)
            jemalloc_install_path="$2"; shift; shift;
            ;;
            -checkpoint_dir)
            checkpoint_dir="$2"; shift; shift;
            ;;
            -ppn)
            PPN="$2"; shift; shift;
            ;;
            -epochs)
            EPOCHS="$2"; shift; shift;
            ;;
            -workers)
            WORKERS="$2"; shift; shift;
            ;;
            -batch_size)
            BATCH_SIZE="$2"; shift; shift;
            ;;
            -log_path)
            log_path="$2"; shift; shift;
            ;;
            -dl_anaconda)
            DOWNLOAD_ANACONDA="1"; shift
            ;;
            -dl_pytorch)
            DOWNLOAD_PYTORCH="1"; shift
            ;;
            -dl_ipex)
            DOWNLOAD_IPEX="1"; shift
            ;;
            -dl_torchccl)
            DOWNLOAD_TORCHCCL="1"; shift
            ;;
            -dl_jemalloc)
            DOWNLOAD_JEMALLOC="1"; shift
            ;;
            -dl_vision)
            DOWNLOAD_VISION="1"; shift
            ;;
            -dl_ccl)
            DOWNLOAD_CCL="1"; shift
            ;;
            -*|--*)
            echo_log "  ERROR: Unsupported flag $key"; exit 1
            ;;
        esac
    done
}

print_env()
{
    ccl_env=`env | grep CCL_`
    impi_env=`env | grep I_MPI_`
    omp_env=`env | grep "OMP_\|KMP_" `

    print_log "CCL env:"
    print_log "${ccl_env}"

    print_log "IMPI env:"
    print_log "${impi_env}"

    print_log "OMP env:"
    print_log "${omp_env}"
}

remove_dir()
{
    dir=$1
    if [ "$dir" != "" ] && [ -d $dir ]
    then
        print_log "remove dir $dir"
        rm -rf $dir
    fi
}

clone_code()
{
    repo=$1
    branch=$2
    if [ "$branch" == "" ]
    then
        branch="master"
    fi
    print_log "clone repo $repo, branch $branch"
    git clone --single-branch --branch ${branch} ${repo}
}

cleanup_hosts()
{
    hostfile=$1
    mapfile -t hostlist < $hostfile

    echo "clean up hosts"
    for host in "${hostlist[@]}"
    do
        echo "host ${host}"
        cmd="killall -9 python mpiexec.hydra pmi_proxy"
        ssh ${host} $cmd
    done
}

check()
{
    print_log "checking log file..."

    file=$1
    unexpected_str="unsupported|error|invalid|Aborted|fail|failed|^bad$|killed|^fault$|runtime_error|terminate"
    expected_strs="CCL_WORKER_COUNT: training_started training_completed"

    count=`grep -E -c -i "${unexpected_str}" ${file}`
    if [ ${count} -ne 0 ]
    then
        print_log "PyTorch test failed, found ${count} unexpected string"
        print_log "See ${file} for details"
        exit 1
    fi

    for str in $expected_strs
    do
        count=`grep -E -c -i "${str}" ${file}`
        if [ ${count} -eq 0 ]
        then
            print_log "PyTorch test failed, did not find expected string (${str})"
            print_log "See ${file} for details"
            exit 1
        fi
    done

    print_log "PyTorch test passed"
}

print_parameters()
{
    print_log "DOWNLOAD_ANACONDA = ${DOWNLOAD_ANACONDA}"
    print_log "DOWNLOAD_PYTORCH = ${DOWNLOAD_PYTORCH}"
    print_log "DOWNLOAD_IPEX = ${DOWNLOAD_IPEX}"
    print_log "DOWNLOAD_TORCHCCL = ${DOWNLOAD_TORCHCCL}"
    print_log "DOWNLOAD_JEMALLOC = ${DOWNLOAD_JEMALLOC}"
    print_log "DOWNLOAD_VISION = ${DOWNLOAD_VISION}"
    print_log "DOWNLOAD_IMAGENET = ${DOWNLOAD_IMAGENET}"
    print_log "DOWNLOAD_CCL = ${DOWNLOAD_CCL}"
    print_log ""
    print_log "BUILD_ANACONDA = ${BUILD_ANACONDA}"
    print_log "BUILD_PYTORCH = ${BUILD_PYTORCH}"
    print_log "BUILD_IPEX = ${BUILD_IPEX}"
    print_log "BUILD_TORCHCCL = ${BUILD_TORCHCCL}"
    print_log "BUILD_JEMALLOC = ${BUILD_JEMALLOC}"
    print_log "BUILD_VISION = ${BUILD_VISION}"
    print_log "BUILD_IMAGENET = ${BUILD_IMAGENET}"
    print_log "BUILD_CCL = ${BUILD_CCL}"
    print_log ""
    print_log "WORK_DIR                       = ${work_dir}"
    print_log "IMAGENET_DIR                   = ${imagenet_dir}"
    print_log "ANACONDA_INSTALL_PATH          = ${anaconda_install_path}"
    print_log "JEMALLOC_INSTALL_PATH          = ${jemalloc_install_path}"
    print_log "CHECKPOINT_DIR                 = ${checkpoint_dir}"
    print_log "WORKERS                        = ${WORKERS}"
    print_log "EPOCHS                         = ${EPOCHS}"
    print_log "N_NODES                        = ${N}"
    print_log "PPN                            = ${PPN}"
    print_log "LOG_PATH                       = ${log_path}"
    print_log "MASTER_IP                      = ${master_ip}"
    print_log ""
}

set_env()
{
    export PATH=${anaconda_install_path}/bin:${PATH}
    compiler_path="/p/pdsd/opt/EM64T-LIN/intel/compilers_and_libraries_2019.1.144/linux/"
    source ${compiler_path}/bin/compilervars.sh intel64
    export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
    GCC_PATH=/p/pdsd/opt/EM64T-LIN/compilers/gnu/gcc-8.3.0
    export PATH=${GCC_PATH}/bin:${PATH}
    export LD_LIBRARY_PATH="${GCC_PATH}/lib64:${GCC_PATH}/lib":${LIB_LIBRARY_PATH}
    export LD_LIBRARY_PATH=/usr/lib64/psm2-compat:${LD_LIBRARY_PATH}
    export CC=gcc
    export CXX=g++
    export USE_NATIVE_ARCH=ON
    export USE_ROCM=0
    export REL_WITH_DEB_INFO=1
    export USE_CUDA=0
    export CCL_LOG_LEVEL=1
    export I_MPI_DEBUG=12
    export DNNL_PRIMITIVE_CACHE_CAPACITY=1024
    export MALLOC_CONF="oversize_threshold:1,background_thread:true,metadata_thp:auto,dirty_decay_ms:9000000000,muzzy_decay_ms:9000000000"
}

adjust_parameters()
{
    if [ "${DOWNLOAD_ANACONDA}" == "1" ]
    then
        BUILD_ANACONDA=1
    fi

    if [ "${DOWNLOAD_PYTORCH}" == "1" ]
    then
        BUILD_PYTORCH=1
        DOWNLOAD_IPEX=1
    fi

    if [ "${DOWNLOAD_IPEX}" == "1" ]
    then
        BUILD_IPEX=1
    fi

    if [ "${DOWNLOAD_TORCHCCL}" == "1" ]
    then
        BUILD_TORCHCCL=1
    fi

    if [ "${DOWNLOAD_JEMALLOC}" == "1" ]
    then
        BUILD_JEMALLOC=1
    fi

    if [ "${DOWNLOAD_VISION}" == "1" ]
    then
        BUILD_VISION=1
    fi

    if [ "${DOWNLOAD_IMAGENET}" == "1" ]
    then
        BUILD_IMAGENET=1
    fi

    if [ "${DOWNLOAD_CCL}" == "1" ]
    then
        BUILD_CCL=1
    fi
}

download_and_build()
{
    cd ${work_dir}

    if [ "${DOWNLOAD_ANACONDA}" == "1" ]
    then
        print_log "download anaconda"
        rm anaconda3.sh
        wget https://repo.continuum.io/archive/Anaconda3-5.0.0-Linux-x86_64.sh -O anaconda3.sh
        chmod +x anaconda3.sh  
        ${work_dir}/anaconda3.sh -b -p ${anaconda_install_path}
    fi

    if [ "${BUILD_ANACONDA}" == "1" ]
    then
        print_log "build anaconda"

        print_log "removing env ${ENV_NAME}"
        ${anaconda_install_path}/bin/conda remove -y --name ${ENV_NAME} --all

        print_log "creating env ${ENV_NAME}"
        ${anaconda_install_path}/bin/conda create -yn ${ENV_NAME} python=3.7

        print_log "install python packages"
        conda config --add channels intel
        conda install ninja pyyaml setuptools cmake cffi typing intel-openmp
        pip install sklearn
        conda install mkl mkl-include numpy -c intel --no-update-deps
    fi

    print_log "activate ${ENV_NAME}"
    source ${anaconda_install_path}/bin/activate ${ENV_NAME}

    if [ "${DOWNLOAD_PYTORCH}" == "1" ]
    then
        print_log "download pytorch"
        remove_dir ${work_dir}/pytorch
        clone_code https://github.com/pytorch/pytorch.git v1.5.0-rc3
        cd ${work_dir}/pytorch
        git submodule sync
        git submodule update --init --recursive
        pip install -r requirements.txt
        cd ..
    fi

    if [ "${DOWNLOAD_IPEX}" == "1" ]
    then
        print_log "download ipex"
        remove_dir ${work_dir}/intel-extension-for-pytorch
        clone_code https://github.com/intel/intel-extension-for-pytorch.git v1.0.2
        cd ${work_dir}/intel-extension-for-pytorch
        git submodule sync
        git submodule update --init --recursive
        pip install lark-parser hypothesis
        cp torch_patches/dpcpp-v1.5-rc3.patch ${work_dir}/pytorch/
        cd ${work_dir}/pytorch
        git apply dpcpp-v1.5-rc3.patch
        cd ..
    fi

    if [ "${DOWNLOAD_TORCHCCL}" == "1" ]
    then
        print_log "download torch-ccl"
        remove_dir ${work_dir}/torch-ccl
        clone_code https://github.com/intel/torch-ccl.git 2021.1-beta09
        cd ${work_dir}/torch-ccl
        git submodule update --init --recursive
        cd ..
    fi

    if [ "${DOWNLOAD_JEMALLOC}" == "1" ]
    then
        print_log "download jemalloc"
        remove_dir ${work_dir}/jemalloc
        clone_code https://github.com/jemalloc/jemalloc.git dev
    fi

    if [ "${DOWNLOAD_VISION}" == "1" ]
    then
        print_log "download vision"
        remove_dir ${work_dir}/vision
        clone_code https://github.com/pytorch/vision
        cd ${work_dir}/vision
        git checkout 883f1fb01a8ba0e1b1cdc16c16f2e6e0ef87e3bd
        cd ..
    fi

    if [ "${DOWNLOAD_IMAGENET}" == "1" ]
    then
        print_log "download imagenet"
        remove_dir ${work_dir}/imagenet
        clone_code https://gitlab.devtools.intel.com/ia-optimized-model-for-pytorch/imagenet.git enbale_ipex_torch_ccl_ddp

        sed -i 's/torch.save/#torch.save/' ${work_dir}/imagenet/imagenet/main.py
        sed -i 's/if is_best:/if False:/' ${work_dir}/imagenet/imagenet/main.py
    fi

    if [ "${DOWNLOAD_CCL}" == "1" ]
    then
        print_log "download ccl"
        remove_dir ${work_dir}/mlsl2
        clone_code git@github.intel.com:ict/mlsl2.git release/ccl_2021.1-beta07
    fi

    if [ "${BUILD_PYTORCH}" == "1" ]
    then
        print_log "build pytoch"
        cd ${work_dir}/pytorch
        remove_dir build
        mkdir build
        cd build && ln -s include/sleef.h . && cd ..
        CC=mpicc CXX=mpicxx python setup.py develop install --cmake
        python setup.py install
    fi

    if [ "${BUILD_IPEX}" == "1" ]
    then
        print_log "build ipex"
        cd ${work_dir}/intel-extension-for-pytorch
        remove_dir build
        python setup.py install
    fi

    if [ "${BUILD_TORCHCCL}" == "1" ]
    then
        print_log "build torch-ccl"
        cd ${work_dir}/torch-ccl
        remove_dir build
        python setup.py install
    fi

    if [ "${BUILD_JEMALLOC}" == "1" ]
    then
        print_log "build jemalloc"
        cd ${work_dir}/jemalloc
        ./autogen.sh
        ./configure --prefix=${jemalloc_install_path}
        make -j12
        make install
    fi

    if [ "${BUILD_VISION}" == "1" ]
    then
        print_log "build vision"
        cd ${work_dir}/vision
        remove_dir build
        python setup.py install
    fi

    torch_ccl_path=$(python -c "import torch; import torch_ccl; import os; print(os.path.abspath(os.path.dirname(torch_ccl.__file__)));")
    torch_ccl_path="${torch_ccl_path}/ccl"

    if [ "${BUILD_CCL}" == "1" ]
    then
        print_log "build ccl"
        cd ${work_dir}/mlsl2
        remove_dir build
        mkdir build
        cd build
        cmake .. && make -j12 install
        torch_ccl_path=${work_dir}/mlsl2/build/_install
    fi

    print_log "torch_ccl_path = ${torch_ccl_path}"
    source $torch_ccl_path/env/setvars.sh

    cleanup_hosts $hostfile

    export LD_PRELOAD=${jemalloc_install_path}/lib/libjemalloc.so
}

test()
{
    set_env
    download_and_build
    print_env

    mkdir -p ${checkpoint_dir}

    print_log "training_started"
    python ${work_dir}/imagenet/imagenet/lauch.py --nnodes ${N} --nproc_per_node ${PPN} --ccl_worker_count=${WORKERS} \
        --master_addr=${master_ip} --hostfile $hostfile \
        python -u ${work_dir}/imagenet/imagenet/main.py --lr 0.1 -a resnet50 --ipex --dnnl -b ${BATCH_SIZE} \
        -j 2 --epochs ${EPOCHS} --use_torch_ccl --dist-backend=ccl --seed 1 --checkpoint-dir ${checkpoint_dir} \
        ${imagenet_dir}
    print_log "training_completed"

    #source ${anaconda_install_path}/bin/deactivate ${ENV_NAME}
    #print_log "deactivated ${ENV_NAME}"
}

#==============================================================================
#                                     MAIN
#==============================================================================

set_default_values
parse_parameters
adjust_parameters
print_parameters
test 2>&1 | tee ${log_path}
check ${log_path}
