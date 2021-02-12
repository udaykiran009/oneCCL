#! /bin/bash 

BASENAME=`basename $0 .sh`

SCOPE="all"

LOG_FILE="clang_format.log"
rm ${LOG_FILE}

echo_log()
{
    echo -e "$*" 2>&1 | tee -a ${LOG_FILE}
}

print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh <options>"
    echo_log ""
    echo_log "<options>:"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -s <value>|--scope <value>"
    echo_log "        Set processing scope (value: all | commit)"
    echo_log "   ------------------------------------------------------------"
    echo_log "    -h|--help"
    echo_log "        Print help information"
    echo_log ""
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
            "-s"| "--scope")
                SCOPE=$2
                shift
                ;;
            *)
                echo_log "ERROR: unknown option ($1)"
                echo_log "See ./clang-format.sh -h"
                exit 1
                ;;
        esac

        shift
    done

    if [ "${SCOPE}" != "all" ] && [ "${SCOPE}" != "commit" ]
    then
        echo_log "ERROR: unknown scope: $SCOPE"
        print_help
        exit 1
    fi

    echo_log "----------------"
    echo_log "PARAMETERS"
    echo_log "----------------"
    echo_log "SCOPE = ${SCOPE}"
    echo_log "----------------"
}

process_files()
{
    if [[ $(git diff HEAD) ]]; then
        echo_log "Found changed files. Please, commit them or revert changes. Exit."
        echo_log "$(git status --porcelain)"
        status=3
    else
        echo_log "Format checking... started in $(git rev-parse --show-toplevel)"
        green_color='\033[0;32m'
        red_color='\033[0;31m'
        yellow_color='\033[0;33m'
        no_color='\033[0m'

        CLANG_TOOL="/p/pdsd/Intel_MPI/Software/clang-format/"
        export PATH=${CLANG_TOOL}:${PATH}

        echo_log "Clang-format version: ${green_color}$(clang-format --version)${no_color}"

        root_dir=`git rev-parse --show-toplevel`
        if [ "${SCOPE}" == "all" ]
        then
            files=`find ${root_dir} -type f`
        elif [ "${SCOPE}" == "commit" ]
        then
            commit_files=`git diff-tree --no-commit-id --name-only -r HEAD`
            for f in ${commit_files}
            do
                files="${files}\n${root_dir}/${f}"
            done
        fi

        for filename in $(echo -e "${files}" | \
            grep -v ".git/" | \
            grep -v "_install/" | \
            grep -v "build/" | \
            grep -v "cmake/"| \
            grep -v "CMakeFiles/" | \
            grep -v "doc/"| \
            grep -v "examples/build/" | \
            grep -v "googletest/" | \
            grep -v "mpi/include" | \
            grep -v "ofi/include"| \
            grep -v "scripts/copyright" | \
            grep -P ".*\.(c|cpp|h|hpp|cl|i)$")
        do
            echo_log "process file: ${filename}"
            clang-format -style=file -i $filename
        done

        echo_log $(git status) | grep "nothing to commit" > /dev/null

        if [[ $? -eq 1 ]]; then
            echo_log "Format checking... ${red_color}failed${no_color}"
            echo_log "$(git status --porcelain)"
            status=3
        else
            echo_log "Format checking... ${green_color}passed${no_color}"
            status=0
        fi
    fi
    exit ${status}
}

parse_arguments "$@"
process_files
