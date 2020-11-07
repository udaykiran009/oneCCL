#!/bin/bash

BASENAME=`basename $0 .sh`
TIMEOUT=240

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
    echo_log "    -v  Path to oneCCL variables script"
    echo_log "    -h  Path to hostfile, one host per line"
    echo_log "    -s  Total number of ranks"
    echo_log ""
    echo_log "Example:"
    echo_log "    ./${BASENAME}.sh -v <vars_dir>/setvars.sh -h <hostfile_dir>/hostfile -s 4"
    echo_log ""
}

parse_arguments()
{
    if [ $# -ne 6 ];
    then
        echo_log "ERROR: unexpected number of options"
        print_help
        exit 1
    fi

    read_count=0

    while [ $# -ne 0 ]
    do
        case $1 in
            "-v"|"--vars")
                VARS=$2
                read_count=$((read_count+1))
                ;;
            "-h"|"--hostfile")
                HOSTFILE=$2
                read_count=$((read_count+1))
                ;;
            "-s"|"--size")
                SIZE=$2
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

    if [ ${read_count} -ne 3 ];
    then
        echo_log "ERROR: unexpected number of read options ($read_count), expected 3"
        print_help
        exit 1
    fi

    if [ ! -f $VARS ];
    then
        echo_log "ERROR: can not find vars file: ${VARS}"
        exit 1
    fi

    if [ ! -f $HOSTFILE ];
    then
        echo_log "ERROR: can not find hostfile: ${HOSTFILE}"
        exit 1
    fi

    if [ -z "${IMPI_PATH}" ];
    then
        echo_log "ERROR: set IMPI_PATH"
        exit 1
    fi

    unique_host_count=( `cat $HOSTFILE | grep -v ^$ | uniq | wc -l` )
    host_count=( `cat $HOSTFILE | grep -v ^$ | wc -l` )

    if [ "${unique_host_count}" != "${host_count}" ];
    then
        echo_log "ERROR: hostfile should contain unique hostnames"
        exit 1
    fi

    if [ "${host_count}" -eq "0" ];
    then
        echo_log "ERROR: hostfile should contain at least one row"
        exit 1
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "VARS     = ${VARS}"
    echo_log "HOSTFILE = ${HOSTFILE}"
    echo_log "SIZE     = ${SIZE}"
    echo_log "-----------------------------------------------------------"
}

cleanup_hosts()
{
    hostlist=$1

    echo "clean up hosts"
    for host in "${hostlist[@]}"
    do
        echo "host ${host}"
        cmd="killall -9 external_launcher"
        ssh ${host} $cmd
    done
}

run_binary()
{
    dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

    mapfile -t hostlist < $HOSTFILE
    host_count=( `cat $HOSTFILE | grep -v ^$ | wc -l` )

    local_size=$((SIZE / host_count))
    echo "local_size: ${local_size}"

    declare -a log_files=()

    cleanup_hosts $hostlist
    store_file="$dir/store"
    if [ -f $store_file ]
    then
        rm $store_file
    fi

    host_idx=0
    for host in "${hostlist[@]}"
    do
        echo "start ranks on host: $host"
        for ((i = 0; i < $local_size ; i++ ));
        do
            rank=$((host_idx * local_size + i))
            log_file="${dir}/${host}_${rank}.out"
            log_files=("${log_files[@]}" "${log_file}")
            cmd="$dir/run_binary.sh -cv ${VARS} -mv ${IMPI_PATH}/env/vars.sh -s ${SIZE} -r ${rank} -ls ${local_size} -lr ${i} -f ${log_file} -e ${store_file}"
            timeout -k $((TIMEOUT))s $((TIMEOUT))s ssh ${host} $cmd&
        done
        host_idx=$((host_idx + 1))
    done

    echo "wait completion"
    wait

    echo "check results"
    for file in "${log_files[@]}"
    do
        echo "check: $file"
        pass_count=`cat $file | grep "PASSED" | wc -l`
        if [ "${pass_count}" != "1" ]
        then
            echo -e "${RED}FAIL: expected 1 pass, got ${pass_count}${NC}"
        else
            echo -e "${GRN}PASSED${NC}"
        fi
    done

    cleanup_hosts $hostlist
    if [ -f $store_file ]
    then
        rm $store_file
    fi
}

run()
{
    RED='\033[0;31m'
    GRN='\033[0;32m'
    PUR='\033[0;35m'
    NC='\033[0m'

    echo -e "${PUR}START EXAMPLE${NC}"
    exec_time=`date +%s`

    run_binary

    exec_time="$((`date +%s`-$exec_time))"
    if [ "$exec_time" -ge "$TIMEOUT" ];
    then
         echo -e "${RED}FAILED: Timeout ($exec_time > $TIMEOUT)${NC}"
         exit 1
    fi
}

parse_arguments $@
run
