#!/bin/bash

ofi_rt_path=`pwd`
ofi_tag="v1.7.1rc2"

git clone https://github.com/ofiwg/libfabric.git
cd ./libfabric
ofi_install_path=`pwd`/_install
git checkout $ofi_tag
./autogen.sh
./configure --prefix=$ofi_install_path --enable-rxm=dl --enable-verbs=dl --enable-tcp=dl --enable-sockets=dl --enable-psm2=dl \
--disable-psm --disable-usnic --disable-mlx --disable-gni --disable-xpmem --disable-udp --disable-rxd --disable-bgq \
--disable-shm --disable-mrail --disable-rstream --disable-static

make -j all && make install

cp -f $ofi_install_path/bin/fi_info $ofi_rt_path/bin
cp -rf $ofi_install_path/include/rdma $ofi_rt_path/include
cp -f -T $ofi_install_path/lib/libfabric.so.1.* $ofi_rt_path/lib/libfabric.so.1
cp -f $ofi_install_path/lib/libfabric/libpsmx2-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/librxm-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libsockets-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libtcp-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libverbs-fi.so $ofi_rt_path/lib/prov
