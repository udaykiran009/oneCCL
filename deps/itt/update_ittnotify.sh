#!/bin/bash

# last built on nnlmpisnb13
# it has old binutils 2.25 which prevents
# any incompatibility issues

REPO=https://github.com/intel/ittapi.git

install_path_root=`pwd`

git clone $REPO ittapi
pushd ./ittapi
python ./buildall.py --force_bits 64

install ./build_linux2/64/bin/libittnotify.a $install_path_root/lib64/
install ./include/ittnotify.h $install_path_root/include
