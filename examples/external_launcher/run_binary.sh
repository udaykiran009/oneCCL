#!/bin/bash

function run()
{
    world_size=$1
    rank=$2
    dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
    host=`hostname`
    logname="$host""_$rank"
    echo $logname
    eval `echo $dir/external_launcher $world_size $rank $dir/storage.bin;`  &> $dir/$logname.out
}

run $1 $2
