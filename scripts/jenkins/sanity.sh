#!/bin/bash

echo "DEBUG: GIT_BRANCH = ${GIT_BRANCH}"
echo "DEBUG: ICCL_BUILD_ID = ${ICCL_BUILD_ID}"

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
fi

CURRENT_ICCL_BUILD_DIR="${ICCL_REPO_DIR}/${ICCL_BUILD_ID}"
ICCL_INSTALL_DIR="${SCRIPT_DIR}/../../build/_install"
BASENAME=`basename $0 .sh`
WORK_DIR=`cd ${SCRIPT_DIR}/../../ && pwd -P`
HOSTNAME=`hostname -s`

echo "SCRIPT_DIR = $SCRIPT_DIR"
echo "WORK_DIR = $WORK_DIR"
echo "ICCL_INSTALL_DIR = $ICCL_INSTALL_DIR"


#==============================================================================
#                                Defaults
#==============================================================================
set_default_values()
{
    ENABLE_DEBUG="no"
    ENABLE_VERBOSE="yes"
    ENABLE_BUILD_TESTS="yes"
    ENABLE_RUN_TESTS="yes"
}
#==============================================================================
#                                Functions
#==============================================================================

# Logging. Always log. Echo if ENABLE_VERBOSE is "yes".
log()
{
    if [ "${ENABLE_VERBOSE}" = "yes" ]; then
        eval $* 2>&1 | tee -a ${LOG_FILE}
    else
        eval $* >> ${LOG_FILE} 2>&1
    fi
}

# Print usage and help information
print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh <options>"
    echo_log ""
    echo_log "<options>:"
    echo_log "   ------------------------------------------------------------"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -h|-H|-help|--help"
    echo_log "        Print help information"
    echo_log ""
}
# Echo with logging. Always echo and always log
echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}
# echo the debug information
echo_debug()
{
    if [ ${ENABLE_DEBUG} = "yes" ]; then
        echo_log "DEBUG: $*"
    fi
}

echo_log_separator()
{
    echo_log "#=============================================================================="
}

check_command_exit_code() {
    if [ $1 -ne 0 ]; then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}
