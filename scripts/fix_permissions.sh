#!/bin/bash

print_help()
{
    readonly ME=$(basename $0)
    echo "Usage: $ME DIR_PATH"
    echo "Usage examples:"
    echo "  $ME ./"
}

if [[ -z "$1" ]]; then
    echo "Incorrect options"
    print_help
    exit 1
fi

ROOT=$(realpath $1)

find ${ROOT}/ -type f \
    -not -path "${ROOT}/git/*" \
    \( \
        -name "*.hpp" -o \
        -name "*.cpp" -o \
        -name "*.h" -o \
        -name "*.c" -o \
        -name "*.txt" -o \
        -name "*.html" -o \
        -name "*.css" -o \
        -name "*.md" -o \
        -name "*.rst" \) \
    -exec chmod --preserve-root --changes -x {} \;
