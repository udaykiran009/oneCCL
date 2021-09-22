#!/bin/bash

BASENAME=`basename $0 .sh`
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
if [ -z ${SCRIPT_DIR} ]
then
    echo "ERROR: unable to define SCRIPT_DIR"
    exit 1
else
    echo "SCRIPT_DIR: ${SCRIPT_DIR}"
fi

current_date=`date "+%Y%m%d%H%M%S"`
LOG_FILE="${SCRIPT_DIR}/perf_log_${current_date}"
touch ${LOG_FILE}

CCL_ENV="CCL_LOG_LEVEL=info CCL_ATL_TRANSPORT=ofi"
CCL_ENV="${CCL_ENV} CCL_MNIC=global CCL_MNIC_COUNT=2"
CCL_ENV="${CCL_ENV} CCL_ALLREDUCE=ring CCL_MAX_SHORT_SIZE=16384"
MPI_ENV="I_MPI_DEBUG=12"
PSM3_ENV="PSM3_MULTI_EP=1 PSM3_RDMA=0 PSM3_MR_CACHE_MODE=2 PSM3_IDENTIFY=1 PSM3_ALLOW_ROUTERS=1"
BASE_ENV="${CCL_ENV} ${MPI_ENV}"

EXTERNAL_ITER_COUNT="5"
PROVS="verbs psm3"
NODE_COUNTS="2 4 8 16"
WORKER_COUNTS="1 2 4 8"
COLLS="allreduce"
NUMA_NODES="0 1"

CCL_LINK="https://github.com/intel-innersource/libraries.performance.communication.oneccl.git"
CCL_BRANCH="master"

DEFAULT_SCRIPT_WORK_DIR="${SCRIPT_DIR}/work_dir_${current_date}"
DEFAULT_FULL_SCOPE="0"
DEFAULT_DOWNLOAD_CCL="0"
DEFAULT_INSTALL_CCL="0"
DEFAULT_RUN_TESTS="0"
DEFAULT_PROCESS_OUTPUT="0"
DEFAULT_MSG_SIZES="trace"

echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

check_exit_code() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}" 1>&2
        echo_log "SCRIPT FAILED"
        exit $1
    fi
}

print_help() {
    echo_log ""
    echo_log "Requirements:"
    echo_log "- export I_MPI_HYDRA_HOST_FILE=<file>"
    echo_log "  <file> should be path to host file, one unique host per line"
    echo_log ""
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  -work_dir <dir>"
    echo_log "      Use existing work directory"
    echo_log "  -full <bool_flag>"
    echo_log "      Run full scope of steps"
    echo_log "  -download_ccl <bool_flag>"
    echo_log "      Download oneCCL"
    echo_log "  -install_ccl <bool_flag>"
    echo_log "      Install oneCCL"
    echo_log "  -run_tests <bool_flag>"
    echo_log "      Run CCL benchmark with different options"
    echo_log "  -process_output <path>"
    echo_log "      Do post-processing for output file (convert raw log to csv log)"
    echo_log ""
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh -full 1"
    echo_log ""
}

