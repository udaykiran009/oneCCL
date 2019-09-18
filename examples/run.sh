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

function CheckTest(){
    test_log=$1
    test_result=`grep -c "PASSED" ${test_log}`
    if [ ${test_result} -ne 1 ]
    then
        echo "Error: example $2 testing failed"
        echo "See log ${test_log} for details"
        total_fails=${total_fails}+1
#        exit 1
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

run_examples()
{
    ppn=1
    EXTRA_ENV="CCL_YIELD=sleep"

    for dir_name in "cpu" "sycl" "common"
    do
        cd $dir_name
        echo "make clean"
        make clean -f ./../Makefile > /dev/null 2>&1
        make clean_logs -f ./../Makefile > /dev/null 2>&1
        echo "Building"
        make all -f ./../Makefile &> $SCRIPT_DIR/$dir_name/build_${dir_name}_${transport}_output.log
        error_count=`grep -c 'error:'  $SCRIPT_DIR/$dir_name/build_${dir_name}_${transport}_output.log` > /dev/null 2>&1
        if [ "${error_count}" != "0" ]
        then
            echo "building ... NOK"
            echo "See logs $SCRIPT_DIR/$dir_name/${dir_name}/build_${dir_name}_${transport}_output.log"
            exit 1
        else
            echo "OK"
        fi
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
                        test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_${backend}_output.log"
                        echo "run examples with $transport transport (${example})" 2>&1 | tee ${test_log}
                        CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend 2>&1 | tee ${test_log}
                        CheckTest ${test_log} ${example}

                        #run extended version of benchmark
                        if [[ "${example}" == *"benchmark"* ]]
                        then
                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_${backend}_${loop}_output.log"
                                echo "run benchmark: transport $transport, backend $backend, loop $loop " 2>&1 | tee ${test_log}
                                CCL_PRIORITY=lifo CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop 2>&1 | tee ${test_log}
                                CheckTest ${test_log} ${example}

                                test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_allreduce_wo_workers_output.log"
                                echo "run benchmark: transport $transport, backend $backend, loop $loop without workers" 2>&1 | tee ${test_log}
                                CCL_WORKER_OFFLOAD=0 CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop allreduce 2>&1 | tee ${test_log}
                                CheckTest ${test_log} ${example}
                            done

                            test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_allreduce_fusion_output.log"
                            echo "run benchmark with fusion: $transport transport (${example})" 2>&1 | tee ${test_log}
                            CCL_FUSION=1 CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend regular allreduce 2>&1 | tee ${test_log}
                            CheckTest ${test_log} ${example}

                            test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_allreduce_log_level_2_output.log"
                            echo "run benchmark with log_level=2: $transport transport (${example})" 2>&1 | tee ${test_log}
                            CCL_LOG_LEVEL=2 CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend regular allreduce 2>&1 | tee ${test_log}
                            CheckTest ${test_log} ${example}
                        fi
                    done
                else
                    test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_output.log"
                    echo "run examples with $transport transport (${example})" 2>&1 | tee ${test_log}
                    CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example 2>&1 | tee ${test_log}
                    CheckTest ${test_log} ${example}
                fi
            done
        done
        cd - > /dev/null 2>&1
    done
    if [ ${total_fails} != 0 ]
    then
        echo "There are ${total_fails} failed tests"
        exit 1
    else
        echo "All tests passed"
        exit 0
    fi
}

run_examples
