#!/bin/bash

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

set_default_values()
{
	LOGIN=$(whoami)
    SYSTEM="207.108.8.122"
    SYSTEM_NAME="DG1"
    LOG_PATH="/p/pdsd/Users/dsazanov/ccl-performance/LOG"
    COMPILER_PATH=""
    MPI_PATH=""
    CCL_PATH=""
    WORK_DIR="/panfs/users/dsazanov/performance"
    CCL_PATH_FIRST=""
    CCL_PATH_SECOND=""
    BENCH_CONF_N="2"
    BENCH_CONF_PPN="1"
    RAW_CONVERTER_PATH="/p/pdsd/scratch/Uploads/oneCCL_performance_tool/Core/"
}


help() {
    echo "Usage:"
    echo "    ./gpu_performance.sh <options>"
    echo "       Example: gpu_performance.sh -l gta -s 10.1.40.12 -w /home/gta/dsazanov/performance -n 2 --ccl_path /p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/"
    echo "<options>:"
    echo "   ------------------------------------------------------------"
    echo "    -l|--login <LOGIN>"
    echo "        Specifies the login used to connect to Endeavour (DG1) or gtax (ATS)"
    echo "    -s|--system <SYSTEM_NAME>"
    echo "        Specifies the name of the system to be used"
    echo "       Default values: ATS - 10.1.40.12; DG1 - 207.108.8.122"
    echo "    -w|--work_dir <PATH_TO_WORK_DIR>"
    echo "        Specifies the path to the main working folder"
    echo "       Example: Work dir for ATS (gtax) - /home/gta/<username>/performance; Work dir for DG1 (Endeavour) - /panfs/users/<username>/performance"
    echo "    -c|--comparison <CCL_PATH_1> <CCL_PATH_2>"
    echo "        Use this key if you want to compare two oneCCL builds with each other"
    echo "       Example: -c /path/to/ccl_1 /path/to/ccl_2"
    echo "    -n <NUMBER_OF_NODES>"
    echo "        Sets the number of nodes"
    echo "    -ppn <NUMBER_OF_PPN>"
    echo "        Sets the PPN"
    echo "    --ccl_path"
    echo "        Sets the path to oneCCL"
    echo "       Example: -ccl_path /path/to/ccl_1"
    echo "    --log_path <PATH_TO_LOG_DIR>"
    echo "        Sets the path to log files that will be recorded during operation"
    echo "    --mpi_path <PATH_TO_MPI>"
    echo "        Sets the path to MPI"
    echo "    --compiler_path <PATH_TO_COMPILER>"
    echo "        Sets the path to compiler"
    echo "    --raw_conv_path <PATH_TO_RAW_CONVERTER>"
    echo "        Sets the path to raw converter"
    echo "   ------------------------------------------------------------"
    echo "    -h|--help"
    echo "        Print help information"
    echo ""
}

info() {
    echo ""
    echo "   INFO: LOGIN             = ${LOGIN}"
    echo "   INFO: SYSTEM            = ${SYSTEM}"
    echo "   INFO: LOG_PATH          = ${LOG_PATH}"
    echo "   INFO: COMPILER_PATH     = ${COMPILER_PATH}"
    echo "   INFO: MPI_PATH          = ${MPI_PATH}"
    echo "   INFO: CCL_PATH          = ${CCL_PATH}"
    echo "   INFO: WORK_DIR          = ${WORK_DIR}"
    echo "   INFO: CCL_PATH_FIRST    = ${CCL_PATH_FIRST}"
    echo "   INFO: CCL_PATH_SECOND   = ${CCL_PATH_SECOND}"
    echo "   INFO: BENCH_CONF_N      = ${BENCH_CONF_N}"
    echo "   INFO: BENCH_CONF_PPN    = ${BENCH_CONF_PPN}"
    echo ""
}

