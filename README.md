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
cmake .. -DCMAKE_C_COMPILER=your_compiler
```

### Specify build type
Modify `cmake` command as follow:

```
cmake .. -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel]
```

### Enable make verbose output
Modify `make` command as follow:

```
make -j VERBOSE=1
```
