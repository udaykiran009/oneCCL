#!/bin/bash

# last built on nnlmpisnb13
# it has old binutils 2.25 which prevents
# any incompatibility issues

repo=https://github.com/intel/ittapi.git
tag="v3.23.0"

install_path_root=`pwd`

git clone --depth 1 --branch ${tag} ${repo} ittapi
pushd ./ittapi
python ./buildall.py --force_bits 64

install ./build_linux2/64/bin/libittnotify.a $install_path_root/lib64/
install ./include/ittnotify.h $install_path_root/include
