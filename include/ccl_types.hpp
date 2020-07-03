#pragma once

#include "ccl_types.h"

#include <bitset>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

class ccl_internal_kvs_impl;
namespace ccl
{

/**
 * Supported reduction operations
 */
enum class reduction
{
    sum = ccl_reduction_sum,
    prod = ccl_reduction_prod,
    min = ccl_reduction_min,
    max = ccl_reduction_max,
    custom = ccl_reduction_custom,

    last_value = ccl_reduction_last_value
};

/**
 * Supported datatypes
 */
enum datatype: int
{
    dt_char = ccl_dtype_char,
    dt_int = ccl_dtype_int,
    dt_bfp16 = ccl_dtype_bfp16,
    dt_float = ccl_dtype_float,
    dt_double = ccl_dtype_double,
    dt_int64 = ccl_dtype_int64,
    dt_uint64 = ccl_dtype_uint64,

    dt_last_value = ccl_dtype_last_value
};

/**
 * Supported stream types
 */
enum class stream_type
{
    host = ccl_stream_host,
    cpu = ccl_stream_cpu,
    gpu = ccl_stream_gpu,

    last_value = ccl_stream_last_value
};

typedef ccl_coll_attr_t coll_attr;

typedef ccl_comm_attr_t comm_attr;

typedef ccl_datatype_attr_t datatype_attr;

template<ccl_comm_split_attributes attrId>
struct ccl_comm_split_attributes_traits {};

/**
 * Exception type that may be thrown by ccl API
 */
class ccl_error : public std::runtime_error
{
public:
    explicit ccl_error(const std::string& message) : std::runtime_error(message)
    {}

    explicit ccl_error(const char* message) : std::runtime_error(message)
    {}
};

/**
 * Type traits, which describes how-to types would be interpretered by ccl API
 */
template<class ntype_t, size_t size_of_type, ccl_datatype_t ccl_type_v, bool iclass = false, bool supported = false>
struct ccl_type_info_export
{
    using native_type = ntype_t;
    using ccl_type = std::integral_constant<ccl_datatype_t, ccl_type_v>;
    static constexpr size_t size = size_of_type;
    static constexpr ccl_datatype_t ccl_type_value = ccl_type::value;
    static constexpr datatype ccl_datatype_value = static_cast<datatype>(ccl_type_value);
    static constexpr bool is_class = iclass;
    static constexpr bool is_supported = supported;
};

class ccl_kvs_interface
{
public:
    virtual bool get(const std::string& prefix, const std::string& key, std::vector<char>& result) const = 0;

    virtual void put(const std::string& prefix, const std::string& key, const std::vector<char>& data) const = 0;

    virtual ~ccl_kvs_interface() = default;
};

#define MASTER_ADDR_MAX_SIZE 256
using ccl_master_addr = std::array<char, MASTER_ADDR_MAX_SIZE>;

class ccl_internal_kvs final: public ccl::ccl_kvs_interface
{
public:
    ccl_internal_kvs();
    ccl_internal_kvs(ccl_master_addr& master_addr);

    ccl_master_addr get_master_addr();

    bool get(const std::string& prefix, const std::string& key, std::vector<char>& result) const override;

    void put(const std::string& prefix, const std::string& key, const std::vector<char>& data) const override;

    ~ccl_internal_kvs() override;

private:
    std::unique_ptr<ccl_internal_kvs_impl> internal_kvs_impl;

};

}
#ifdef MULTI_GPU_SUPPORT
    #include "ccl_device_types.hpp"
#endif
