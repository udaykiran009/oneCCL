-------------------------------------------------------
Intel(R) oneAPI Collective Communications Library for Linux* OS
README
-------------------------------------------------------

----------------------
License Acceptance
----------------------

Review the file cclEULA.txt. To accept the End User License Agreement,
unzip the included file package.zip to a temporary directory with
the following command:
 
> unzip  package.zip

and type "accept" when prompted.

------------
Introduction
------------

Intel(R) oneAPI Collective Communications Library (Intel(R) CCL) is a library providing
an efficient implementation of communication patterns used in deep learning.

    - Built on top of MPI, allows for use of other communication libraries
    - Optimized to drive scalability of communication patterns
    - Works across various interconnects: Intel(R) Omni-Path Architecture,
      InfiniBand*, and Ethernet
    - Common API to support Deep Learning frameworks (Caffe*, Theano*,
      Torch*, etc.)

Intel(R) CCL package comprises the Intel CCL Software Development Kit (SDK)
and the Intel(R) MPI Library Runtime components.

----------------------------------------------------
Installing Intel(R) oneAPI Collective Communications Library
----------------------------------------------------

I.   Installing Intel(R) CCL using RPM Package Manager (root mode):

        1. Log in as root.

        2. Install the package:
            $ rpm -i intel-iccl-devel-64-<version>.<update>-<package#>.x86_64.rpm

            where <version>.<update>-<package#> is a string, such as: 2017.0-009

     To uninstall Intel(R) CCL, use the command:
     $ rpm -e intel-iccl-devel-64-<version>.<update>-<package#>.x86_64

II.  Installing Intel(R) CCL using install.sh (user mode):

     Run install.sh and follow the instructions.

     There is no uninstall script. To uninstall Intel(R) CCL, delete the entire
     directory where you have installed the package.

-------------------
Directory Structure
-------------------

Following a successful installation, the files associated with the Intel(R) CCL
are installed on your host system. The following directory map indicates the 
default structure and identifies the file types stored in each sub-directory:

`-- opt
    `-- intel        Common directory for Intel(R) Software Development Products.
            `-- iccl_<version>.<update>.<package#>
                |               Subdirectory for the version, specific update
                |               and package number of Intel(R) CCL.
                |-- doc         Subdirectory with documentation.
                |-- |-- API_Reference.htm
                |   |-- Developer_Guide.pdf
                |   |-- README.txt
                |   |-- Release_Notes.txt
                |   |-- api     Subdirectory with API reference.
                |-- example     Intel(R) CCL example
                |   |-- Makefile
                |   `-- iccl_example.cpp
                |-- intel64     Files for specific architecture.
                |   |-- bin     Binaries, scripts, and executable files.
                |   |   |-- ep_server
                |   |   |-- hydra_persist
                |   |   |-- icclvars.sh
                |   |   |-- mpiexec -> mpiexec.hydra
                |   |   |-- mpiexec.hydra
                |   |   |-- mpirun
                |   |   `-- pmi_proxy
                |   |-- etc     Configuration files.
                |   |   |-- mpiexec.conf
                |   |   `-- tmi.conf
                |   |-- include Include and header files.
                |   |   |-- iccl    Subdirectory for Python* module
                |   |   |-- iccl.hpp
                |   |   `-- iccl.h
                |   `-- lib     Libraries
                |       |-- libiccl.so -> libiccl.so.1
                |       |-- libiccl.so.1 -> libiccl.so.1.0
                |       |-- libiccl.so.1.0
                |       |-- libmpi.so -> libmpi.so.12
                |       |-- libmpi.so.12 -> libmpi.so.12.0
                |       |-- libmpi.so.12.0
                |       |-- libtmi.so -> libtmi.so.1.2
                |       |-- libtmi.so.1.2
                |       |-- libtmip_psm.so -> libtmip_psm.so.1.2
                |       |-- libtmip_psm.so.1.2
                |       |-- libtmip_psm2.so -> libtmip_psm2.so.1.0
                |       `-- libtmip_psm2.so.1.0
                |-- licensing
                |   |-- iccl    Subdirectory for supported files, license
                |   |           of the Intel(R) CCL
                |   `-- mpi     Subdirectory for supported files, EULAs,
                |               redist files, third-party-programs file 
                |               of the Intel(R) MPI Library
                `-- test        Intel(R) CCL tests
                    |-- Makefile
                    |-- iccl_test.py
                    `-- iccl_test.cpp

--------------------------------
Disclaimer and Legal Information
--------------------------------
No license (express or implied, by estoppel or otherwise) to any intellectual
property rights is granted by this document.

Intel disclaims all express and implied warranties, including without limitation,
the implied warranties of merchantability, fitness for a particular purpose, and
non-infringement, as well as any warranty arising from course of performance,
course of dealing, or usage in trade.

This document contains information on products, services and/or processes in
development. All information provided here is subject to change without notice.
Contact your Intel representative to obtain the latest forecast, schedule,
specifications and roadmaps.

The products and services described may contain defects or errors known as
errata which may cause deviations from published specifications. Current
characterized errata are available on request.

No computer software can provide absolute security. End users are responsible for
securing their own deployment of computer software in any environment.

Intel, Intel Core, Xeon, Xeon Phi and the Intel logo are trademarks of Intel
Corporation in the U.S. and/or other countries.

* Other names and brands may be claimed as the property of others.

(C) Intel Corporation.

Optimization Notice
-------------------

Intel's compilers may or may not optimize to the same degree for non-Intel
microprocessors for optimizations that are not unique to Intel microprocessors.
These optimizations include SSE2, SSE3, and SSSE3 instruction sets and other
optimizations. Intel does not guarantee the availability, functionality, or
effectiveness of any optimization on microprocessors not manufactured by Intel.
Microprocessor-dependent optimizations in this product are intended for use 
with Intel microprocessors. Certain optimizations not specific to Intel 
microarchitecture are reserved for Intel microprocessors. Please refer to the 
applicable product User and Reference Guides for more information regarding the
specific instruction sets covered by this notice.

Notice revision #20110804