set_env()
{
	if [ -z "${I_MPI_HYDRA_HOST_FILE}" ]
	then
		if [ -f ${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts ]
		then
			export I_MPI_HYDRA_HOST_FILE=${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts
		else
			echo "WARNING: hostfile (${WORK_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts) isn't available"
			echo "WARNING: I_MPI_HYDRA_HOST_FILE isn't set"
		fi
	fi

	#export I_MPI_JOB_TIMEOUT=400

	if [ -n "${TESTING_ENVIRONMENT}" ]
	then
		echo "TESTING_ENVIRONMENT is " ${TESTING_ENVIRONMENT}
		for line in ${TESTING_ENVIRONMENT}
		do
			export "$line"
		done
	fi

	pwd
	rm -rf ${WORK_DIR}/_log
	mkdir -p ${WORK_DIR}/_log

	if [ -z "$runtime" ]
	then
		runtime="ofi"
	fi


	if [ -z "${IMPI_PATH}" ]
	then
		echo "WARNING: I_MPI_ROOT isn't set, impi2019u4 will be used."
		. /p/pdsd/scratch/jenkins/artefacts/impi-ch4-weekly/33/impi/intel64/bin/mpivars.sh release_mt
	else
		. ${IMPI_PATH}/intel64/bin/mpivars.sh release_mt
	fi


	if [ -z "${ICCL_ROOT}" ]
	then
		echo "WARNING: ICCL_ROOT isn't set"
		if [ -f ${ICCL_INSTALL_DIR}/intel64/bin/icclvars.sh ]
		then
			. ${ICCL_INSTALL_DIR}/intel64/bin/icclvars.sh
		else
			echo "ERROR: ${ICCL_INSTALL_DIR}/intel64/bin/icclvars.sh doesn't exist"
			exit 1
		fi
	fi
}
run()
{
    if [ "${ENABLE_BUILD_TESTS}" = "yes" ]
    then
        echo_log_separator
        echo_log "#\t\t\Building tests..."
        echo_log_separator
		make_tests
		echo_log_separator
        echo_log "#\t\t\Running tests..."
        echo_log_separator
		run_tests
        echo_log_separator
        echo_log "#\t\t\t... DONE"
        echo_log_separator
    fi
}
make_tests()
{
	cd ${WORK_DIR}/tests/functional
	mkdir -p build
	cd ./build
	cmake .. && make install
}
run_tests()
{
	case "$runtime" in
		   mpi )
			   export ICCL_ATL_TRANSPORT=MPI
			   ctest -VV -C Mpi
			   ;;
		   mpi_adjust )
				export ICCL_ATL_TRANSPORT=MPI
				for bcast in "ring" "double_tree" "direct"
					do
						ICCL_BCAST_ALGO=$bcast ctest -VV -C mpi_bcast_$bcast
					done
				for reduce in "tree" "double_tree" "direct"
					do
						ICCL_REDUCE_ALGO=$reduce ctest -VV -C mpi_reduce_$reduce
					done
				for allreduce in "tree" "starlike" "ring" "double_tree" "direct"
					do
						ICCL_ALLREDUCE_ALGO=$allreduce ctest -VV -C mpi_allreduce_$allreduce
					done
				for allgatherv in "naive" "direct"
					do
						ICCL_ALLGATHERV_ALGO=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
					done
			   ;;
		   ofi_adjust )
				for bcast in "ring" "double_tree"
					do
						ICCL_BCAST_ALGO=$bcast ctest -VV -C mpi_bcast_$bcast
					done
				for reduce in "tree" "double_tree"
					do
						ICCL_REDUCE_ALGO=$reduce ctest -VV -C mpi_reduce_$reduce
					done
				for allreduce in "tree" "starlike" "ring" "double_tree"
					do
						ICCL_ALLREDUCE_ALGO=$allreduce ctest -VV -C mpi_allreduce_$allreduce
					done
				for allgatherv in "naive"
					do
						ICCL_ALLGATHERV_ALGO=$allgatherv ctest -VV -C mpi_allgatherv_$allgatherv
					done
			   ;;
			priority_mode )
				ICCL_PRIORITY_MODE=lifo ctest -VV -C Default
				ICCL_PRIORITY_MODE=direct ctest -VV -C Default
			   ;;
			dynamic_pointer_mode )
				ICCL_TEST_DYNAMIC_POINTER=1 ctest -VV -C Default
			   ;;
			out_of_order_mode )
				ICCL_OUT_OF_ORDER_SUPPORT=1 ctest -VV -C Default
			   ;;
		   * )
			   ctest -VV -C Default
			   ;;
	esac
}

set_default_values
set_env
run

# echo "ICCL_ATL_TRANSPORT is " $ICCL_ATL_TRANSPORT

# FTESTS_DIR=${WORK_DIR}/tests/functional
# if [ "$coverage" = "true" ] && [ "$compiler" != "gcc" ]
# then

    # echo "Code Coverage"
    # cd $(UTESTS_DIR) && profmerge -prof_dpi pgopti_unit.dpi && cp pgopti_unit.dpi $(BASE_DIR)/pgopti_unit.dpi
    # cd $(FTESTS_DIR) && profmerge -prof_dpi pgopti_func.dpi && cp pgopti_func.dpi $(CURRENT_ICCL_BUILD_DIR)/pgopti_func.dpi
    # cd $(EXAMPLES_DIR) && profmerge -prof_dpi pgopti_examples.dpi && cp pgopti_examples.dpi $(BASE_DIR)/pgopti_examples.dpi
    # profmerge -prof_dpi pgopti.dpi -a pgopti_unit.dpi pgopti_func.dpi
    # codecov -prj iccl -comp $(ICT_INFRA_DIR)/code_coverage/codecov_filter_iccl.txt -spi $(TMP_COVERAGE_DIR)/pgopti.spi -dpi pgopti.dpi -xmlbcvrgfull codecov.xml -srcroot $(CODECOV_SRCROOT)
    # python $(ICT_INFRA_DIR)/code_coverage/codecov_to_cobertura.py codecov.xml coverage.xml
    # if [ $? -ne 0 ]
    # then
        # echo "make codecov... NOK"
        # exit 1
    # else
        # echo "make codecov... OK"
    # fi
# fi