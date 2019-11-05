#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

declare -i total_fails=0
declare -i total_skipped=0

function CheckTest(){
    test_log=$1
    test_result=`grep -c "PASSED" ${test_log}`
    test_skipped=`grep -c "GPU is unavailable" ${test_log}`
    if [ ${test_result} -ne 1 ]
    then
        echo "Error: example $2 testing failed"
        echo "See log ${test_log} for details"
        total_fails=${total_fails}+1
    fi
    if [ ${test_skipped} -ne 0 ]
    then
        echo "GPU test for example  $2 has been skipped, default selector used instead."
        echo "See log ${test_log} for details"
        total_skipped=${total_skipped}+1
    fi
}

export FI_PROVIDER=tcp

which clang++
COMPILER_INSTALL_CHECK=$?
if [ "$COMPILER_INSTALL_CHECK" != "0" ]
then
    echo "Error: Need to source clang++ compiler"
    exit 1
fi

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
run_benchmark()
{
    echo "================ENVIRONMENT=================="
    local ccl_extra_env=$1
    echo "ccl_extra_env: " $ccl_extra_env
    local dir_name=$2
    echo "dir_name: " $dir_name
    local transport=$3
    echo "transport: "$transport
    local example=$4
    echo "example: "$example
    local backend=$5
    echo "backend: "$backend
    local loop=$6
    echo "loop: "$loop
    local coll=$7
    echo "coll: " $coll
    echo "================ENVIRONMENT=================="
    test_log="$SCRIPT_DIR/$dir_name/run_${transport}_${example}_${backend}_${loop}_output.log"
    echo "run ${example}: ccl_extra_env  "$ccl_extra_env" transport $transport, backend $backend, loop $loop " 2>&1 | tee ${test_log}
    eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop $coll` 2>&1 | tee ${test_log}
    CheckTest ${test_log} ${example}
}
run_example()
{
    echo "================ENVIRONMENT=================="
    local ccl_extra_env="$1"
    echo "ccl_extra_env: " "$ccl_extra_env"
    local dir_name=$2
    echo "dir_name: " $dir_name
    local transport=$3
    echo "transport: "$transport
    local example=$4
    echo "example: "$example
    echo "================ENVIRONMENT=================="
    test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_output.log"
    echo "run ${example}: ccl_extra_env "$ccl_extra_env" transport $transport" 2>&1 | tee ${test_log}
    eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example` 2>&1 | tee ${test_log}
    CheckTest ${test_log} ${example}
}

build()
{
    local dir_name="$1"
    echo "make clean"
    make clean -f ./../Makefile > /dev/null 2>&1
    make clean_logs -f ./../Makefile > /dev/null 2>&1
    echo "Building"
    make all -f ./../Makefile &> $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log
    error_count=`grep -E -c 'error:|Aborted|failed'  $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log` > /dev/null 2>&1
    if [ "${error_count}" != "0" ]
    then
        echo "building ... NOK"
        echo "See logs $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log"
        exit 1
    else
        echo "OK"
    fi
}
run()
{
    ppn=1
    EXTRA_ENV="CCL_YIELD=sleep"

    for dir_name in "cpu" "sycl" "common"
    do
        cd $dir_name
        build $dir_name
        for transport in "ofi" "mpi";
        do
            if [ "$transport" == "mpi" ];
            then
                examples_to_run=`ls . | grep '.out' | grep -v '.log' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce'`
            else
                examples_to_run=`ls . | grep '.out' | grep -v '.log'`
                # | grep -v 'allgatherv_iov' | grep -v 'allgatherv'`
            fi
            for example in $examples_to_run
            do
                if [ "$dir_name" == "common" ];
                then
                    for backend in "cpu" "sycl"
                    do
                        ccl_extra_env="CCL_ATL_TRANSPORT=${transport}"
                        run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} 
                        #run extended version of benchmark
                        if [[ "${example}" == *"benchmark"* ]]
                        then
                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                ccl_extra_env="CCL_PRIORITY=lifo CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop}
                                ccl_extra_env="CCL_WORKER_OFFLOAD=0 CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop}
                            done
                            ccl_extra_env="CCL_FUSION=1 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                            ccl_extra_env="CCL_LOG_LEVEL=2 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                        fi
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in "cpu" "gpu" "host" "default"
                    do
                        test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${selector}_${example}_output.log"
                        echo "run sycl examples with $transport transport and selector $selector (${example})" 2>&1 | tee ${test_log}
                        CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $selector 2>&1 | tee ${test_log}
                        CheckTest ${test_log} ${example}
                    done
                else
                    echo "run examples with $transport transport (${example})" 2>&1 | tee ${test_log}
                    if [[ "${example}" == *"sparse_allreduce"* ]]
                    then
                        for sparse_algo in "basic" "mask" "allgather" "size";
                        do
                            ccl_extra_env="CCL_SPARSE_ALLREDUCE=$sparse_algo CCL_ATL_TRANSPORT=${transport}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                        done
                    else
                        ccl_extra_env="CCL_ATL_TRANSPORT=${transport}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                    fi
                fi
            done
        done
        cd - > /dev/null 2>&1
    done
    if [ ${total_fails} != 0 ]
    then
        echo "There are ${total_fails} failed tests"
        exit 1
    elif [ ${total_skipped} != 0 ]
    then
        echo "Tests passed, except ${total_skipped} GPU skipped tests"
        exit 0
    else
        echo "All tests passed!"
        exit 0
    fi
}

run
