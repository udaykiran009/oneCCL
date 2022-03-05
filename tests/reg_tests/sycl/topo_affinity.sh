#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"

source ${ROOT_DIR}/utils.sh

make_common_actions ${SCRIPT_DIR} ${TEST_LOG} "sycl"

transports="ofi mpi"
topo_colors="ze fixed env"
pvc_random_device_list="3.0 1.0 3.1 4.1 0.0 4.0 2.1 2.0 1.1 5.1 0.1 5.0"
bench_ops="-w 0 -i 1 -c all -l allgatherv,allreduce,reduce -b sycl -y 1024"

check_string() {
    input_str=$1
    if [ -z "$input_str" ]
    then
        echo "Fail: string is empty" >> ${TEST_LOG} 2>&1
    fi
}

check_color() {
    current_str=$1
    pattern=$2
    if [ "$current_str" != "$pattern" ]
    then
        echo "Fail: $current_str, $pattern" >> ${TEST_LOG} 2>&1
    fi
}

parse_colors() {
    expected_intra_str=$1
    expected_inter_str=$2

    intra_str=""
    inter_str=""

    while IFS='' read -r line
    do
        if [[ $line =~ intra_card_colors:\ ([0-9\ ]+) ]]
        then
            intra_str="${BASH_REMATCH[1]}"
            break
        fi
    done < "${TEST_LOG}"

    while IFS='' read -r line
    do
        if [[ $line =~ inter_card_colors:\ ([0-9\ ]+) ]]
        then
            inter_str="${BASH_REMATCH[1]}"
            break
        fi
    done < "${TEST_LOG}"

    check_string "$intra_str"
    check_string "$inter_str"

    check_color "$intra_str" "$expected_intra_str"
    check_color "$inter_str" "$expected_inter_str"
}

run_case() {
    n=$1
    ppn=$2
    intra_colors=$3
    inter_colors=$4
    topo_map=$5
    env_var=${6:-}
    extra_bench_ops=${7:-}

    for transport in ${transports}
    do
        for topo_color in ${topo_colors}
        do
            cmd="CCL_ATL_TRANSPORT=${transport}"
            cmd+=" CCL_ALLGATHERV=topo"
            cmd+=" CCL_ALLREDUCE=topo"
            cmd+=" CCL_REDUCE=topo"
            cmd+=" CCL_LOG_LEVEL=info"

            if [ "${topo_color}" == "env" ]  && [ ! -z "$topo_map" ]
            then
                cmd+=" CCL_TOPO_COLOR=${topo_map}"
            else
                cmd+=" CCL_TOPO_COLOR=${topo_color}"
            fi

            cmd+=" ${env_var}"
            cmd+=" mpiexec -l -n $n -ppn $ppn ${SCRIPT_DIR}/benchmark"
            cmd+=" ${bench_ops} ${extra_bench_ops} > ${TEST_LOG} 2>&1"

            run_cmd "${cmd}"
            parse_colors "${intra_colors} " "${inter_colors} "
            check_log ${TEST_LOG}
            rm ${TEST_LOG}
        done
    done
}

if [[ ${PLATFORM_HW_DISCRETE_GPU} = "ats" || ${PLATFORM_HW_DISCRETE_GPU} = "gen9" ]]
then
    # [ATS/GEN9]: case 1: 1 node, 2 ranks
    run_case 2 2 "0 0" "0 1" "\"card:{0,1};plane:{0},{1}\""

    # [ATS/GEN9]: case 2: 1 node, 4 ranks
    run_case 4 4 "0 0 1 1" "0 1 0 1" "\"card:{0,1},{2,3};plane:{0,2},{1,3}\""

    # [ATS/GEN9]: case 3: 1 node, 8 ranks
    run_case 8 8 "0 0 1 1 2 2 3 3" "0 1 0 1 0 1 0 1" \
                 "\"card:{0,1},{2,3},{4,5},{6,7};plane:{0,2,4,6},{1,3,5,7}"\"

    if [[ ${PLATFORM_HW_DISCRETE_GPU} = "gen9" ]]
    then
        # [GEN9]: case 4: 2 nodes, 2 ranks
        run_case 2 1 "0 1000" "0 1000" "\"card:{0};plane:{0}\""

        # [GEN9]: case 5: 2 nodes, 4 ranks
        run_case 4 2 "0 0 1000 1000" "0 1 1000 1001" "\"card:{0,1};plane:{0},{1}\""

        # [GEN9]: case 6: 2 nodes, 8 ranks
        run_case 8 4 "0 0 1 1 1000 1000 1001 1001" "0 1 0 1 1000 1001 1000 1001" \
                     "\"card:{0,1},{2,3};plane:{0,2},{1,3}"\"
    fi
