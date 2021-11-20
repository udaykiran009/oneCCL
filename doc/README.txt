-------------------------------------------------------
oneAPI Collective Communications Library for Linux* OS
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

oneAPI Collective Communications Library (oneCCL) provides
an efficient implementation of communication patterns used in deep learning.

oneCCL package comprises the oneCCL Software Development Kit (SDK)
and the Intel(R) MPI Library Runtime components.

----------------------------------------------------
Installing oneAPI Collective Communications Library
----------------------------------------------------

Installing oneCCL using install.sh (user mode):

 Run install.sh and follow the instructions.

 There is no uninstall script. To uninstall oneCCL, delete the entire
 directory where you have installed the package.

-------------------
Directory Structure
-------------------

Following a successful installation, the files associated with the oneCCL
are installed on your host system. The following directory map indicates the 
default structure and identifies the file types stored in each sub-directory:

`-- opt
    `-- intel        Common directory for Intel(R) Software Development Products.
            `-- ccl_<version>.<update>.<package#>
                |               Subdirectory for the version, specific update
                |               and package number of oneCCL.
                |-- env
                |-- examples
                |   |-- benchmark
                |   |-- common
                |   |-- cpu
                |   |-- include
                |   `-- sycl
                |-- include
                |   |-- cpu_gpu_dpcpp
                |   `-- cpu
                |-- lib
                |   |-- cpu_gpu_dpcpp
                |   `-- cpu
                |-- licensing
                `-- modulefiles

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
