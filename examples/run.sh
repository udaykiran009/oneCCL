#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt


CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

export FI_PROVIDER=tcp

run_examples()
{
    for dir_name in "cpu" "sycl" "common"
    do
		cd $dir_name
        make clean -f ./../Makefile > /dev/null 2>&1
        make clean_logs -f ./../Makefile > /dev/null 2>&1	
		for transport in "ofi" "mpi";
		do
			make all -f ./../Makefile &> build_${dir_name}_${transport}_ouput.log
			error_count=grep -c 'error:'  build_${dir_name}_${transport}_ouput.log > /dev/null 2>&1
			if [ "${error_count}" == 0 ]
			then
				echo "building ... NOK"
				exit 1
			fi
			if [ "$transport" == "mpi" ];
			then
				examples_to_run=`ls . | grep '.out' | grep -v '.log' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce'`
			else
				examples_to_run=`ls . | grep '.out' | grep -v '.log'`
			fi
			for example in $examples_to_run
			do
				echo "run examples with $transport transport (${example})" >> ./run_${dir_name}_${transport}_ouput.log
				if [ "$dir_name" == "common" ];
				then
					for backend in "cpu" "sycl"
					do
					    CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -n 2 -ppn 1 -l ./$example $backend &>> ./run_${dir_name}_${transport}_ouput.log
				    done
				else
				    CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -n 2 -ppn 1 -l ./$example &>> ./run_${dir_name}_${transport}_ouput.log
			    fi
				grep -r 'PASSED' ./run_${dir_name}_${transport}_ouput.log > /dev/null 2>&1
				log_status=${PIPESTATUS[0]}
				if [ "$log_status" -ne 0 ]
				then
					echo "example ${example} testing ... NOK"
					exit 1
				fi
			done
		done
		cd - > /dev/null 2>&1
	done
    exit 0
}

run_examples
