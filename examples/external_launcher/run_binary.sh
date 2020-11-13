#!/bin/bash

BASENAME=`basename $0 .sh`

echo_log()
{
    echo -e "$*"
}

print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh [options]"
    echo_log ""
    echo_log "<options>:"
    echo_log "    -s   Total number of ranks"
    echo_log "    -r   Rank"
    echo_log "    -ls  Local number of ranks"
    echo_log "    -lr  Local rank"
    echo_log "    -cv  Path to oneCCL variables script"
    echo_log "    -mv  Path to IMPI variables script"
    echo_log "    -lf  Log file"
    echo_log "    -sf  Store file"
    echo_log ""
    echo_log "Example:"
    echo_log "    ./${BASENAME}.sh -s 4 -r 0 -ls 2 -lr 0 -cv <ccl_vars> -mv <mpi_vars> -lf <log_file> -sf <store_file>"
    echo_log ""
}

parse_arguments()
{
    if [ $# -ne 16 ];
    then
        echo_log "ERROR: unexpected number of options"
        print_help
        exit 1
    fi

    read_count=0

    while [ $# -ne 0 ]
    do
        case $1 in
            "-s"|"--size")
                SIZE=$2
                read_count=$((read_count+1))
                ;;
            "-r"|"--rank")
                RANK=$2
                read_count=$((read_count+1))
                ;;
            "-ls"|"--local_size")
                LOCAL_SIZE=$2
                read_count=$((read_count+1))
                ;;
            "-lr"|"--local_rank")
                LOCAL_RANK=$2
                read_count=$((read_count+1))
                ;;
            "-cv"|"--ccl_vars")
                CCL_VARS=$2
                read_count=$((read_count+1))
                ;;
            "-mv"|"--mpi_vars")
                MPI_VARS=$2
                read_count=$((read_count+1))
                ;;
            "-lf"|"--log_file")
                LOG_FILE=$2
                read_count=$((read_count+1))
                ;;
            "-sf"|"--store_file")
                STORE_FILE=$2
                read_count=$((read_count+1))
                ;;
            *)
                echo_log "ERROR: unknown option ($1)"
                print_help
                exit 1
                ;;
        esac

        shift
        shift
    done

    expected_read_count=8
    if [ "${read_count}" -ne "${expected_read_count}" ];
    then
        echo_log "ERROR: unexpected number of read options ($read_count), expected ${expected_read_count}"
        print_help
        exit 1
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SIZE       = ${SIZE}"
    echo_log "RANK       = ${RANK}"
    echo_log "LOCAL_SIZE = ${LOCAL_SIZE}"
    echo_log "LOCAL_RANK = ${LOCAL_RANK}"
    echo_log "CCL_VARS   = ${CCL_VARS}"
    echo_log "MPI_VARS   = ${MPI_VARS}"
    echo_log "LOG_FILE   = ${LOG_FILE}"
    echo_log "STORE_FILE = ${STORE_FILE}"
    echo_log "-----------------------------------------------------------"
}

function run()
{
    dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
    host=`hostname`

    binary_env="FI_PROVIDER=tcp CCL_LOG_LEVEL=1 CCL_LOCALRANKID=$LOCAL_RANK CCL_LOCALNRANKS=$LOCAL_SIZE"
    binary_path="$dir/external_launcher"
    binary_arg="$SIZE $RANK ${STORE_FILE}"

    if [ -f $LOG_FILE ];
    then
        rm $LOG_FILE
    fi
    echo $LOG_FILE

    if [[ $CCL_VARS == *"setvars.sh"* ]];
    then
        echo "Use standalone CCL variables script"
    elif [[ $CCL_VARS == *"vars.sh"* ]];
    then
        echo "Use oneAPI CCL variables script"
        source ${MPI_VARS} -i_mpi_library_kind=release_mt
    fi

    export CCL_CONFIGURATION="cpu_icc"
    source ${CCL_VARS} --ccl-configuration="${CCL_CONFIGURATION}"

    eval `echo $binary_env $binary_path $binary_arg ;` &> $LOG_FILE
}

parse_arguments $@
run
