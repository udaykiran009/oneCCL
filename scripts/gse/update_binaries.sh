#!/bin/bash

BASENAME=`basename $0 .sh`
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
CURR_DATE=`date "+%Y%m%d%H%M%S"`
LOG_FILE="${SCRIPT_DIR}/log_${CURR_DATE}.txt"
GTA_BINARIES="/nfs/inn/proj/mpi/pdsd/opt/tools/gse/"
PATH_TO_ABN="${GTA_BINARIES}/last"
ROOT_URL="https://gfx-assets.igk.intel.com/artifactory"
ASSETS="gfx-ocl-abn-assets-igk/oneAPI/linux"
PACKAGE_DIR="${SCRIPT_DIR}/_gse_package"
touch ${LOG_FILE}


echo_log() {
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

CheckCommandExitCode() {
    if [ $1 -ne 0 ]
    then
        echo_log "ERROR: ${2}" 1>&2
        exit $1
    fi
}

print_help()
{
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  --ccl <path>"
    echo_log "      Path to oneCCL"
    echo_log "  --mpi <path>"
    echo_log "      Path to IMPI"
    echo_log "  --compiler <path>"
    echo_log "      Path to Intel DPC++/C++ Compiler"
    echo_log "  --create_package"
    echo_log "      Enable creating package"
    echo_log "  --test"
    echo_log "      Enable testing of the final package using ABN Dynamic Tests"
    echo_log "  --package <path>"
    echo_log "      Path to package"
    echo_log "  --push"
    echo_log "      Enable push binaries"
    echo_log "  --username <IDSID>"
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh --create_package --ccl /p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/ "
    echo_log "                  --mpi /p/pdsd/scratch/jenkins/artefacts/impi-ch4-nightly/last/oneapi_impi/ "
    echo_log "                  --compiler /p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/"
    echo_log "  ${BASENAME}.sh --test --package /p/pdsd/Users/asoluyan/GitHub/mlsl2/scripts/gse/_gse_package"
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")                  
                print_help
                exit 0
                ;;
            "--ccl" | "-ccl")                         
                CCL_ARTEFACTS=$2
                shift
                ;;
            "--mpi" | "-mpi")                         
                PATH_TO_IMPI=$2
                shift
                ;;
            "--compiler" | "-compiler")                         
                PATH_TO_COMPILER=$2
                shift
                ;;
            "--create_package" | "-c")                         
                ENABLE_CREATE_PACKAGE="1"
                ;;
            "--test" | "-t")                         
                ENABLE_TESTING="1"
                ;;
            "--package" | "-p")                         
                PATH_TO_PACKAGE=$2
                shift
                ;;
            "--push" | "-push")                         
                ENABLE_PUSH="1"
                ;;
            "--username" | "-u")                         
                USERNAME=$2
                shift
                ;;
            *)
                echo_log "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
                ;;
        esac
        shift
    done

    if [[ ! -z ${ENABLE_CREATE_PACKAGE} ]]; then
        if [[ -z ${CCL_ARTEFACTS} ]]; then 
            echo_log "ERROR: missing path to CCL artefacts"
            print_help
            exit 1
        fi
        if [[ -z ${PATH_TO_IMPI} ]]; then 
            echo_log "ERROR: missing path to IMPI"
            print_help
            exit 1
        fi 
        if [[ -z ${PATH_TO_COMPILER} ]]; then 
            echo_log "ERROR: missing path to compiler"
            print_help
            exit 1
        fi 
    fi

    if [[ ! -z ${ENABLE_TESTING} || ! -z ${ENABLE_PUSH} ]]; then
        if [[ -z ${PATH_TO_PACKAGE} || ! -d ${PATH_TO_PACKAGE} ]]; then
            echo_log "ERROR: incorrect path to package"
            print_help
            exit 1
        fi
    fi

    if [[ ! -z ${ENABLE_PUSH} ]]; then
        if [[ -z ${USERNAME} ]]; then
            echo_log "ERROR: missing username"
            print_help
            exit 1
        fi
    fi


    echo_log "-----------------------------------------------------------"
    echo_log "PARAMETERS"
    echo_log "-----------------------------------------------------------"

    echo_log "CCL_ARTEFACTS         = ${CCL_ARTEFACTS}"
    echo_log "PATH_TO_IMPI          = ${PATH_TO_IMPI}"
    echo_log "PATH_TO_COMPILER      = ${PATH_TO_COMPILER}"
    echo_log "PATH_TO_PACKAGE       = ${PATH_TO_PACKAGE}"

    echo_log "ENABLE_CREATE_PACKAGE = ${ENABLE_CREATE_PACKAGE}"
    echo_log "ENABLE_TESTING        = ${ENABLE_TESTING}"
    echo_log "ENABLE_PUSH           = ${ENABLE_PUSH}"
    echo_log "USERNAME              = ${USERNAME}"
}