checking_and_copying_components() {

    scp -q /nfs/inn/proj/mpi/users/dsazanov/ccl-performance/ccl_performance.sh ${LOGIN}@${SYSTEM}:${WORK_DIR}
    scp -q /nfs/inn/proj/mpi/users/dsazanov/ccl-performance/installing_components.sh ${LOGIN}@${SYSTEM}:${WORK_DIR}/oneapi

    #SET VALUES FOR COMPILER
    if [ -z "$COMPILER_PATH" ]
    then
        echo "MESSAGE: Using the default compiler path"
        export COMPILER_PATH="/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/CCL_oneAPI/compiler/last/"
        NEW_COMPILER="$(ls ${COMPILER_PATH}/.. -t | head -1)"
        export COMPILER_PATH=${COMPILER_PATH}/../${NEW_COMPILER}
        echo "   INFO: COMPILER_PATH = ${COMPILER_PATH}"
        echo "   INFO: NEW_COMPILER  = ${NEW_COMPILER}"
    else
        NEW_COMPILER="$(echo ${COMPILER_PATH} | grep -o '[^/]*$')"
        echo "   INFO: COMPILER_PATH = ${COMPILER_PATH}"
        echo "   INFO: NEW_COMPILER  = ${NEW_COMPILER}"
    fi

    #SET VALUES FOR MPI
    if [ -z "$MPI_PATH" ]
    then
        echo "MESSAGE: Using the default MPI path"
        export MPI_PATH="/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/CCL_oneAPI/mpi_oneapi/last/"
        NEW_MPI="$(ls ${MPI_PATH}/.. -t | head -1)"
        export MPI_PATH=${MPI_PATH}/../${NEW_MPI}
        echo "   INFO: MPI_PATH = ${MPI_PATH}"
        echo "   INFO: NEW_MPI  = ${NEW_MPI}"
    else
        NEW_MPI="$(echo ${MPI_PATH} | grep -o '[^/]*$')"
        echo "   INFO: MPI_PATH = ${MPI_PATH}"
        echo "   INFO: NEW_MPI  = ${NEW_MPI}"
    fi

    #GET INSALLED COMPONENTS
    ACTUAL_COMPILER="$(ssh -q ${LOGIN}@${SYSTEM} "ls ${WORK_DIR}/oneapi/compiler/ -t | tail -1")"
    echo "   INFO: ACTUAL_COMPILER = ${ACTUAL_COMPILER}"
    ACTUAL_MPI="$(ssh -q ${LOGIN}@${SYSTEM} "ls ${WORK_DIR}/oneapi/mpi_oneapi/ -t | tail -1")"
    echo "   INFO: ACTUAL_MPI = ${ACTUAL_MPI}"

    #COPY NEW COMPILER
    if [[ ${ACTUAL_COMPILER} == ${NEW_COMPILER} ]]
    then
        echo "MESSAGE: This compiler is already installed"
    else
        echo "MESSAGE: Installing the latest compiler: ${ACTUAL_COMPILER} ---> ${NEW_COMPILER}"
        ssh -q ${LOGIN}@${SYSTEM} "rm -rf ${WORK_DIR}/oneapi/compiler/*"
        scp -q ${COMPILER_PATH} ${LOGIN}@${SYSTEM}:${WORK_DIR}/oneapi/compiler/
        CheckCommandExitCode $? "  ERROR: Copying the latest compiler: ERROR"
        echo "MESSAGE: Copying the latest compiler: SUCCESS"
        ssh -q ${LOGIN}@${SYSTEM} "${WORK_DIR}/oneapi/installing_components.sh compiler ${SYSTEM_NAME}"
    fi

    #COPY NEW MPI
    if [[ ${ACTUAL_MPI} == ${NEW_MPI} ]]
    then
        echo "MESSAGE: This MPI is already installed"
    else
        echo "MESSAGE: Installing the latest MPI: ${ACTUAL_MPI} ---> ${NEW_MPI}"
        ssh -q ${LOGIN}@${SYSTEM} "rm -rf ${WORK_DIR}/oneapi/mpi_oneapi/*"
        scp -qr ${MPI_PATH} ${LOGIN}@${SYSTEM}:${WORK_DIR}/oneapi/mpi_oneapi/
        CheckCommandExitCode $? "  ERROR: Copying the latest MPI: ERROR"
        echo "MESSAGE: Copying the latest MPI: SUCCESS"
        ssh -q ${LOGIN}@${SYSTEM} "${WORK_DIR}/oneapi/installing_components.sh mpi_oneapi ${SYSTEM_NAME}"
    fi
}

