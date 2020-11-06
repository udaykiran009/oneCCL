#!/bin/bash
test_timeout=120

function run_binary()
{
    mapfile -t host_list < $host_file
    h_id=0
    for ((i = 0; i < $world_size ; i++ )); do
        cmd="source $ccl_vars && $dir/run_binary.sh $world_size $i"
        timeout -k $((test_timeout))s $((test_timeout))s ssh ${host_list[$h_id]} $cmd&
        h_id=$(($h_id + 1))
        if [ "${#host_list[@]}" -eq "$h_id" ];
        then
            h_id=0
        fi           
    done
    wait
}

run()
{
    ccl_vars=$1
    host_file=$2
    world_size=$3
    dir=`pwd`
    PUR='\033[0;35m'
    RED='\033[0;31m'
    NC='\033[0m'

    echo -e "${PUR}Start test${NC}"
    exec_time=`date +%s`

    run_binary

    exec_time="$((`date +%s`-$exec_time))"
    if [ "$exec_time" -ge "$test_timeout" ];
    then
         echo -e "${RED}FAIL: Timeout ($exec_time > $test_timeout)${NC}"
    fi
}

run $1 $2 $3