elif [[ ${PLATFORM_HW_DISCRETE_GPU} = "pvc" ]]
then
    # [PVC] case 1: 1 node, 6 ranks, 0-th tiles only
    affinity_env="ZE_AFFINITY_MASK=0.0,1.0,2.0,3.0,4.0,5.0"
    run_case 6 6 "0 1 2 3 4 5" "0 0 0 0 0 0" \
                 "\"card:{0},{1},{2},{3},{4},{5};plane:{0,1,2,3,4,5}\"" "${affinity_env}"

    # [PVC] case 2: 1 node, 6 ranks, root devices only
    run_case 6 6 "0 1 2 3 4 5" "0 0 0 0 0 0" \
                 "\"card:{0},{1},{2},{3},{4},{5};plane:{0,1,2,3,4,5}\"" "" "-g 1"

    # [PVC] case 3: 1 node, 6 ranks
    affinity_env="ZE_AFFINITY_MASK=0,1,2,3,4,5"
    run_case 6 6 "0 0 1 1 2 2" "0 1 0 1 0 1" \
                 "\"card:{0,1},{2,3},{4,5};plane:{0,2,4},{1,3,5}\"" "${affinity_env}"

    # [PVC] case 4: 1 node, 12 ranks
    run_case 12 12 "0 0 1 1 2 2 3 3 4 4 5 5" "0 1 0 1 0 1 0 1 0 1 0 1" \
                   "\"card:{0,1},{2,3},{4,5},{6,7},{8,9},{10,11};plane:{0,2,4,6,8,10},{1,3,5,7,9,11}\""

    # [PVC] case 5: 1 node, 12 ranks, affinity mask is pseudo-random
    affinity_env=$(create_ze_affinity_env "${pvc_random_device_list}")
    run_case 12 12 "0 0 1 1 2 2 3 3 4 4 5 5" "0 1 0 1 0 1 0 1 0 1 0 1" \
                   "\"card:{0,1},{2,3},{4,5},{6,7},{8,9},{10,11};plane:{0,2,4,6,8,10},{1,3,5,7,9,11}\"" \
                   "${affinity_env}"

    # [PVC] case 6: 2 nodes, 24 ranks, affinity mask is pseudo-random
    affinity_env=$(create_ze_affinity_env "${pvc_random_device_list} ${pvc_random_device_list}")
    run_case 24 12 "0 0 1 1 2 2 3 3 4 4 5 5 1000 1000 1001 1001 1002 1002 1003 1003 1004 1004 1005 1005" \
                   "0 1 0 1 0 1 0 1 0 1 0 1 1000 1001 1000 1001 1000 1001 1000 1001 1000 1001 1000 1001" \
                   "\"card:{0,1},{2,3},{4,5},{6,7},{8,9},{10,11};plane:{0,2,4,6,8,10},{1,3,5,7,9,11}\"" \
                   "${affinity_env}"

    # [PVC] case 7: ranks are placed in round-robin on 2 nodes
    run_case 24 1 "0 1000 1 1001 2 1002 3 1003 4 1004 5 1005 0 1000 1 1001 2 1002 3 1003 4 1004 5 1005" \
                  "0 1000 1 1001 0 1000 1 1001 0 1000 1 1001 0 1000 1 1001 0 1000 1 1001 0 1000 1 1001" \
                  "\"card:{0,6},{1,7},{2,8},{3,9},{4,10},{5,11};plane:{0,1,2,3,4,5},{6,7,8,9,10,11}\""
fi

echo "Pass"
