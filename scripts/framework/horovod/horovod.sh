#!/bin/bash +x
#
# The script expects data and sources to at specific locations realtive 
# to user's HOME directory. The variables/functions to be mindful are:
# are as follows:
#   1. FFN_OUT_DIR
#   2. FFN_DATA_DIR
#   3. FFN_SRC_DIR 
#   4. conda_env
# 
# To use /dev/shm for data,  first the data have to be copied to each node 
# through a mpirun.
#
# The usage of the script is as follows.
#                       np ppn bs threaddivisor hostnames
# bash -x ./ffn_oneccl_perf0.sh 2 2 1 1 HOSTLIST

# top level configs 
#MYNAME=endv_occl_perf1nx
#HOME=~
#USER=`id -nu`
#FFN_OUT_DIR=$HOME/lfs/a21/outputs_ffn # defined here for log file redirection
TSTAMP=$(date +%s)

MYNAME=$(id -u -n)

BASENAME=$(basename $0 .sh)
SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
SRC_DIR="${SCRIPT_DIR}/../../../"
HOROVOD_OUT_DIR=${SRC_DIR}/_horovod
HOROVOD_TESTING_DIR="/p/pdsd/scratch/Uploads/Horovod_testing"

export ENV_NAME="horovod_manual"

if [[ -z ${FFN_OUT_DIR} ]]; then
    mkdir -p ${HOROVOD_OUT_DIR}
    FFN_OUT_DIR=${HOROVOD_OUT_DIR}/TEST_OUT
fi
mkdir -p ${FFN_OUT_DIR}
JOBLOGFILE=${FFN_OUT_DIR}/${TSTAMP}_ffn_${MYNAME}.log

TSTAMP=$(date +%s)

DATE=$(date +%Y%m%d)
TIME=$(date +%H%M%S)

echo "HOME: $HOME"
echo "USER: $USER"
echo "tail -f $JOBLOGFILE"

export MKLDNN_VERBOSE=0

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

#
# bash functions 
#

# source TF, Horovod, and oneCCL environment
conda_envs() {
    if [[ -z ${CONDA_DIR} ]]; then
        CONDA_DIR="/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Drops/tools/oneCI/miniconda3"
    fi
    source ~/.bashrc
    conda activate ${ENV_NAME}
    PYTHON="${CONDA_DIR}/envs/${ENV_NAME}/bin/python"
    echo "USAGE PYTHON: $(ls ${PYTHON})"
    echo "HOROVOD: $(conda list -n ${ENV_NAME}| grep horovod)"
    echo "HOROVOD: $(conda list | grep horovod)"
    ${PYTHON} ${HOROVOD_TESTING_DIR}/FFN_examples/tests/tf_hvd_hello.py
}

# oneCCL related variables

