#!/bin/bash

BASENAME=`basename $0 .sh`
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

LEVEL_ZERO_PACKAGES=""
NEO_PACKAGES=""

INTEL_GRAPHICS_COMPILER_URL="https://api.github.com/repos/intel/intel-graphics-compiler/releases/latest"
COMPUTE_RUNTIME_URL="https://api.github.com/repos/intel/compute-runtime/releases/latest"
LEVEL_ZERO_URL="https://api.github.com/repos/oneapi-src/level-zero/releases/latest"

print_help()
{
    echo "Usage:"
    echo "    ./${BASENAME}.sh <options>"
    echo ""
    echo "<options>:"
    echo "   ------------------------------------------------------------"
    echo "    -update-runtimes|--update-runtimes"
    echo "        Download Level-Zero, NEO runtimes and install it"
    echo "    -download-level-zero|--download-level-zero"
    echo "        Download Level-Zero runtimes"
    echo "    -download-neo|--download-neo"
    echo "        Download NEO runtimes"
    echo "    -install|--install"
    echo "        Install runtimes"
    echo "    -f|--runtime-dir"
    echo "        The path to the directory where the runtime packages will be uploaded"
    echo "    -run-examples|--run-examples"
    echo "        Run examples to check the correct installation of runtimes"
    echo "    -p|-password"
    echo "        Path to file with password"
    echo "    -ccl-src|--ccl-src"
    echo "        Path to dir with oneCCL source files"
    echo "   ------------------------------------------------------------"
    echo "    -h|-H|-help|--help"
    echo "        Print help information"
    echo ""
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "--update-runtimes"|"-update-runtimes")
                DOWNLOAD_L0="yes"
                DOWNLOAD_NEO="yes"
                ENABLE_INSTALL="yes"
                ;;
            "--download-level-zero"|"-download-level-zero")
                DOWNLOAD_L0="yes"
                ;;
            "--download-neo"|"-download-neo")
                DOWNLOAD_NEO="yes"
                ;;
            "--install"|"-install")
                ENABLE_INSTALL="yes"
                ;;
            "--run-examples"|"-run-examples")
                RUN_EXAMPLES="yes"
                ;;
            "--ccl-src"|"-ccl-src")
                CCL_SRC_DIR="${2}"
                shift
                ;;
            "-help"|"--help"|"-h"|"-H")
                print_help
                exit 0
                ;;
            "-f"|"--runtime-dir")
                RUNTIMES_DIR="${2}"
                shift
                ;;
            "-p"|"--password")
                PASSWORD="${2}"
                shift
                ;;
            *)
                echo "ERROR: unknown option ($1)"
                echo "See ./update_runtimes.sh -help"
                exit 1
                ;;
        esac

        shift
    done

    if [[ -z ${RUNTIMES_DIR} ]]; then
        RUNTIMES_DIR="/tmp/neo"
    fi

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "DOWNLOAD_L0          = ${DOWNLOAD_L0}"
    echo "DOWNLOAD_NEO         = ${DOWNLOAD_NEO}"
    echo "ENABLE_INSTALL       = ${ENABLE_INSTALL}"
    echo "RUNTIMES_DIR         = ${RUNTIMES_DIR}"
    echo "CCL_SRC_DIR          = ${CCL_SRC_DIR}"
    echo "RUN_EXAMPLES         = ${RUN_EXAMPLES}"
    echo "-----------------------------------------------------------"
}

print_header() {
    echo "###"
    echo "### ${1}"
    echo "###"
}

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

load_level_zero() {
    if [[ ! -z ${DOWNLOAD_L0} ]]; then
        print_header "LOAD LEVEL ZERO..."
        mkdir -p ${RUNTIMES_DIR}
        cd ${RUNTIMES_DIR}
        for package in $(curl -s ${LEVEL_ZERO_URL} | grep "browser_download_url" | awk '{print $2}' | sed -e 's/^"//' -e 's/"$//')
        do
            echo "wget ${package}"
            wget ${package}
            LEVEL_ZERO_PACKAGES="${LEVEL_ZERO_PACKAGES} $(basename ${package})"
        done
        check_version "${LEVEL_ZERO_PACKAGES}"
        if [[ $? -eq 0 ]]; then
            echo "All versions are up to date"
        fi
    fi
}

