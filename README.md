# oneAPI Collective Communications Library (oneCCL)

[Installation](#installation)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Usage](#usage)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Release Notes](https://software.intel.com/content/www/us/en/develop/articles/oneapi-collective-communication-library-ccl-release-notes.html)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Documentation](https://oneapi-src.github.io/oneCCL/)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[How to Contribute](CONTRIBUTING.md)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[License](LICENSE)

oneAPI Collective Communications Library (oneCCL) provides an efficient implementation of communication patterns used in deep learning.

oneCCL is integrated into:
* [Horovod\*](https://github.com/horovod/horovod) (distributed training framework). Refer to [Horovod with oneCCL](https://github.com/horovod/horovod/blob/master/docs/oneccl.rst) for details.
* [PyTorch\*](https://github.com/pytorch/pytorch) (machine learning framework). Refer to [PyTorch bindings for oneCCL](https://github.com/intel/torch-ccl) for details.


## Prerequisites

- Ubuntu* 18
- GNU*: C, C++ 4.8.5 or higher.

### SYCL support
Intel(R) oneAPI DPC++/C++ Compiler with L0 v1.0 support

## Installation

General installation scenario:

```
cd oneccl
mkdir build
cd build
cmake ..
make -j install
```

If you need a "clear" build, create a new build directory and invoke `cmake` within it.

You can also do the following during installation:
- [Specify installation directory](#specify-installation-directory)
- [Specify the compiler](#specify-the-compiler)
- [Specify `SYCL` cross-platform abstraction level](#specify-sycl-cross-platform-abstraction-level)
- [Specify the build type](#specify-the-build-type)
- [Enable `make` verbose output](#enable-make-verbose-output)
- [Build with address sanitizer](#build-with-address-sanitizer)

### Specify installation directory

Modify `cmake` command as follows:

```
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/installation/directory
```

If no `-DCMAKE_INSTALL_PREFIX` is specified, oneCCL will be installed into `_install` subdirectory of the current
build directory, for example, `ccl/build/_install`.

### Specify the compiler

Modify `cmake` command as follows:

```
cmake .. -DCMAKE_C_COMPILER=your_c_compiler -DCMAKE_CXX_COMPILER=your_cxx_compiler
```

### Specify `SYCL` cross-platform abstraction level

If your CXX compiler requires SYCL, it is possible to specify it (DPC++ is supported for now).
Modify `cmake` command as follows:

```
cmake .. -DCMAKE_C_COMPILER=your_c_compiler -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_RUNTIME=dpcpp
```

### Specify the build type

Modify `cmake` command as follows:

```
cmake .. -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel]
```

### Enable `make` verbose output

To see all parameters used by `make` during compilation
and linkage, modify `make` command as follows:

```
make -j VERBOSE=1
```


## Usage

### Launching Example Application

Use the command:
```bash
$ source <install_dir>/env/setvars.sh
$ mpirun -n 2 <install_dir>/examples/benchmark/benchmark
```
### Setting workers affinity

There are two ways to set worker threads (workers) affinity: [automatically](#setting-affinity-automatically) and [explicitly](#setting-affinity-explicitly).

#### Automatic setup

1. Set the `CCL_WORKER_COUNT` environment variable with the desired number of workers per process.
2. Set the `CCL_WORKER_AFFINITY` environment variable with the value `auto`.

Example:
```
export CCL_WORKER_COUNT=4
export CCL_WORKER_AFFINITY=auto
```
With the variables above, oneCCL will create four workers per process and the pinning will depend from process launcher.

If an application has been launched using `mpirun` that is provided by oneCCL distribution package then workers will be automatically pinned to the last four cores available for the launched process. The exact IDs of CPU cores can be controlled by `mpirun` parameters.

Otherwise, workers will be automatically pinned to the last four cores available on the node.

#### Explicit setup

1. Set the `CCL_WORKER_COUNT` environment variable with the desired number of workers per process.
2. Set the `CCL_WORKER_AFFINITY` environment variable with the IDs of cores to pin local workers.

Example:
```
export CCL_WORKER_COUNT=4
export CCL_WORKER_AFFINITY=3,4,5,6
```
With the variables above, oneCCL will create four workers per process and pin them to the cores with the IDs of 3, 4, 5, and 6 respectively.


## FAQ

### When do I need a clean build? When should I remove my favorite build directory?

In most cases, there is no need to remove the current build directory. You can jsut run `make` to
compile and link changed files. 

However, if you see some suspicious build errors after a significant
change in code (for example, after rebase or a change of branch), it is a hint for you to clean the build directory.

## Contribute

See [CONTRIBUTING](CONTRIBUTING.md) for more information.

## License

Distributed under the Apache License 2.0 license. See [LICENSE](LICENSE) for more
information.

## Security

To report a vulnerability, refer to [Intel vulnerability reporting policy](https://www.intel.com/content/www/us/en/security-center/default.html).
