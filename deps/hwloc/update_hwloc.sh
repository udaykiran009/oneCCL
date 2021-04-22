#!/bin/bash

runtime_path=`pwd`

export CFLAGS="-fPIC"
export CXXFLAGS="-fPIC"

git clone https://github.com/open-mpi/hwloc.git
cd ./hwloc
install_path=`pwd`/_install

./autogen.sh
./configure --prefix=$install_path \
    --disable-netloc \
    --disable-opencl \
    --disable-cuda \
    --disable-nvml \
    --disable-rsmi \
    --disable-levelzero \
    --disable-gl \
    --disable-libxml2 \
    --enable-static
make -j all && make install

cp -rf $install_path/include $runtime_path
cp -f $install_path/lib/libhwloc.a $runtime_path/lib
