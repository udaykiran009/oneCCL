#! /bin/bash 

if [[ $(git diff HEAD) ]]; then
    echo "Found changed files. Please, commit them or revert changes. Exit."
    echo "$(git status --porcelain)"
    status=3
else
    echo -e "Format checking... started in $(git rev-parse --show-toplevel)"
    green_color='\033[0;32m'
    red_color='\033[0;31m'
    yellow_color='\033[0;33m'
    no_color='\033[0m'

    CLANG_TOOL="/p/pdsd/Intel_MPI/Software/clang-format/"
    export PATH=${CLANG_TOOL}:${PATH}

    echo -e "Clang-format version: ${green_color}$(clang-format --version)${no_color}"

    for filename in $(find $(git rev-parse --show-toplevel) -type f | \
        grep -v "googletest" | \
        grep -v "doc"| \
        grep -v "ofi/include"| \
        grep -v "mpi/include" | \
        grep -v "scripts/copyright" | \
        grep -v "examples/build" | \
        grep -v "CMakeFiles" | \
        grep -v "_install" | \
        grep -P ".*\.(c|cpp|h|hpp|cl|i)$")
    do
        echo "process file ${filename}"
        clang-format -style=file -i $filename
    done

    echo $(git status) | grep "nothing to commit" > /dev/null

    if [[ $? -eq 1 ]]; then
        echo -e "Format checking... ${red_color}failed${no_color}"
        echo "$(git status --porcelain)"
        status=3
    else
        echo -e "Format checking... ${green_color}passed${no_color}"
        status=0
    fi
fi
exit ${status}
