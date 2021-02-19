# oneAPI Collective Communications Library (oneCCL) <!-- omit in toc -->

[Installation](#installation)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Usage](#usage)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Release Notes](https://software.intel.com/content/www/us/en/develop/articles/oneapi-collective-communication-library-ccl-release-notes.html)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[Documentation](https://oneapi-src.github.io/oneCCL/)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[How to Contribute](CONTRIBUTING.md)&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;[License](LICENSE)

oneAPI Collective Communications Library (oneCCL) provides an efficient implementation of communication patterns used in deep learning.

oneCCL is integrated into:
* [Horovod\*](https://github.com/horovod/horovod) (distributed training framework). Refer to [Horovod with oneCCL](https://github.com/horovod/horovod/blob/master/docs/oneccl.rst) for details.
* [PyTorch\*](https://github.com/pytorch/pytorch) (machine learning framework). Refer to [PyTorch bindings for oneCCL](https://github.com/intel/torch-ccl) for details.

## Table of Contents <!-- omit in toc -->

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
  - [Launching Example Application](#launching-example-application)
  - [Setting workers affinity](#setting-workers-affinity)
    - [Automatic setup](#automatic-setup)
    - [Explicit setup](#explicit-setup)
- [Additional Resources](#additional-resources)
  - [Blog Posts](#blog-posts)
  - [Workshop Materials](#workshop-materials)

## Prerequisites 

- Ubuntu* 18
- GNU*: C, C++ 4.8.5 or higher.

Refer to [System Requirements](https://software.intel.com/content/www/us/en/develop/articles/oneapi-collective-communication-library-system-requirements.html) for more details.

### SYCL support <!-- omit in toc -->
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

If you need a clean build, create a new build directory and invoke `cmake` within it. Refer to FAQ to learn [when you might need a clean build](#when-do-i-need-a-clean-build-when-should-i-remove-my-favorite-build-directory).

You can also do the following during installation:
- [Specify installation directory](INSTALL.md#specify-installation-directory)
- [Specify the compiler](INSTALL.md#specify-the-compiler)
- [Specify `SYCL` cross-platform abstraction level](INSTALL.md#specify-sycl-cross-platform-abstraction-level)
- [Specify the build type](INSTALL.md#specify-the-build-type)
- [Enable `make` verbose output](INSTALL.md#enable-make-verbose-output)
- [Build with address sanitizer](INSTALL.md#build-with-address-sanitizer)

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

---

#### Explicit setup

1. Set the `CCL_WORKER_COUNT` environment variable with the desired number of workers per process.
2. Set the `CCL_WORKER_AFFINITY` environment variable with the IDs of cores to pin local workers.

Example:
```
export CCL_WORKER_COUNT=4
export CCL_WORKER_AFFINITY=3,4,5,6
```
With the variables above, oneCCL will create four workers per process and pin them to the cores with the IDs of 3, 4, 5, and 6 respectively.

## Additional Resources

### Blog Posts

- [Optimizing DLRM by using PyTorch with oneCCL Backend](https://pytorch.medium.com/optimizing-dlrm-by-using-pytorch-with-oneccl-backend-9f85b8ef6929)
- [Intel MLSL Makes Distributed Training with MXNet Faster](https://medium.com/apache-mxnet/intel-mlsl-makes-distributed-training-with-mxnet-faster-7186ad245e81)

### Workshop Materials

- oneAPI, oneCCL and OFI: Path to Heterogeneous Architecure Programming with Scalable Collective Communications: [recording](https://www.youtube.com/watch?v=ksiZ90EtP98&feature=youtu.be) and [slides](https://www.openfabrics.org/wp-content/uploads/2020-workshop-presentations/502.-OFA-Virtual-Workshop-2020-oneCCL-v5.pdf)

## FAQ <!-- omit in toc -->

### When do I need a clean build? When should I remove my favorite build directory? <!-- omit in toc -->

In most cases, there is no need to remove the current build directory. You can just run `make` to
compile and link changed files. 

However, if you see some suspicious build errors after a significant
change in code (for example, after rebase or a change of branch), it is a hint for you to clean the build directory.

## Contribute <!-- omit in toc -->

See [CONTRIBUTING](CONTRIBUTING.md) for more information.

## License <!-- omit in toc -->

Distributed under the Apache License 2.0 license. See [LICENSE](LICENSE) for more
information.

## Security <!-- omit in toc -->

To report a vulnerability, refer to [Intel vulnerability reporting policy](https://www.intel.com/content/www/us/en/security-center/default.html).