copy_ccl() {
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]
    then
        if [ -z ${CCL_PATH} ]
        then
            echo "MESSAGE: Using the default ccl path"
            export CCL_PATH="/p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/"
            #CCL_PACKAGE_NAME=$(find ${CCL_PATH} -maxdepth 1 -name "l_ccl_release*.tgz" | grep -o '[^/]*$')
            CCL_PACKAGE_NAME="ccl_src.tgz"
            export CCL_PATH=${CCL_PATH}/${CCL_PACKAGE_NAME}
            echo "   INFO: CCL_PACKAGE_NAME = ${CCL_PACKAGE_NAME}"
            echo "   INFO: CCL_PATH         = ${CCL_PATH}"
        else
            CCL_PACKAGE_NAME=$(echo ${CCL_PATH} | grep -o '[^/]*$')
            echo "   INFO: CCL_PACKAGE_NAME = ${CCL_PACKAGE_NAME}"
            echo "   INFO: CCL_PATH         = ${CCL_PATH}"
        fi

        echo "MESSAGE: Removing old ccl"
        ssh -q ${LOGIN}@${SYSTEM} "rm -rf ${WORK_DIR}/ccl/*"
        echo "MESSAGE: Copying ${CCL_PACKAGE_NAME}"
        scp -qr ${CCL_PATH} ${LOGIN}@${SYSTEM}:${WORK_DIR}/ccl/
        CheckCommandExitCode $?  "  ERROR: copy ccl: ERROR"
    else
        echo "MESSAGE: Compare ccl packages"
        echo "   INFO: CCL_PATH_FIRST         = ${CCL_PATH_FIRST}"
        echo "   INFO: CCL_PATH_SECOND        = ${CCL_PATH_SECOND}"
        echo "MESSAGE: Removing old ccl_c1 & ccl_c2"
        ssh -q ${LOGIN}@${SYSTEM} "rm -rf ${WORK_DIR}/ccl_c1/* | rm -rf ${WORK_DIR}/ccl_c2/*"

        echo "MESSAGE: Copying ${CCL_PATH_FIRST}"
        scp -qr ${CCL_PATH_FIRST} ${LOGIN}@${SYSTEM}:${WORK_DIR}/ccl_c1/
        CheckCommandExitCode $?  "  ERROR: copy ccl_c1: ERROR"

        echo "MESSAGE: Copying ${CCL_PATH_SECOND}"
        scp -qr ${CCL_PATH_SECOND} ${LOGIN}@${SYSTEM}:${WORK_DIR}/ccl_c2/
        CheckCommandExitCode $?  "  ERROR: copy ccl_c2: ERROR"
    fi
}

