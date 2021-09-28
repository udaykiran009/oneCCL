#!/bin/bash

INTEL_GRAPHICS_COMPILER_URL="https://api.github.com/repos/intel/intel-graphics-compiler/releases/latest"
COMPUTE_RUNTIME_URL="https://api.github.com/repos/intel/compute-runtime/releases/latest"
LEVEL_ZERO_URL="https://api.github.com/repos/oneapi-src/level-zero/releases/latest"

print_help()
{
    echo "Usage:"
    echo "    ./update-runtimes.sh <options>"
    echo ""
    echo "<options>:"
    echo "   ------------------------------------------------------------"
    echo "    -u|--unpack"
    echo "        Unpacking .deb packages"
    echo "    -unpack-dir|--unpack-dir"
    echo "        The path to the directory where the runtime packages will be unpacked"
    echo "    -check-version|--check-version"
    echo "        Comparison of the latest available version and the version of the system"
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
            "-help"|"--help"|"-h"|"-H")
                print_help
                exit 0
                ;;
            "-u"|"--unpack")
                UNPACK="yes"
                ;;
            "-unpack-dir"|"--unpack-dir")
                UNPACK_DIR="${2}"
                shift
                ;;
            "-check-version"|"--check-version")
                CHECK_VERSION="yes"
                ;;
            *)
                echo "ERROR: unknown option ($1)"
                echo "See ./update_runtimes.sh -help"
                exit 1
                ;;
        esac

        shift
    done

    if [[ -z ${UNPACK_DIR} && ! -z ${UNPACK} ]]; then
        UNPACK_DIR="/p/pdsd/scratch/Uploads/runtimes"
    fi

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "UNPACK               = ${UNPACK}"
    echo "UNPACK_DIR           = ${UNPACK_DIR}"
    echo "CHECK_VERSION        = ${CHECK_VERSION}"
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

unpack_packages() {
    if [[ ! -z ${UNPACK} ]]; then
        print_header "UNPACK PACKAGES..."
        TAG_LEVEL_ZERO=$(curl ${LEVEL_ZERO_URL} | grep tag_name | awk '{print $2}' | sed -e 's/"//g' -e 's/,//g')
        TAG_COMPUTE_RUNTIME=$(curl ${COMPUTE_RUNTIME_URL} | grep tag_name | awk '{print $2}' | sed -e 's/"//g' -e 's/,//g')

        print_header "UNPACK LEVEL ZERO (${TAG_LEVEL_ZERO})"
        if [ -e ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO} ]; then
            echo "This version is already unpacked into ${UNPACK_DIR}/level-zero"
        else
            mkdir -p ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO}/{packages,unpack}

            cd ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO}/packages
            for package in $(curl -s ${LEVEL_ZERO_URL} | grep "browser_download_url" | awk '{print $2}' | sed -e 's/^"//' -e 's/"$//')  
            do
                echo "wget ${package}"
                wget ${package}
            done

            for package in $(ls ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO}/packages)
            do
                dpkg -x ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO}/packages/${package} ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO}/unpack
            done

            rm -rf ${UNPACK_DIR}/level-zero/last
            ln -s ${UNPACK_DIR}/level-zero/${TAG_LEVEL_ZERO} ${UNPACK_DIR}/level-zero/last
        fi

        print_header "UNPACK COMPUTE RUNTIME (${TAG_COMPUTE_RUNTIME})"
        if [ -e ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME} ]; then
            echo "This version is already unpacked into ${UNPACK_DIR}/compute-runtime"
        else
            mkdir -p ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME}/{packages,unpack}
            cd ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME}/packages
            for package in $(curl -s ${COMPUTE_RUNTIME_URL} | grep "browser_download_url" | awk '{print $2}' | grep "deb" | sed -e 's/^"//' -e 's/"$//')
            do
                echo "wget ${package}"
                wget ${package}
            done
            for package in $(curl -s ${INTEL_GRAPHICS_COMPILER_URL} | grep "browser_download_url" | grep "core\|opencl_" | awk '{print $2}' | sed -e 's/^"//' -e 's/"$//')
            do
                echo "wget ${package}"
                wget ${package}
            done

            for package in $(ls ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME}/packages)
            do
                dpkg -x ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME}/packages/${package} ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME}/unpack
            done

            rm -rf ${UNPACK_DIR}/compute-runtime/last
            ln -s ${UNPACK_DIR}/compute-runtime/${TAG_COMPUTE_RUNTIME} ${UNPACK_DIR}/compute-runtime/last
        fi
    fi
}

check_versions() {
    if [[ ! -z ${CHECK_VERSION} ]]; then
        print_header "CHECK VERSIONS..."
        for link in ${LEVEL_ZERO_URL} ${COMPUTE_RUNTIME_URL} ${INTEL_GRAPHICS_COMPILER_URL}
        do
            packages=`curl ${link} | awk '/name/ && /.deb/ && !/body/ && gsub(/"/,"") && gsub(/_/," ") && !/intel-igc-media/ && !/intel-igc-opencl-devel/ {print $2, $3}'`
            latest_packages_list+="${packages}\n"
        done
        echo -e "\nLATEST_PACKAGES_LIST:\n${latest_packages_list}"
        for package_name in `echo -e "${latest_packages_list}" | awk '!/intel-igc-media/ && !/intel-igc-opencl-devel/ {print $1}'`
        do
            system_packages_list+="`dpkg -l ${package_name} | grep ${package_name} | awk '{print $2,$3}'`\n"
        done
        echo -e "\nSYSTEM_PACKAGES_LIST:\n${system_packages_list}"

        if [[ `echo -e "${system_packages_list}"` == `echo -e "${latest_packages_list}"` ]]; then
            echo "All versions are up-to-date"
        else
            echo "Some packages have an outdated version"
        fi
    fi
}

parse_arguments "$@"
check_versions
unpack_packages
