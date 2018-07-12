#!/bin/bash

cat Packages | gzip -9c > Packages.gz

#create the Release file
PKGS=$(wc -c Packages)
PKGS_GZ=$(wc -c Packages.gz)
cat > Release << EOF
Architectures: amd64
Label: Intel(R) Machine Learning Scaling Library MLSL_SUBSTITUTE_OFFICIAL_VERSION for Linux* OS
Date: $(date -R -u)
MD5Sum:
 $(md5sum Packages  | cut -d" " -f1) $PKGS
 $(md5sum Packages.gz  | cut -d" " -f1) $PKGS_GZ
SHA1:
 $(sha1sum Packages  | cut -d" " -f1) $PKGS
 $(sha1sum Packages.gz  | cut -d" " -f1) $PKGS_GZ
SHA256:
 $(sha256sum Packages | cut -d" " -f1) $PKGS
 $(sha256sum Packages.gz | cut -d" " -f1) $PKGS_GZ
EOF
