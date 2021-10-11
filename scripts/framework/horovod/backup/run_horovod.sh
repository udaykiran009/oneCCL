#!/bin/bash +x

export TESTSPACE_DIR=${SRC_DIR}/testspace
export FFN_OUT_DIR=${TESTSPACE_DIR}/ffn_out_dir
export SCRIPT_DIR=${SRC_DIR}/scripts/framework/horovod/

# get nodes 
hostnames=$(srun hostname)
if [ $? -ne 0 ]; then
    echo "ERROR: Can't define hosts"
    exit 1
fi

num_of_hosts=0
for host in ${hostnames}; do 
    hosts="${hosts},${host}"
    num_of_hosts=$((num_of_hosts+1))
done

hosts=$(echo $hosts | cut -c 2-)
echo "HOSTS = ${hosts}"

export CONDA_DIR=${CONDA_DIR}

mkdir -p ${TESTSPACE_DIR}

echo TESTSPACE_DIR = ${TESTSPACE_DIR}
echo FFN_OUT_DIR = ${FFN_OUT_DIR}
echo CONDA_DIR = ${CONDA_DIR}

source /nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/linux/bin/compilervars.sh intel64
source ${SRC_DIR}/build/_install/env/vars.sh
source /p/pdsd/scratch/jenkins/artefacts/impi-ch4-nightly/last/psxe_impi/intel64/bin/mpivars.sh

env

ENABLE_TESTING=yes ${SCRIPT_DIR}/horovod.sh $((${num_of_hosts}*2)) 2 1 1 ${hosts}
