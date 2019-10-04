# Intel(R) oneAPI Collective Communications Library for Linux* OS
## Introduction ##

TODO: Should be updated!!!

oneAPI Collective Communications Library (oneCCL) is a library providing
an efficient implementation of communication patterns used in deep learning.

    - Built on top of MPI, allows for use of other communication libraries
    - Optimized to drive scalability of communication patterns
    - Works across various interconnects: Intel(R) Omni-Path Architecture,
      InfiniBand*, and Ethernet
    - Common API to support Deep Learning frameworks (Caffe*, Theano*,
      Torch*, etc.)

oneCCL package comprises the oneCCL Software Development Kit (SDK)
and the Intel(R) MPI Library Runtime components.
## SOFTWARE SYSTEM REQUIREMENTS ##
This section describes the required software.

Operating Systems:

    - Red Hat* Enterprise Linux* 6 or 7
    - SuSE* Linux* Enterprise Server 12
    - Ubuntu* 16

Compilers:

    - GNU*: C, C++ 4.4.0 or higher
    - Intel(R) C++ Compiler for Linux* OS 16.0 through 17.0 or higher

Virtual Environments:
    - Docker*
    - KVM*
## Installing Intel(R) oneAPI Collective Communications Library ##
Installing the oneCCL using RPM Package Manager (root mode):

    1. Log in as root

    2. Install the package:

        $ rpm -i intel-ccl-devel-64-<version>.<update>-<package#>.x86_64.rpm

        where <version>.<update>-<package#> is a string, such as: 2017.0-009

    3. Uninstalling the oneCCL using the RPM Package Manager

        $ rpm -e intel-ccl-devel-64-<version>.<update>-<package#>.x86_64

Installing the oneCCL using install.sh (user mode):

    1. Run install.sh and follow the instructions.

    There is no uninstall script. To uninstall the oneCCL, delete the
    full directory you have installed the package into.
## Documentation ##
Please refer to [Documentation Generation](doc_build.md) for instructions on how to generate documentation on your computer.

## License ##
oneCCL is licensed under [Intel Simplified Software License](https://github.com/01org/ICCL/blob/master/LICENSE).
## Optimization Notice ##
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
