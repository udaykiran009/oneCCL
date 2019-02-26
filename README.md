# mlsl2
Intel Machine Learning Scaling Library

## Installation
### General installation scenario

```
cd mlsl2
mkdir build
cd build
cmake ..
make -j install
```

If a "clear" build is needed, then one should create a new build directory and invoke `cmake` within it.

### Specify installation directory
Modify `cmake` command as follow:

```
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/installation/directory
```

If no `-DCMAKE_INSTALL_PREFIX` is specified then mlsl2 will be installed into `_install` subdirectory of the current
build directory, e.g. `mlsl2/build/_install`

### Specify compiler
Modify `cmake` command as follow:

```
cmake .. -DCMAKE_C_COMPILER=your_c_compiler -DCMAKE_CXX_COMPILER=your_cxx_compiler
```

### Specify build type
Modify `cmake` command as follow:

```
cmake .. -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel]
```

### Enable `make` verbose output
Modify `make` command as follow to see all parameters used by `make` during compilation
and linkage:

```
make -j VERBOSE=1
```

### Generate archive with installed files
```
make -j install
make archive
```

## FAQ

### When do I need a clean build? When should I remove my favorite build directory?

In the most cases there is no need in removal of the current build directory. Just run `make` to 
compile and link changed files. Only if one sees some suspicious build errors after significant 
change in the code (e.g. after rebase or change of branch) then it is a hint to clean build directory.

### I can't run mlsl examples

It is recommended to use environment values stored in `intel64/bin//mlslvars.sh` of your current
installation directory.
