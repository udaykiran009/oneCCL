#pragma once

#include "oneapi/ccl.hpp"

#define PRINT(fmt, ...) printf(fmt "\n", ##__VA_ARGS__);

#ifndef PRINT_BY_ROOT
#define PRINT_BY_ROOT(comm, fmt, ...) \
    if (comm.rank() == 0) { \
        printf(fmt "\n", ##__VA_ARGS__); \
    }
#endif //PRINT_BY_ROOT

#define ASSERT(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            printf("FAILED\n"); \
            fprintf(stderr, "ASSERT '%s' FAILED " fmt "\n", #cond, ##__VA_ARGS__); \
            throw std::runtime_error("ASSERT FAILED"); \
        } \
    } while (0)

// TODO: add ccl::bf16
constexpr std::initializer_list<ccl::datatype> all_dtypes = {
    ccl::datatype::int8,    ccl::datatype::int32, ccl::datatype::float32,
    ccl::datatype::float64, ccl::datatype::int64, ccl::datatype::uint64
};

typedef enum { BACKEND_HOST, BACKEND_SYCL } backend_type_t;
typedef enum { LOOP_REGULAR, LOOP_UNORDERED } loop_type_t;
typedef enum { BUF_SINGLE, BUF_MULTI } buf_type_t;

typedef enum { SYCL_MEM_USM, SYCL_MEM_BUF } sycl_mem_type_t;
typedef enum { SYCL_USM_SHARED, SYCL_USM_DEVICE } sycl_usm_type_t;

std::map<backend_type_t, std::string> backend_names = {
    std::make_pair(BACKEND_HOST, "host"),
    std::make_pair(BACKEND_SYCL, "sycl")
};

std::map<loop_type_t, std::string> loop_names = { std::make_pair(LOOP_REGULAR, "regular"),
                                                  std::make_pair(LOOP_UNORDERED, "unordered") };

std::map<buf_type_t, std::string> buf_names = { std::make_pair(BUF_MULTI, "multi"),
                                                std::make_pair(BUF_SINGLE, "single") };

std::map<sycl_mem_type_t, std::string> sycl_mem_names = {
    std::make_pair(SYCL_MEM_USM, "usm"),
    std::make_pair(SYCL_MEM_BUF, "buf")
};

std::map<sycl_usm_type_t, std::string> sycl_usm_names = {
    std::make_pair(SYCL_USM_SHARED, "shared"),
    std::make_pair(SYCL_USM_DEVICE, "device")
};

// TODO: add ccl::bf16
std::map<ccl::datatype, std::string> dtype_names = {
    std::make_pair(ccl::datatype::int8, "char"),
    std::make_pair(ccl::datatype::int32, "int"),
    std::make_pair(ccl::datatype::float32, "float"),
    std::make_pair(ccl::datatype::float64, "double"),
    std::make_pair(ccl::datatype::int64, "int64"),
    std::make_pair(ccl::datatype::uint64, "uint64"),
};

std::map<ccl::reduction, std::string> reduction_names = {
    std::make_pair(ccl::reduction::sum, "sum"),
    std::make_pair(ccl::reduction::prod, "prod"),
    std::make_pair(ccl::reduction::min, "min"),
    std::make_pair(ccl::reduction::max, "max"),
};

// TODO: add ccl::bf16
template <class native_type>
using checked_dtype_t = std::pair<bool, native_type>;
using supported_dtypes_t = std::tuple<checked_dtype_t<char>,
                                      checked_dtype_t<int>,
                                      checked_dtype_t<float>,
                                      checked_dtype_t<double>,
                                      checked_dtype_t<int64_t>,
                                      checked_dtype_t<uint64_t>>;
supported_dtypes_t launch_dtypes;
