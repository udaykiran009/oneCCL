#!/bin/bash
set -o pipefail

# This script allows to compare external symbols of two libraries

readonly ME=$(basename $0)
readonly ARGS="$@"

readonly EXIT_FAILURE=-1
readonly EXIT_TEST_PASSED=0
readonly EXIT_TEST_FAILED=1

TEST_RESULT=$EXIT_TEST_PASSED

print_help()
{
    echo "Usage: $ME --new file [--old file | --expected-api file]"
    echo "  --new file              path to the new .so file"
    echo "  --old file              path to the old .so file"
    echo "  --expected-abi file     path to dump with expected ABI"
    echo "                          (can be used instead of --new option)"
    echo "  --export file           export symbol dump to specified file"
    echo
    echo "Usage examples:"
    echo "  1) Compare external symbols of two libraries:"
    echo "     $ME --new ./new_lib.so --old ./old_lib.so"
    echo
    echo "  2) Export symbol dump to specified file:"
    echo "     $ME --old ./lib.so --export ./expected.dump"
    echo
    echo "  3) Compare library external symbols with symbol dump file:"
    echo "     $ME --new ./lib.so --expected-abi ./expected.dump"
}

parse_arguments()
{
    if [[ $# -ne 4 ]]; then
        echo "Incorrect options"
        print_help
        exit $EXIT_FAILURE
    fi

    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")                  print_help ; exit 0 ;;
            "--new" | "-new")                           NEW_BIN_FILE=$2 ; shift ;;
            "--old" | "-old")                           OLD_BIN_FILE=$2 ; shift ;;
            "--expected-abi" | "-expected-abi")         EXPECTED_ABI_FILE=$2 ; shift ;;
            "--export" | "-export")                     EXPORT_ABI_FILE=$2 ; shift ;;
            *)
            echo "$ME: ERROR: unknown option ($1)"
            print_help
            exit $EXIT_FAILURE
            ;;
        esac
        shift
    done

    if [[ ! -z "$NEW_BIN_FILE" ]] && [[ ! -z "$EXPORT_ABI_FILE" ]]; then
        echo "Incorrect options"
        print_help
        exit $EXIT_FAILURE
    fi

    if [[ ! -z "$OLD_BIN_FILE" ]] && [[ ! -z "$EXPECTED_ABI_FILE" ]]; then
        echo "Incorrect options"
        print_help
        exit $EXIT_FAILURE
    fi

    echo "Parameters:"
    [[ ! -z "$NEW_BIN_FILE" ]] && echo "NEW_FILE: $NEW_BIN_FILE"
    [[ ! -z "$OLD_BIN_FILE" ]] && echo "OLD_FILE: $OLD_BIN_FILE"
    [[ ! -z "$EXPECTED_ABI_FILE" ]] && echo "EXPECTED_ABI_FILE: $EXPECTED_ABI_FILE"
    [[ ! -z "$EXPORT_ABI_FILE" ]] && echo "EXPORT_ABI_FILE: $EXPORT_ABI_FILE"
    echo
}

compare_symbols()
{
    local old_abi_file=$1
    local new_abi_file=$2
    
    if [[ ! -f $old_abi_file ]] || [[ ! -f $new_abi_file ]]; then
        echo "$ME: export_symbols: file not found!"
        exit $EXIT_FAILURE
    fi

    local abi_changes="$(diff -u $old_abi_file $new_abi_file | diffstat | grep deletion)"
    if [[ ! -z "$abi_changes" ]]; then
        TEST_RESULT=$EXIT_TEST_FAILED
        echo
        (diff -y --suppress-common-lines $old_abi_file $new_abi_file)
        echo
        echo "$abi_changes"
        echo
        echo -e '\033[1mABI does not match!!!\033[0m'
    else
        TEST_RESULT=$EXIT_TEST_PASSED
        echo "Diff stat: $(diff -u $old_abi_file $new_abi_file | diffstat | tail -n1)"
    fi
}

export_symbols()
{
    local in_file=$1
    local out_file=$2

    if [[ -z "$in_file" ]] || [[ ! -f $in_file ]]; then
        echo "$ME: export_symbols: file not found!"
        exit $EXIT_FAILURE
    fi

    (nm --dynamic --defined-only --extern-only $in_file | grep 3ccl | cut -d " " -f 3- > $out_file)
}

run()
{
    local new_tmp_file=""
    local old_tmp_file=""

    if [[ ! -z "$NEW_BIN_FILE" ]]; then
        echo "Export symbols from $NEW_BIN_FILE..."
        new_tmp_file=$(mktemp) # temp file
        export_symbols $NEW_BIN_FILE $new_tmp_file
    fi

    if [[ ! -z "$OLD_BIN_FILE" ]] && [[ -z "$EXPORT_ABI_FILE" ]]; then
        echo "Export symbols from $OLD_BIN_FILE..."
        old_tmp_file=$(mktemp) # temp file
        export_symbols $OLD_BIN_FILE $old_tmp_file
    fi

    if [[ ! -z "$EXPORT_ABI_FILE" ]]; then
        echo "Export symbols from $OLD_BIN_FILE..."
        export_symbols $OLD_BIN_FILE $EXPORT_ABI_FILE
    fi

    if [[ ! -z "$new_tmp_file" ]] && [[ ! -z "$old_tmp_file" ]]; then
        echo "Comparison..."
        compare_symbols $old_tmp_file $new_tmp_file
    elif [[ ! -z "$new_tmp_file" ]] && [[ ! -z "$EXPECTED_ABI_FILE" ]]; then
        echo "Comparison..."
        compare_symbols $EXPECTED_ABI_FILE $new_tmp_file
    fi

    # Deleting temporary files
    [[ -f $new_tmp_file ]] && (rm $new_tmp_file)
    [[ -f $old_tmp_file ]] && (rm $old_tmp_file)

    if [[ $TEST_RESULT != $EXIT_TEST_PASSED ]]; then
        echo "FAILED"
        exit $EXIT_TEST_FAILED
    else
        echo "OK"
        exit $EXIT_TEST_PASSED
    fi
}

parse_arguments $ARGS
run
