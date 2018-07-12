Name: RPM_NAME
Version: VERSION_SHORT
Release: PACKAGE_ID
Summary: Intel(R) Machine Learning Scaling Library MLSL_SUBSTITUTE_OFFICIAL_VERSION for Linux* OS
Group: Development/Languages
License: Intel Copyright 1999-MLSL_SUBSTITUTE_COPYRIGHT_YEAR
AutoReqProv: no
Prefix: INSTALL_DIR
BuildArch: ARCH
BuildRoot: BUILD_ROOT
Vendor: Intel Corporation

%description
Intel(R) Machine Learning Scaling Library MLSL_SUBSTITUTE_OFFICIAL_VERSION for Linux* OS

%files
    %defattr(755,root,root,755)
    %attr(644,root,root) INSTALL_FULLDIR/doc/*
    %attr(644,root,root) INSTALL_FULLDIR/intel64/etc/*
    %attr(644,root,root) INSTALL_FULLDIR/intel64/include/*
    %attr(644,root,root) INSTALL_FULLDIR/licensing/mlsl/*
    %attr(644,root,root) INSTALL_FULLDIR/licensing/mpi/*
    %attr(644,root,root) INSTALL_FULLDIR/test/*
        INSTALL_FULLDIR

%pre
    if [ "$RPM_INSTALL_PREFIX" = INSTALL_DIR ] && [ ! -d INSTALL_DIR ]
    then
        mkdir -p INSTALL_DIR
        chmod 755 INSTALL_DIR
    fi

%post
    sed -i -e "s|MLSL_SUBSTITUTE_INSTALLDIR|$RPM_INSTALL_PREFIX/INSTALL_SUBDIR|g" $RPM_INSTALL_PREFIX/INSTALL_SUBDIR/intel64/bin/mlslvars.sh
    sed -i -e "s|I_MPI_SUBSTITUTE_INSTALLDIR|$RPM_INSTALL_PREFIX/INSTALL_SUBDIR|g" $RPM_INSTALL_PREFIX/INSTALL_SUBDIR/intel64/etc/mpiexec.conf