parse_arguments() {
    SCRIPT_WORK_DIR=${DEFAULT_SCRIPT_WORK_DIR}
    FULL_SCOPE=${DEFAULT_FULL_SCOPE}

    DOWNLOAD_CCL=${DEFAULT_DOWNLOAD_CCL}
    INSTALL_CCL=${DEFAULT_INSTALL_CCL}

    RUN_TESTS=${DEFAULT_RUN_TESTS}
    PROCESS_OUTPUT=${DEFAULT_PROCESS_OUTPUT}
    MSG_SIZES=${DEFAULT_MSG_SIZES}

    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-work_dir")
                SCRIPT_WORK_DIR=${2}
                shift
                ;;
            "-full")
                FULL_SCOPE=$2
                shift
                ;;
            "-download_ccl")
                DOWNLOAD_CCL=${2}
                shift
                ;;
            "-install_ccl")
                INSTALL_CCL=${2}
                shift
                ;;
            "-run_tests")
                RUN_TESTS=${2}
                shift
                ;;
            "-process_output")
                PROCESS_OUTPUT=${2}
                shift
                ;;
            "-msg_sizes")
                MSG_SIZES=${2}
                shift
                ;;
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    mkdir -p ${SCRIPT_WORK_DIR}

    CCL_SRC_DIR=${SCRIPT_WORK_DIR}/ccl
    CCL_INSTALL_DIR=${CCL_SRC_DIR}/build/_install

    if [[ ${FULL_SCOPE} = "1" ]]
    then
        DOWNLOAD_CCL="1"
        INSTALL_CCL="1"
        RUN_TESTS="1"
        PROCESS_OUTPUT="0"

        if [[ -d ${SCRIPT_WORK_DIR} ]]
        then
            rm -rf ${SCRIPT_WORK_DIR}
            mkdir -p ${SCRIPT_WORK_DIR}
        fi
    fi

    if [[ ${RUN_TESTS} = "1" ]] && [[ -f ${PROCESS_OUTPUT} ]]
    then
        echo_log "RUN_TESTS and PROCESS_OUTPUT actions are requested both"
        echo_log "Please specify single action only"
        return
    fi

    if [[ ${MSG_SIZES} = "regular" ]]
    then
        msg_sizes_arg="-f 16384 -t 67108864"
    elif [[ ${MSG_SIZES} = "trace" ]]
    then
        msg_sizes_arg="-y 1,188418,821760,2650112,4201474,4773198,6553600,19142658,21077326,21703312,24188816,40465152,40598530"
    else
        check_exit_code 1 "unknown msg_sizes '${MSG_SIZES}', select from: regular, trace"
    fi

    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"
    echo_log "SCRIPT_WORK_DIR     = ${SCRIPT_WORK_DIR}"
    echo_log "FULL_SCOPE          = ${FULL_SCOPE}"

    echo_log "DOWNLOAD_CCL        = ${DOWNLOAD_CCL}"
    echo_log "INSTALL_CCL         = ${INSTALL_CCL}"

    echo_log "RUN_TESTS           = ${RUN_TESTS}"
    echo_log "PROCESS_OUTPUT      = ${PROCESS_OUTPUT}"

    echo_log "MSG_SIZES           = ${MSG_SIZES}"

    echo_log "EXTERNAL_ITER_COUNT = ${EXTERNAL_ITER_COUNT}"
}

download_ccl() {
    if [[ ${DOWNLOAD_CCL} != "1" ]]
    then
        return
    fi

    if [[ -d ${CCL_SRC_DIR} ]]
    then
        rm -rf ${CCL_SRC_DIR}
    fi

    cd ${SCRIPT_WORK_DIR}
    git clone --branch ${CCL_BRANCH} --single-branch ${CCL_LINK} ${CCL_SRC_DIR}
    check_exit_code $? "failed to download CCL"
}

install_ccl() {
    if [[ ${INSTALL_CCL} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_SRC_DIR} ]]
    then
        echo_log "ERROR: CCL_SRC_DIR (${CCL_SRC_DIR}) is not directory, try to run script with \"-download_ccl 1\""
        check_exit_code 1 "failed to install CCL"
    fi

    cd ${CCL_SRC_DIR}

    rm -rf build
    mkdir build
    cd build

    # intentionally build CCL with MPI support
    # to get MPI launcher in CCL install dir to be used for test launches below
    cmake .. -DBUILD_FT=0 -DBUILD_EXAMPLES=1 -DBUILD_CONFIG=0 -DCMAKE_INSTALL_PREFIX=${CCL_INSTALL_DIR} \
        -DENABLE_MPI=1 -DENABLE_MPI_TESTS=1
    check_exit_code $? "failed to configure CCL"

    make -j install
    check_exit_code $? "failed to install CCL"
}