get_logs() {
    LOG_FOLDER_NAME="$(date +%Y.%m.%d-%H:%M)_${SYSTEM_NAME}"
    mkdir -p ${LOG_PATH}/${LOG_FOLDER_NAME}
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]
    then
        scp -r ${LOGIN}@${SYSTEM}:${WORK_DIR}/logs/single/* ${LOG_PATH}/${LOG_FOLDER_NAME}
    else
        scp -r ${LOGIN}@${SYSTEM}:${WORK_DIR}/logs/comparsion/* ${LOG_PATH}/${LOG_FOLDER_NAME}
    fi
}

start_bench() {
    if [[ -z ${CCL_PATH_FIRST} && -z ${CCL_PATH_SECOND} ]]
    then
    ssh -q ${LOGIN}@${SYSTEM} "${WORK_DIR}/ccl_performance.sh -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} ${SYSTEM_NAME}"
    else
    ssh -q ${LOGIN}@${SYSTEM} "${WORK_DIR}/ccl_performance.sh -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} ${SYSTEM_NAME} -c"
    fi
}

raw_convert() {
    echo "Converting logs to raw."
    echo "LOG_PATH: ${LOG_PATH}"
    echo "RAW_CONVERTER_PATH: ${RAW_CONVERTER_PATH}"
    python3 ${RAW_CONVERTER_PATH}/convert_to_raw.py -f ${LOG_PATH}/${LOG_FOLDER_NAME}
    CheckCommandExitCode $?  "  Converting logs to raw: ERROR"
}

# parse_parameters() {
#     echo "PARAM = $#"
#     while [ $# -gt 0 ]
#     do
#         key="$1"
#         case $key in
#             -l|--login)
#             echo "HI"
#             ${LOGIN}="$2"; shift; shift;
#             ;;
#             -s|--system)
#             export SYSTEM="$2"; shift; shift;
#             ;;
#             -w|--work_dir)
#             export WORK_DIR="$2"; shift; shift;
#             ;;                
#             -c|--comparison)
#             export CCL_PATH_FIRST="$2"; export CCL_PATH_SECOND="$3"; shift; shift; shift;
#             ;;
#             -h|--help)
#             help; shift;
#             ;;
#             -n)
#             export BENCH_CONF_N="$2"; shift; shift;
#             ;;
#             -ppn)
#             export BENCH_CONF_PPN="$2"; shift; shift;
#             ;;
#             --ccl_path)
#             export CCL_PATH="$2"; shift; shift;
#             ;;
#             --log_path)
#             export LOG_PATH="$2"; shift; shift;
#             ;;
#             --mpi_path)
#             export MPI_PATH="$2"; shift; shift;
#             ;;
#             --compiler_path)
#             export COMPILER_PATH="$2"; shift; shift;
#             ;;
#             -*|--*)
#             echo "  ERROR: Unsupported flag $key"; exit 1
#             ;;
#         esac
#     done
# }

set_default_values
#parse_parameters

echo "PARAM = $#"
    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            -l|--login)
            export LOGIN="$2"; shift; shift;
            ;;
            -s|--system)
            export SYSTEM="$2"; shift; shift;
            ;;
            -w|--work_dir)
            export WORK_DIR="$2"; shift; shift;
            ;;                
            -c|--comparison)
            export CCL_PATH_FIRST="$2"; export CCL_PATH_SECOND="$3"; shift; shift; shift;
            ;;
            -h|--help)
            help; shift;
            ;;
            -n)
            export BENCH_CONF_N="$2"; shift; shift;
            ;;
            -ppn)
            export BENCH_CONF_PPN="$2"; shift; shift;
            ;;
            --ccl_path)
            export CCL_PATH="$2"; shift; shift;
            ;;
            --log_path)
            export LOG_PATH="$2"; shift; shift;
            ;;
            --mpi_path)
            export MPI_PATH="$2"; shift; shift;
            ;;
            --compiler_path)
            export COMPILER_PATH="$2"; shift; shift;
            ;;
            -*|--*)
            echo "  ERROR: Unsupported flag $key"; exit 1
            ;;
            --raw_conv_path)
            export RAW_CONVERTER_PATH="$2"; shift; shift;
            ;;
        esac
    done

    if [[ ${SYSTEM} == "207.108.8.122" ]]
    then
        export SYSTEM_NAME="DG1"
        export WORK_DIR="/panfs/users/dsazanov/performance"
    else
        export SYSTEM_NAME="ATS"
        export WORK_DIR="/home/gta/dsazanov/performance"
    fi

info
checking_and_copying_components
copy_ccl
start_bench
get_logs
raw_convert