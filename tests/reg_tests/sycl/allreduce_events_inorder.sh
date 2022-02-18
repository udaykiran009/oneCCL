#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "${BASH_SOURCE}") && pwd -P)
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BASENAME="$(basename $0 .sh)"
TEST_LOG="${BASENAME}.log"
BINFILE="${BASENAME}"

source ${ROOT_DIR}/utils.sh

check_impi
check_ccl

mode=""

case $1 in
    "test"|"") mode="test";;
    "perf") mode="perf";;
    *) echo "unknown mode: $1";;
esac

function common_env() {
    export CCL_SYCL_OUTPUT_EVENT=1
    export CCL_ZE_CLOSE_IPC_WA=1
    export EnableDirectSubmission=1
}

function run_cmd() {
    echo "Running $1"
    echo "Running $1" >> ${TEST_LOG}
    eval $1 >> ${TEST_LOG} 2>&1
    echo "\n\n" >> ${TEST_LOG}
}

function single_test_run() {
    algo=$1
    allreduce_mode=$2
    skip_iter_count=$3
    cache_mode=$4
    iter_count=$5
    proc_count=$6

    cmd_args=""
    env_cmd=""

    env_cmd+=" CCL_ALLREDUCE=$algo"

    if [[ $allreduce_mode == "single" ]]
    then
        cmd_args+=" -b 0"
    else
        cmd_args+=" -b 1"
    fi

    cmd_args+=" -s $skip_iter_count"
    if [[ $cache_mode == "1" ]]
    then
        cmd_args+=" -p"
    fi

    cmd_args+=" -i $iter_count"
    # run with smaller size than default one to speedup execution"
    cmd_args+=" -c 1000"

    run_cmd "$env_cmd mpiexec -n $proc_count -ppn $proc_count ${SCRIPT_DIR}/${BINFILE} $cmd_args"
    rc=$?

    if [[ $rc -ne 0 ]]
    then
        echo "Fail"
    fi
}

function test_run() {
    #algos="topo ring"
    # TODO: MLSL-1358
    algos="topo"
    allreduce_modes="single multi"
    skip_iter_counts="0 10"
    cache_modes="0 1"
    iter_counts="20 1000"
    # TODO: MLSL-1296
    # proc_counts="2 4"
    proc_counts="2"

    common_env

    rm -rf ${TEST_LOG}

    for algo in $algos
    do
        for allreduce_mode in $allreduce_modes
        do
            for skip_iter_count in $skip_iter_counts
            do
                for cache_mode in $cache_modes
                do
                    for iter_count in $iter_counts
                    do
                        for proc_count in $proc_counts
                        do
                            single_test_run $algo $allreduce_mode $skip_iter_count $cache_mode $iter_count $proc_count
                        done
                    done
                done
            done
        done
    done

    check_log ${TEST_LOG}

    rm -f ${BINFILE} ${TEST_LOG}
    echo "Pass"
}

function perf_run() {
    common_env

    mpiexec -n 2 -ppn 2 ${SCRIPT_DIR}/${BINFILE}
    rc=$?
    if [[ ${rc} -ne 0 ]]
    then
        echo "Fail"
        exit 1
    fi

    echo "Pass"
}

echo "Running in $mode mode"
if [[ $mode == "test" ]]
then
    test_run
else
    perf_run
fi

