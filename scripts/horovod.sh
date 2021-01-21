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
HOROVOD_OUT_DIR=${SCRIPT_DIR}/_horovod
HOROVOD_TESTING_DIR="/p/pdsd/scratch/Uploads/Horovod_testing"


#. ~/bashrc
export ENV_NAME="horovod_manual"

if [[ -z ${FFN_OUT_DIR} ]]; then
    mkdir -p ${HOROVOD_OUT_DIR}
    FFN_OUT_DIR=${HOROVOD_OUT_DIR}/TEST_OUT
fi
mkdir -p ${FFN_OUT_DIR}
JOBLOGFILE=${FFN_OUT_DIR}/${TSTAMP}_ffn_${MYNAME}.log

TSTAMP=$(date +%s)

if [[ -z ${CCL_ROOT} ]]; then
    echo "ERROR: CCL_ROOT was not set!"
    exit 1
fi
if [[ -z ${I_MPI_ROOT} ]]; then
    echo "ERROR: I_MPI_ROOT was not set!"
    exit 1
fi

echo "HOME: $HOME"
echo "USER: $USER"
echo "tail -f $JOBLOGFILE"
exec 1>$JOBLOGFILE
exec 2>&1

export MKLDNN_VERBOSE=0

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
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
    #export I_MPI_FABRICS_LIST=dapl,ofa,tmi,ofi,tcp
    export I_MPI_FALLBACK=enable
    export I_MPI_FABRICS=shm:ofi
    #export I_MPI_TMI_PROVIDER=psm2
    export PSM2_IDENTIFY=1
    #export FI_PROVIDER=psm2
    
    # Horovod related
    #export HOROVOD_LOG_LEVEL=DEBUG
    export HOROVOD_FUSION_THRESHOLD=0 #67108864 #64MB default
    #HOROVOD_CYCLE_TIME=5
    #HOROVOD_TIMELINE_MARK_CYCLES=1
    export HOROVOD_CPU_OPERATIONS=CCL
    
    # oneCCL related
    export CCL_LOG_LEVEL=2 #{ ERROR = 0, INFO, DEBUG, TRACE };
    export CCL_ATL_SHM=1
    export CCL_ATL_TRANSPORT=mpi
    export ATL_SYNC_COLL=1
    export CCL_YIELD=sched_yield
    export ATL_EXTRA_EP=1

    # core assignment
    export CCL_WORKER_OFFLOAD=1
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
        #    -genv HOROVOD_TIMELINE=$HOROVOD_TIMELINE \

        #    -bind-to socket \
        #export I_MPI_PIN_DOMAIN=socket 
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
    #FFN_DATA_DIR=$HOME/panfs/a21/FFN/data
    #FFN_DATA_DIR=/panfs/projects/ML_datasets/FFN/data
    #FFN_DATA_DIR=/tmp/$USER/a21/FFN/data
    #FFN_DATA_DIR=$HOME/lfs/a21/FFN/data
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
    MAX_ITERS=300

}

# run

conda_envs

HOME=${FFN_OUT_DIR}/../TEST

oneccl_config
ffn_config $@

for batchsize in 4 16
do
   BATCHSIZE=${batchsize}
   single_run
   wait
   BATCHSIZE=$((4*$BATCHSIZE))
done

echo "Finished running!"