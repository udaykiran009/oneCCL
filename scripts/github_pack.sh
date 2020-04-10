#!/bin/bash -x

#git clone git@github.intel.com:ict/mlsl2.git

dstdir="_package"
rm -rf $dstdir
rm -rf $dstdir.tgz
excluded=".git
.gitignore
./oneccl_license.txt
./boms
./scripts
./tests/cfgs
./doc/README.md
./doc/README.txt
./doc/cclEULA.txt
./doc/archive
./doc/third_party_programs
./doc/copyright
./doc/spec
./examples/run.sh
./examples/Makefile
./ofi/ofi_update.sh
./ccl_public
./ccl_oneapi"

count=0
mkdir -p $dstdir
cd ./../mlsl2

#preparing files
./scripts/preparing_files.sh ccl_public

ls -al

for srcfile in `find .`
do
    for exfile in $excluded
    do
    if [ `echo "${srcfile}" | grep  "${exfile}"` ]
    then
        count=$((++count))
        echo $count
    fi
    done
    if [ $count -eq 0  ]
    then
        dstfile=$srcfile
        echo $dstfile
        echo "new" $dstfile
        cp --parents $srcfile $dstdir
    fi
    count=0
done
cd $dstdir
for CUR_FILE in `find .  -type f | grep -e "\.hpp$" -e "\.cpp$" -e "\.h$" -e "\.c$" | grep -v "googletest"`; do
	ed $CUR_FILE < ../scripts/copyright/copyright.c ; \
done
for CUR_FILE in `find .  -type f | grep  -e "\.sh$" -e "\.sh.in$" | grep -v "googletest"` ; do
	ed $CUR_FILE < ../scripts/copyright/copyright.sh > /dev/null 2>&1; \
done
for CUR_FILE in `find .  -type f | grep -e "\.py$"| grep -v "doc" | grep -v "googletest"` ; do
	ed $CUR_FILE < ../scripts/copyright/copyright.m > /dev/null 2>&1; \
done
for CUR_FILE in `find .  -type f | grep -e "Makefile$" -e "CMakeLists.txt"| grep -v "doc" | grep -v "googletest"` ; do
	ed $CUR_FILE < ../scripts/copyright/copyright.m > /dev/null 2>&1; \
done
# tar -czf ../$dstdir.tgz .