convert_to_csv() {
    input_log=$1

    echo_log "convert to csv\ninput log: ${input_log}"

    line_count=$(cat ${input_log} | wc -l)
    if [[ ${line_count} = "0" ]]
    then
        echo_log "no data to process"
        return
    fi

    output_log=${input_log}".csv"
    rm -f ${output_log}

    size_cmd="sed -n -e '/#bytes/,/# All done/ p' ${input_log} \
            | grep -v \"#bytes\" | grep -v \"All\" | awk '{ print \$2 }' \
            | awk 'NF' | sort -n | uniq"
    #echo "size_cmd: ${size_cmd}"
    eval sizes=\$\(${size_cmd}\)

    size_count=$(echo -e "${sizes}" | wc -l)
    #echo "size_count: ${size_count}"

    size_cmd="${size_cmd} | awk '{ ORS=\";\" } { print \$1 }'"
    #echo "size_cmd: ${size_cmd}"
    eval sizes=\$\(${size_cmd}\)

    node_count_cmd="cat ${input_log} | grep \"exec config\" \
        | awk '{ print \$4 }' | sed -e \"s/nodes=//\" | sort -n | uniq"
    # echo "node_count_cmd: ${node_count_cmd}"
    eval node_counts=\$\(${node_count_cmd}\)

    for node_count in ${node_counts}
    do
        tmp_log=${input_log}".tmp.csv"
        rm -f ${tmp_log}

        tmp_avg_log=${input_log}".tmp.avg.csv"
        rm -f ${tmp_avg_log}

        echo -e "\nnode_count_${node_count}" >> ${output_log}

        echo -n "bytes;${sizes}" >> ${tmp_log}
        echo "" >> ${tmp_log}

        config_name_cmd="cat ${input_log} | grep nodes=${node_count}"
        #echo "config_name_cmd: ${config_name_cmd}"
        eval config_names=\$\(${config_name_cmd}\)

        # iterate over different configs for specific node_count
        while IFS= read -r config_name
        do
            config_short_name_cmd="echo "\${config_name}" | awk '{ print \$5\"_\"\$7 }' \
                | sed -e \"s/workers=/w/\" | sed -e \"s/numa_node=/n/\""
            #echo "config_short_name_cmd: ${config_short_name_cmd}"
            eval config_short_name=\$\(${config_short_name_cmd}\)

            echo -n "${config_short_name};" >> ${tmp_log}

            config_values_cmd="cat ${input_log} | grep \"${config_name}\" -A 2000 \
                | sed -n -e '/#bytes/,/# All done/ p' | grep -v \"#bytes\" | grep -v \"All done\" \
                | head -n ${size_count} | awk -n '{ print \$5 }' | awk 'NF' \
                | awk '{ print \$1 }' ORS=\";\""
            #echo "config_values_cmd: ${config_values_cmd}"
            eval config_values=\$\(${config_values_cmd}\)
            echo -e "${config_values}" >> ${tmp_log}
        done <<< "${config_names}"

        uniq_config_name_cmd="cat ${tmp_log} | grep -v bytes | awk -F ';' '{ print \$1 }' | sort | uniq"
        #echo "uniq_config_name_cmd: ${uniq_config_name_cmd}"
        eval uniq_config_names=\$\(${uniq_config_name_cmd}\)

        cat ${tmp_log} | grep "bytes" > ${tmp_avg_log}

        for config_name in ${uniq_config_names}
        do
            echo -n "${config_name};" >> ${tmp_avg_log}

            config_values_cmd="cat ${tmp_log} | grep ${config_name} \
                | awk -F ';' '{ \$1=\"\"; print \$0 }' \
                | awk -F ' ' \
                    '{ for (i = 1; i <= NF; i++) { arr[NR,i] = \$i } } NF>field_count { field_count = NF } \
                    END { for (j = 1; j <= field_count; j++) { \
                              { sum = 0 } \
                              for (i = 1; i <= NR; i++) { \
                                  sum += arr[i,j] \
                              } \
                              { res = sum / NR } \
                              { printf \"%.1f;\", res } \
                           } }'"
            #echo "config_values_cmd: ${config_values_cmd}"
            eval config_values=\$\(${config_values_cmd}\)
            echo -e "${config_values}" >> ${tmp_avg_log}
        done

        transpose_cmd="cat ${tmp_avg_log} \
            | awk -F ';' \
              '{ for (i = 1; i <= NF; i++) { arr[NR,i] = \$i } } NF>field_count { field_count = NF }
              END { for (j = 1; j <= field_count; j++) { \
                        { str = arr[1,j] } \
                        { if (str == \"\") break }
                        for (i = 2; i <= NR; i++) { \
                            str = str\";\"arr[i,j] \
                        } \
                        print str } }'"
        #echo "transpose_cmd: ${transpose_cmd}"
        eval transpose_values=\$\(${transpose_cmd}\)
        echo "${transpose_values}" >> ${output_log}
    done

    rm -f ${tmp_log}
    rm -f ${tmp_avg_log}
    sed -i 's/\./,/g' ${output_log}

    echo_log "output log: ${output_log}"
}

run_tests() {
    if [[ ${RUN_TESTS} != "1" ]]
    then
        return
    fi

    if [[ ! -d ${CCL_INSTALL_DIR} ]]
    then
        echo_log "can not find CCL install dir: ${CCL_INSTALL_DIR}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi

    bench_path="${CCL_INSTALL_DIR}/examples/benchmark/benchmark"
    if [[ ! -f ${bench_path} ]]
    then
        echo_log "can not find CCL benchmark: ${bench_path}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi

    vars_file="${CCL_INSTALL_DIR}/env/setvars.sh"
    if [[ ! -f ${vars_file} ]]
    then
        echo_log "can not find CCL vars file: ${vars_file}"
        echo_log "please run script with: -download_ccl 1 -install_ccl 1"
        check_exit_code 1 "failed to run tests"
    fi
    source ${vars_file}

    if [[ ! -d ${CCL_ROOT} ]]
    then
        echo_log "can not find CCL root dir: ${CCL_ROOT}"
        echo_log "please source CCL vars script"
        check_exit_code 1 "failed to run tests"
    fi
    echo "CCL_ROOT ${CCL_ROOT}"

    hostfile=${I_MPI_HYDRA_HOST_FILE}

    if [[ ! -f ${hostfile} ]]
    then
        echo_log "can not find hydra hostfile: ${hostfile}"
        echo_log "please set env var I_MPI_HYDRA_HOST_FILE with path to hostile"
        check_exit_code 1 "failed to run tests"
    fi
    echo "I_MPI_HYDRA_HOST_FILE ${hostfile}"

    unique_node_count=( `cat $hostfile | grep -v ^$ | uniq | wc -l` )
    total_node_count=( `cat $hostfile | grep -v ^$ | wc -l` )

    if [ "${unique_node_count}" != "${total_node_count}" ]
    then
        check_exit_code 1 "hostfile should contain unique hostnames"
    fi

    if [ "${total_node_count}" -eq "0" ]
    then
        check_exit_code 1 "hostfile should contain at least one row"
    fi

    cores_per_socket=$(lscpu | grep "Core(s) per socket:" | awk '{ print $4 }')

    for prov in ${PROVS}
    do
        PROV_LOG_FILE="${LOG_FILE}_${prov}"
        touch ${PROV_LOG_FILE}

        for node_count in ${NODE_COUNTS}
        do
            if [ "${node_count}" -gt "${total_node_count}" ]
            then
                echo_log "no enough nodes to run at scale ${node_count}, max available ${total_node_count}"
                continue
            fi

            for worker_count in ${WORKER_COUNTS}
            do
                # workers + main thread + skip core 0
                required_core_count=$((worker_count+2))
                if [ "${required_core_count}" -gt "${cores_per_socket}" ]
                then
                    echo_log "no enough cores per socket to run with ${worker_count} workers, max available ${cores_per_socket}"
                    continue
                fi

                for coll in ${COLLS}
                do
                    for numa_node in ${NUMA_NODES}
                    do
                        main_core=$((worker_count+1))
                        if [[ ${numa_node} = "1" ]]
                        then
                            # use first core from NUMA node 1
                            main_core=$(lscpu | grep "NUMA node1 CPU(s):" | awk '{ print $4 }' | awk -F "-|," '{ print $1 }')
                        fi

                        config_env="${BASE_ENV} I_MPI_PIN_PROCESSOR_LIST=${main_core}"
                        config_env="${config_env} CCL_WORKER_AFFINITY=1-${worker_count}"
                        config_env="${config_env} CCL_WORKER_COUNT=${worker_count}"
                        config_env="${config_env} FI_PROVIDER=${prov}"
                        if [[ ${prov} = "psm3" ]]
                        then
                            config_env="${config_env} ${PSM3_ENV}"
                        fi

                        bench_args="-c all -i 30 -w 10 -j off -d float32 -p 0 -l ${coll} -s ${numa_node}"
                        if [[ ${coll} = "allreduce" ]]
                        then
                            bench_args="-q 1 ${bench_args}"
                        fi
                        bench_args="${msg_sizes_arg} ${bench_args}"

                        cmd="${config_env} mpiexec -l -n ${node_count} -ppn 1"
                        cmd="${cmd} ${bench_path} ${bench_args} 2>&1 | tee -a ${PROV_LOG_FILE}"

                        for iter in `seq 1 ${EXTERNAL_ITER_COUNT}`
                        do
                            config="prov=${prov} nodes=${node_count} workers=${worker_count}"
                            config="${config} coll=${coll} numa_node=${numa_node} iter=${iter}"
                            echo -e "\nexec config: ${config}\n" | tee -a ${PROV_LOG_FILE}
                            echo -e "\n${cmd}\n" | tee -a ${PROV_LOG_FILE}
                            echo -e "\n$(lscpu)\n" | tee -a ${PROV_LOG_FILE}
                            eval ${cmd}
                            check_exit_code $? "${config} failed"
                        done
                    done
                done
            done
        done

        fail_pattern="abort|^bad$|corrupt|^fault$|invalid|kill|runtime_error|terminate|timed|unexpected"
        ignore_fail_pattern=""

        fail_strings=`grep -E -i "${fail_pattern}" ${PROV_LOG_FILE}`
        if [[ ! -z ${ignore_fail_pattern} ]]
        then
            fail_strings=`echo "${fail_strings}" | grep -E -i -v "${ignore_fail_pattern}"`
        fi
        fail_count=`echo "${fail_strings}" | sed '/^$/d' | wc -l`

        echo_log ""
        echo_log "prov log: ${PROV_LOG_FILE}"
        echo_log "work dir: ${SCRIPT_WORK_DIR}"
        echo_log ""

        if [[ ${fail_count} -ne 0 ]]
        then
            echo_log ""
            echo_log "fail strings:"
            echo_log ""
            echo_log "${fail_strings}"
            echo_log ""
            check_exit_code ${fail_count} "TEST FAILED"            
        fi

        echo_log "TEST PASSED for prov ${prov}"
        convert_to_csv ${PROV_LOG_FILE}
    done

    echo_log "TEST PASSED"
}

process_output() {
    if [[ ! -f ${PROCESS_OUTPUT} ]]
    then
        return
    fi

    convert_to_csv ${PROCESS_OUTPUT}
}

parse_arguments $@

download_ccl
install_ccl

run_tests

process_output