check_sha_sum() {
    print_header "CHECK SHA SUM..."
    sum=$(curl -s ${COMPUTE_RUNTIME_URL} | grep "browser_download_url" | awk '{print $2}' | grep "sum" | sed -e 's/^"//' -e 's/"$//')
    cd ${RUNTIMES_DIR}
    wget ${sum}
    sha256sum -c $(basename ${sum})
    rm $(basename ${sum})
}

load_neo() {
    if [[ ! -z ${DOWNLOAD_NEO} ]]; then
        print_header "LOAD NEO..."
        mkdir -p ${RUNTIMES_DIR}
        cd ${RUNTIMES_DIR}
        for package in $(curl -s ${INTEL_GRAPHICS_COMPILER_URL} | grep "browser_download_url" | grep "core\|opencl_" | awk '{print $2}' | sed -e 's/^"//' -e 's/"$//')
        do
            wget ${package}
            NEO_PACKAGES="${NEO_PACKAGES} $(basename ${package})"
        done
        for package in $(curl -s ${COMPUTE_RUNTIME_URL} | grep "browser_download_url" | awk '{print $2}' | grep "deb" | sed -e 's/^"//' -e 's/"$//')
        do
            wget ${package}
            NEO_PACKAGES="${NEO_PACKAGES} $(basename ${package})"
        done
        check_sha_sum
        check_version "${NEO_PACKAGES}"
        if [[ $? -eq 0 ]]; then
            echo "All versions are up to date"
        fi
    fi
}

install() {
    if [[ ! -z ${ENABLE_INSTALL} ]]; then
        print_header "INSTALL..."
        cd ${RUNTIMES_DIR}
        for package in $(ls ${RUNTIMES_DIR})
        do
            /usr/bin/sudo -S dpkg -i ${package} < ${PASSWORD}
            CheckCommandExitCode $? "Install ${package} failed"
        done
    fi
}

check_version() {
    print_header "CHECK VERSION..."
    rc=0
    for package in ${1}
    do  
        pkg_name=$(echo ${package} | cut -d'_' -f1)
        current_version=$(dpkg -l ${pkg_name} | grep ${pkg_name} | awk '{print $3}')
        new_version=$(echo ${package} | cut -d'_' -f2 | cut -d'%' -f1)
        if [[ ${current_version} == ${new_version} ]]
        then
            echo "The ${package} package has the current version."
        else
            echo "The ${package} version outdated (current version: ${current_version} | new_version: ${new_version})."
            rc=$((rc+1))
        fi
    done
    return ${rc}
}

run_examples() {
    if [[ ! -z ${RUN_EXAMPLES} ]] && [[ ! -z ${CCL_SRC_DIR} ]]; then
        print_header "RUN EXAMPLES..."
        if [[ ! -d ${CCL_SRC_DIR} ]]; then
            echo "${CCL_SRC_DIR} does not exists."
            exit 1
        fi
        IMPI_PATH="/p/pdsd/scratch/Uploads/CCL_oneAPI/mpi_oneapi/last/mpi/latest/"
        CCL_PATH="/p/pdsd/scratch/jenkins/artefacts/ccl-nightly/last/"
        SYCL_BUNDLE_ROOT="/p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/linux/"

        HOSTNAME=`hostname -s`
        export I_MPI_HYDRA_HOST_FILE=${CCL_SRC_DIR}/tests/cfgs/clusters/${HOSTNAME}/mpi.hosts

        source ${SYCL_BUNDLE_ROOT}/../../../setvars.sh
        source ${IMPI_PATH}/env/vars.sh -i_mpi_library_kind=release_mt
        source ${CCL_PATH}/l_ccl_release*/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp

        ${CCL_SRC_DIR}/examples/run.sh gpu
        CheckCommandExitCode $? "Examples failed"
    fi
}

parse_arguments "$@"

if [[ -d ${RUNTIMES_DIR} ]]; then
    echo "Path ${RUNTIMES_DIR} already exists. Do you want to overwrite the data?"
    echo "Type 'y' to continue or 'q' to exit and then press Enter"
    CONFIRMED=0
    while [ ${CONFIRMED} = 0 ]
    do
        read -e ANSWER
        case $ANSWER in
            'q')
                exit 0
                ;;
            'y')
                CONFIRMED=1
                rm -rf ${RUNTIMES_DIR}
                ;;
            *)
                ;;
        esac
    done
fi

load_level_zero
load_neo
install
run_examples
