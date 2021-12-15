#!/bin/bash

REPO=https://github.com/oneapi-src/level-zero.git

install_path_root=`pwd`

git clone $REPO

cp $install_path_root/level-zero/include/ze_api.h $install_path_root/include
cp $install_path_root/level-zero/include/zes_api.h $install_path_root/include

rm -rf $install_path_root/level-zero
