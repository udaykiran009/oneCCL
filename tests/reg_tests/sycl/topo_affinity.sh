#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl
get_bench ${SCRIPT_DIR} ${TEST_LOG} "sycl"

cd ${SCRIPT_DIR}

#common vars
export CCL_ALLGATHERV=topo
export CCL_ALLREDUCE=topo
export CCL_REDUCE=topo

# TODO: uncoment after fix MLSL-1155
#transports="ofi mpi"
transports="mpi"
random_device_list="3.0 1.0 3.1 4.1 0.0 4.0 2.1 2.0 1.1 5.1 0.1 5.0"
bench_options="-w 1 -i 8 -c all -l allgatherv,allreduce,reduce -b sycl -t 131072"

create_affinity_env() {
    device_list=$1

    result_env=""
    counter=0
    for device in ${device_list}; do
        result_env+="env ZE_AFFINITY_MASK=${device}:${counter};"
        counter=$((counter + 1))
    done
    echo "I_MPI_GTOOL=\"$result_env"\"
}

run_case() {
    n=$1
    ppn=$2
    affinity_env=$3

    for transport in ${transports}
    do
        cmd="CCL_ATL_TRANSPORT=${transport}"
        cmd+=" ${affinity_env}"
        cmd+=" mpiexec -l -n $n -ppn $ppn ${SCRIPT_DIR}/benchmark"
        cmd+=" ${bench_options} > ${TEST_LOG} 2>&1"
        run_cmd "${cmd}"
        check_log ${TEST_LOG}
        rm ${TEST_LOG}
    done
}

#case 1: 1 node, 6 ranks, 0-th tiles only
affinity_env="ZE_AFFINITY_MASK=0.0,1.0,2.0,3.0,4.0,5.0"
run_case 6 6 "${affinity_env}"

#case 2: 1 node, 12 ranks, affinity mask is randomly generated
affinity_env=$(create_affinity_env "${random_device_list}")
run_case 12 12 "${affinity_env}"

# MLSL-1274
#case 3: 2 nodes, 24 ranks, affinity mask is randomly generated
#affinity_env=$(create_affinity_env "${random_device_list} ${random_device_list}")
#run_case 24 12 "${affinity_env}"

# MLSL-1274
#case 4: ranks are placed in round-robin on 2 nodes
#run_case 24 1 ""

# MLSL-1274
#case 5: 3 ranks: 2 ranks on first card, 1 rank on second card
#affinity_env=$(create_affinity_env "1.0 1.1 2.0")
#run_case 3 3 "${affinity_env}"

echo "Pass"