oneccl_config() {
    # oneCCL IMPI related
    export I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0 
    export I_MPI_PIN_DOMAIN=socket
    export I_MPI_DEBUG=6
    export I_MPI_FALLBACK=enable
    export I_MPI_FABRICS=shm:ofi
    export PSM2_IDENTIFY=1
    
    # Horovod related
    export HOROVOD_FUSION_THRESHOLD=0 #67108864 #64MB default
    export HOROVOD_CPU_OPERATIONS=CCL
    
    # oneCCL related
    export CCL_LOG_LEVEL=info
    export CCL_ATL_TRANSPORT=mpi

    # core assignment
    export CCL_WORKER_COUNT=2

    # Get socket's info
    SOCKETS_INFO=$(cpuinfo | grep sockets)
    NUM_OF_SOCKETS=${SOCKETS_INFO##*:}
    
    # Get core's info
    CORES_INFO=$(cpuinfo | grep "Cores\s*:")
    TOTAL_NUM_OF_CORES=${CORES_INFO##*:}

    CORES_PER_SOCKET_INFO=$(cpuinfo | grep "Cores per package")
    CORES_PER_SOCKET=${CORES_PER_SOCKET_INFO##*:}

    # Get HT info

    grep -o '^flags\b.*: .*\bht\b' /proc/cpuinfo | tail -1
    if [[ $? = 0 ]]
    then
        HT="yes"
    else
        HT="no"
    fi

    # Calculate CCL_WORKER_AFFINITY, HOROVOD_THREAD_AFFINITY and I_MPI_PIN_PROCESSOR_EXCLUDE_LIST

    for i in `seq ${NUM_OF_SOCKETS}`; do 
        for j in `seq ${CCL_WORKER_COUNT} -1 1`; do 
            (( CURR_POSITION = ${CORES_PER_SOCKET} * ${i} - 1 - ${j} ))
            CCL_WORKER_AFFINITY=${CCL_WORKER_AFFINITY},${CURR_POSITION} 
        done
    done
    CCL_WORKER_AFFINITY=${CCL_WORKER_AFFINITY:1}

    if [[ ${HT} = "yes" ]]
    then
        (( NUM_OF_HOROVOD_THREAD = ${NUM_OF_SOCKETS} * 2 ))
    else
        NUM_OF_HOROVOD_THREAD=${NUM_OF_SOCKETS}
    fi

    for i in `seq ${NUM_OF_HOROVOD_THREAD}`; do
        (( CURR_POSITION = ${CORES_PER_SOCKET} * ${i} - 1 ))
        HOROVOD_THREAD_AFFINITY=${HOROVOD_THREAD_AFFINITY},${CURR_POSITION}
    done
    HOROVOD_THREAD_AFFINITY=${HOROVOD_THREAD_AFFINITY:1}

    (( EXCLUDE_PER_SOCKET = ${CCL_WORKER_COUNT} + 1 ))
    for i in `seq ${NUM_OF_HOROVOD_THREAD}`; do
        for j in `seq ${EXCLUDE_PER_SOCKET} -1 1`; do 
            (( CURR_POSITION = ${CORES_PER_SOCKET} * ${i} - ${j} ))
            I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=${I_MPI_PIN_PROCESSOR_EXCLUDE_LIST},${CURR_POSITION}
        done
    done
    I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=${I_MPI_PIN_PROCESSOR_EXCLUDE_LIST:1}

    export HOROVOD_THREAD_AFFINITY=${HOROVOD_THREAD_AFFINITY}
    export CCL_WORKER_AFFINITY=${CCL_WORKER_AFFINITY}
    export I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=${I_MPI_PIN_PROCESSOR_EXCLUDE_LIST}

    echo "HOROVOD_THREAD_AFFINITY          = ${HOROVOD_THREAD_AFFINITY}"
    echo "CCL_WORKER_AFFINITY              = ${CCL_WORKER_AFFINITY}"
    echo "I_MPI_PIN_PROCESSOR_EXCLUDE_LIST = ${I_MPI_PIN_PROCESSOR_EXCLUDE_LIST}"
}

# one mpi run 

single_run() {
    for LRATE in 1.6e-6
    do
        TRAINDIR=${FFN_OUT_DIR}/${TSTAMP}_ffn_${MYNAME}_b${BATCHSIZE}_np${NRANKS}_ppn${NRANK_PER_NODE}_t${NTHREAD}_r${LRATE}_o${OPTIMIZER}_s${SHARDING_RULE}${SCALING_RULE}
        mkdir -p $TRAINDIR
    
        TIMELINEFILE=$TRAINDIR/horovod_timeline.json
        HOROVOD_TIMELINE=""
        if [ "$HVD_TIMELINE" = "tl" ]; then
            touch $TIMELINEFILE
            export HOROVOD_TIMELINE=$TIMELINEFILE
        else
            echo "HOROVOD_TIMELINE else"
        fi
        echo "HOROVOD_TIMELINE $HOROVOD_TIMELINE"

        mpirun -n $NRANKS -ppn  $NRANK_PER_NODE \
            -hosts $HOSTS -bootstrap ssh  \
            -bind-to socket \
            -genv KMP_BLOCKTIME=0 \
            -genv KMP_AFFINITY="granularity=fine,compact,1,0,verbose" \
            -genv OMP_NUM_THREADS=$NTHREAD \
            -genv HDF5_USE_FILE_LOCKING=False \
            -genv H5PY_DEFAULT_READONLY=1 \
            -genv HOROVOD_TIMELINE=$HOROVOD_TIMELINE \
            ${CONDA_DIR}/envs/${ENV_NAME}/bin/python $TRAINER \
                --train_coords $TFRECORDFILE \
                --data_volumes valdation1:${GRAYSCALE}:raw \
                --label_volumes valdation1:${GROUNDTRUTH}:stack \
                --model_name convstack_3d.ConvStack3DFFNModel \
                --model_args "{\"depth\": 12, \"fov_size\": [33, 33, 33], \"deltas\": [8, 8, 8]}" \
                --image_mean 128 \
                --image_stddev 33 \
                --batch_size $BATCHSIZE \
                --learning_rate $LRATE \
                --optimizer $OPTIMIZER \
                --scaling_rule $SCALING_RULE \
                --sharding_rule $SHARDING_RULE \
                --num_intra_threads $NTHREAD \
                --num_inter_threads 2 \
                --max_steps $MAX_ITERS \
                --summary_rate_secs 4000 \
                --train_dir $TRAINDIR &> ${TRAINDIR}.log &
        #    --learning_rate 0.0001 \
        #flags.DEFINE_float('learning_rate', 0.001, 'Initial learning rate.')
        #flags.DEFINE_integer('summary_rate_secs', 120,
        #                     'How often to save summaries (in seconds).')
        pgrep -a train >> simple_logging.log
    done
    sleep 1
    echo Run starting np$NRANKS ppn$NRANK_PER_NODE bs$BATCHSIZE for $PER_RUN_TIME secs  " !"
}


# FFN realated configs
ffn_config() {
    HDF5_USE_FILE_LOCKING=False
    H5PY_DEFAULT_READONLY=1
    if [[ -z ${FFN_DATA_DIR} ]]; then
        FFN_DATA_DIR=${HOROVOD_TESTING_DIR}/data/
    fi
    TFRECORDFILE=$FFN_DATA_DIR/tf_record_file
    GROUNDTRUTH=$FFN_DATA_DIR/groundtruth.h5 
    GRAYSCALE=$FFN_DATA_DIR/grayscale_maps.h5
    if [[ -z ${FFN_SRC_DIR} ]]; then
        FFN_SRC_DIR=${HOROVOD_TESTING_DIR}/FFN
    fi
    TRAINER=$FFN_SRC_DIR/train_hvd.py
    HVD_TIMELINE=${6:-notl}
    HOSTS=${5:-localhost}
    THREADFACTOR=${4:-1}
    BATCHSIZE=$3
    NRANK_PER_NODE=$2
    echo "HOSTS: ${HOSTS}"
    echo "THREADFACTOR: ${THREADFACTOR}"
    echo "BATCHSIZE: ${BATCHSIZE}"
    echo "NRANK_PER_NODE: ${NRANK_PER_NODE}"
    NTHREAD_PER_CORE=`lscpu | grep Thread | grep -Eo "[0-9]+"`
    SOCKETS=`lscpu | grep Thread | grep -Eo "[0-9]+"`
    CORES_PER_SOCKETS=`lscpu | grep "Core(s) per socket" | grep -Eo "[0-9]+"`
    CPUS=`lscpu | grep "^CPU(s):" | grep -Eo "[0-9]+"`
    NTHREAD_UNUSED=0
    for i in $(echo $I_MPI_PIN_PROCESSOR_EXCLUDE_LIST | sed "s/,/ /g")
    do
        echo $i
        NTHREAD_UNUSED=`expr $NTHREAD_UNUSED + 1`
    done
    NCORES_UNUSED=`expr $NTHREAD_UNUSED / $NTHREAD_PER_CORE`

    NCORES=$(($(nproc)/$NTHREAD_PER_CORE))
    NTHREAD=$(((($NCORES-$NCORES_UNUSED)/$NRANK_PER_NODE)*$THREADFACTOR))                          # Number of threads per MPI rank

    NRANKS=$1                                       # Number of ranks of procs
    OPTIMIZER=adam
    LRATE=0.001
    SHARDING_RULE=1 # 0 for no sharding, 1 for sharding
    SCALING_RULE=1 # 0 for no scaling, 1 for linear scaling, 2 for sqrt scaling wrt number of ranks
    MAX_ITERS=100
}

check_environment()
{
    which mpiexec
    MPI_INSTALL_CHECK=$?
    if [ "$MPI_INSTALL_CHECK" != "0" ]
    then
        echo "Error: IMPI wasn't found"
        exit 1
    fi

    MPI_INSTALL_CHECK_2=`echo $I_MPI_ROOT`
    if [ "$MPI_INSTALL_CHECK_2" == "" ]
    then
        echo "Error: I_MPI_ROOT wasn't found"
        exit 1
    fi

    CCL_INSTALL_CHECK=`echo $CCL_ROOT`
    if [ "$CCL_INSTALL_CHECK" == "" ]
    then
        echo "Error: CCL_ROOT wasn't found"
        exit 1
    fi
}

build_horovod() {
    if [[ ${BUILD_HOROVOD} == "yes" ]]; then
        echo "DEBUG: BUILD HOROVOD..."
        echo "DEBUG: pip = $(which pip)"
        echo "DEBUG: python = $(which python)"
        check_environment
        conda list -n ${ENV_NAME} | grep horovod
        pip uninstall -y horovod

        cd ${SRC_DIR}
        git clone --recursive https://github.com/horovod/horovod.git
        cd horovod
        export HOROVOD_WITH_MPI=1
        export HOROVOD_CPU_OPERATIONS=CCL
        export HOROVOD_WITHOUT_MXNET=1
        export HOROVOD_WITHOUT_PYTORCH=1
        export HOROVOD_WITH_TENSORFLOW=1
        export HOROVOD_WITHOUT_GLOO=1

        python setup.py install > ${HOROVOD_OUT_DIR}/build_horovod_${DATE}_${TIME}.log 2>&1
        CheckCommandExitCode $? "Build Horovod failed"
        echo "DEBUG: BUILD HOROVOD...DONE"
    fi
}

ccl_build_cpu() {
    if [[ ${BUILD_CPU}  == "yes" ]]; then
        cd ${SRC_DIR}
        BUILD_COMPILER_TYPE=intel ${SRC_DIR}/scripts/build.sh --build-cpu > ${HOROVOD_OUT_DIR}/build_cpu_${DATE}_${TIME}.log 2>&1
        CheckCommandExitCode $? "Build cpu failed"
        cp ${SRC_DIR}/ccl_public/vars.sh.in ${SRC_DIR}/build/_install/env
        mv ${SRC_DIR}/build/_install/env/vars.sh.in ${SRC_DIR}/build/_install/env/vars.sh
    fi
}

ccl_build_gpu() {
    if [[ ${BUILD_GPU}  == "yes" ]]; then
        cd ${SRC_DIR}
        ${SRC_DIR}/scripts/build.sh --build-gpu > ${HOROVOD_OUT_DIR}/build_gpu_${DATE}_${TIME}.log 2>&1
        CheckCommandExitCode $? "Build gpu failed"
        cp ${SRC_DIR}/ccl_public/vars.sh.in ${SRC_DIR}/build_gpu/_install/env
        mv ${SRC_DIR}/build_gpu/_install/env/vars.sh.in ${SRC_DIR}/build_gpu/_install/env/vars.sh
    fi
}

submit_job() {
    if [[ ${SUBMIT_JOB} == "yes" ]]; then
        if [[ -z ${HOSTS} ]]; then
            if [[ ${PARTITION} ]]; then
                PARTITION="skx-opa-18"
            fi
            if [[ -z ${N} ]]; then
                N=2
            fi
            echo "INFO: The 'HOSTS' variable is not set. The task will be run in SLURM on ${PARTITION} nodes" 
        else
            #TODO
            echo "Running on specific nodes is not supported yet"
            exit 1
        fi

        ###
        ### Checking that the task is still alive
        ###

        SLURM_JOB_ID=$(ssh nnlmpisnb03 "sbatch --export=ALL,SRC_DIR=${SRC_DIR},SCRIPT_DIR=${SCRIPT_DIR} --export=SRC_DIR=${SRC_DIR} --output=${SRC_DIR}/slurm_logfile.log --error=${SRC_DIR}/slurm_err_logfile.log -N ${N} -p ${PARTITION} ${SCRIPT_DIR}/run_horovod.sh")
        SLURM_JOB_ID=$(grep -o "[[:digit:]]*" <<< ${SLURM_JOB_ID})

        while true
        do
            sleep 30
            ssh nnlmpisnb03 "squeue --job $SLURM_JOB_ID" | grep -c "$SLURM_JOB_ID"
            STATUS="$?"
            if [ "$STATUS" != "0" ]
            then
                break
            fi
        done

        TESTSPACE_DIR=${ARTEFACT_DIR}/testspace
        FFN_OUT_DIR=${TESTSPACE_DIR}/ffn_out_dir

        ###
        ###	Check exit status
        ###

        if [[ $(ls ${FFN_OUT_DIR} | grep ".*ppn.*log" | wc -l) -eq 0 ]]
        then 
            echo "ERROR: Log files was not found!"
            exit 1
        fi

        for TEST_LOG in $(ls ${FFN_OUT_DIR} | grep ".*ppn.*log")
        do 
            test_failed=$(grep -E -c -i 'error|invalid|Aborted|fail|failed' ${FFN_OUT_DIR}/${TEST_LOG})
            
            echo "#"
            echo "# ${TEST_LOG}"
            echo "#"
            
            if [[ ${test_failed} -ne 0 ]]
            then
                echo "ERROR: testing failed"
                exit 1
            fi
        done


        echo "LOGS:"
        for TEST_LOG in $(ls ${FFN_OUT_DIR} | grep ".*ppn.*log")
        do 
            echo "	->  ${FFN_OUT_DIR}/${TEST_LOG}"
    done
    fi
}
# run

conda_envs

HOME=${FFN_OUT_DIR}/../TEST

ccl_build_cpu
ccl_build_gpu
build_horovod
submit_job

if [[ "${ENABLE_TESTING}" == "yes" ]]
then
    oneccl_config
    ffn_config $@
    check_environment
    for batchsize in 4 16
    do
        BATCHSIZE=${batchsize}
        single_run
        wait
        BATCHSIZE=$((4*$BATCHSIZE))
    done
fi
echo "Finished running!"
