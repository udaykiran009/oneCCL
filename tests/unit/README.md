# Level_zero Unit Tests

The folder for unit tests

## Installation
General installation scenario:
```
1) cd oneccl
   mkdir build
   cd build
```

2a) To compile with level_zero only 
```
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCOMPUTE_BACKEND=level_zero ..
```
2b) To compile with dpcpp + level_zero
```
cmake -DCMAKE_CXX_COMPILER=dpcpp -DCMAKE_C_COMPILER=clang -DCOMPUTE_BACKEND=dpcpp_level_zero ..
```

3) Then make final target

```
make -j <target>
```
In order to know targets use:
```
make help
```
## TODOs:
1) Investigate required flags for `test/unit` from top-level CMakeLists.txt
2) Have to disband this folder into independent essences, like: api, device_topology, kernels and etc.
