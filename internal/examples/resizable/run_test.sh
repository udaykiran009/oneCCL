#!/bin/bash
ccl_extra_env="CCL_ATL_TRANSPORT=ofi CCL_KVS_IP_EXCHANGE=env CCL_KVS_IP_PORT=`hostname -i`_1244 CCL_PM_TYPE=resizable CCL_DEFAULT_RESIZABLE=1"

function run_test()
{
    test_case=$1
    world_size=$2
    logname="$test_case""_$3"
    echo $logname
    eval `echo $ccl_extra_env "CCL_WORLD_SIZE=$world_size" ./resizable.out $test_case;`  &> $logname.out
    if [[ "1" -eq "$?" ]] && [[ "$test_case" == "reconnect" ]];
    then
        eval `echo $ccl_extra_env "CCL_WORLD_SIZE=$world_size" ./resizable.out $test_case;`
    fi
}

run_test $1 $2 $3