prepare_ccl() 
{
    echo_log "### Preparing oneCCL..."
    mkdir -p ${PACKAGE_DIR}/ccl/lib
    pushd ${PACKAGE_DIR}/ccl/lib
    cp -R ${CCL_ARTEFACTS}/l_ccl_release_*/lib/cpu* .
    cp -R ${CCL_ARTEFACTS}/l_ccl_release_*/lib/kernels .
    cp ${CCL_ARTEFACTS}/build_gpu_release.tgz .
    mkdir ./build_gpu_release && tar xfz build_gpu_release.tgz -C ./build_gpu_release
    cp ./build_gpu_release/build_gpu/examples/benchmark/benchmark ${PACKAGE_DIR}
    rm -rf ./build_gpu_release*
    popd
    echo_log "### Preparing oneCCL...DONE"
}

prepare_mpi() 
{
    echo_log "### Preparing IMPI..."
    mkdir ${PACKAGE_DIR}/mpi_lib
    pushd ${PACKAGE_DIR}/mpi_lib
    cp -R ${PATH_TO_IMPI}/lib/release_mt/ .
    cp -R ${PATH_TO_IMPI}/libfabric/ .
    cp -R ${PATH_TO_IMPI}/bin/ .
    popd
    echo_log "### Preparing IMPI...DONE"
}

prepare_compiler() 
{
    echo_log "### Preparing compiler..."
    mkdir ${PACKAGE_DIR}/compiler_lib
    pushd ${PACKAGE_DIR}/compiler_lib
    cp -R ${PATH_TO_COMPILER}/linux/lib/clang .
    cp -R ${PATH_TO_COMPILER}/linux/lib/x64 .
    cp -R ${PATH_TO_COMPILER}/linux/compiler/lib/intel64_lin .
    cp ${PATH_TO_COMPILER}/linux/lib/* .
    popd
    echo_log "### Preparing compiler...DONE"
}

create_package() 
{
    if [[ ${ENABLE_CREATE_PACKAGE} = "1" ]]; then
        echo_log "### Creating zip..."
        mkdir ${PACKAGE_DIR}
        pushd ${PACKAGE_DIR}
        prepare_ccl
        prepare_mpi
        prepare_compiler
        
        cp ${SCRIPT_DIR}/benchmark.sh .
        cp ${SCRIPT_DIR}/abn_metadata.json .

        popd
        echo_log "### Creating zip...DONE"
    fi
}

run_tests() 
{
    if [[ ${ENABLE_TESTING} = "1" ]]; then
        echo_log "### Run tests..."
        pushd ${PACKAGE_DIR}

        cp ${SCRIPT_DIR}/abn-linux-settings.json .
        sed -i "s|%PACKAGE_DIR%|${PACKAGE_DIR}|" ${PACKAGE_DIR}/abn-linux-settings.json
        for bench in $(grep -E -o "oneCCLBench([^\"]+)" abn_metadata.json);
        do
            python3 ${PATH_TO_ABN}/src/abn_cmd.py -t "DynamicTest" -s ${bench} --settings-file ${PACKAGE_DIR}/abn-linux-settings.json --working_path ${SCRIPT_DIR}/tmp
            CheckCommandExitCode $? "${bench} failed"
        done
        rm -rf ${PACKAGE_DIR}/results/
        rm ${PACKAGE_DIR}/abn-linux-settings.json   

        popd
        echo_log "### Run tests...DONE"
    fi
}

push()
{
    if [[ ${ENABLE_PUSH} = "1" ]]
    then
        ORIG_HOME=${HOME}
        export HOME=.
        ${GTA_BINARIES}/gta-asset push ${ASSETS} oneCCLBench $(date "+%Y%m%d") ${PATH_TO_PACKAGE} --user ${USERNAME} --root-url=${ROOT_URL}
        CheckCommandExitCode $? "push failed"
        export HOME=${ORIG_HOME}
    fi
}

parse_arguments "$@"
create_package
run_tests
push
