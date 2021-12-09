#!/bin/bash

install_path_root=`pwd`

ITT_DIR="`pwd`/../itt/"

# FIXME: right now we use an older version of pre-generated
# tracing_functions.so, if we generate a new one using the
# script, for some unknown reason it makes L0 call fail
# with error or make oneCCL hang when running with vtune
pushd ./gen
python ./generate_tracing_collector.py

gcc tracing_functions.c -D_GNU_SOURCE -fPIC -I$ITT_DIR/include \
        -L$ITT_DIR/lib64 -littnotify -lpthread -ldl --shared \
        -o tracing_functions.so

install ./tracing_functions.so $install_path_root/lib64/
