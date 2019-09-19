#ifndef TRAITS_H_
#define TRAITS_H_

#include <tuple>

template<ccl_datatype_t ccl_type_id>
struct type_info;

#define CCL_TYPE_TRAITS(ccl_type_id, dtype)                   \
    template<>                                                \
    struct type_info<ccl_type_id>                             \
    {                                                         \
        using native_type = dtype;                            \
        using ccl_type = std::integral_constant<ccl_datatype_t, ccl_type_id>; \
        static constexpr const char* name() { return #dtype; }\
    };

CCL_TYPE_TRAITS(ccl_dtype_char, char);
CCL_TYPE_TRAITS(ccl_dtype_int, int);
CCL_TYPE_TRAITS(ccl_dtype_int64, int64_t);
CCL_TYPE_TRAITS(ccl_dtype_uint64, uint64_t);
CCL_TYPE_TRAITS(ccl_dtype_float, float);
CCL_TYPE_TRAITS(ccl_dtype_double, double);

#define CCL_DATA_TYPE_LIST \
    ccl_dtype_char, ccl_dtype_int, ccl_dtype_int64, \
    ccl_dtype_uint64, ccl_dtype_float, ccl_dtype_double

#define CCL_IDX_TYPE_LIST \
    ccl_dtype_char, ccl_dtype_int, ccl_dtype_int64, ccl_dtype_uint64

template<ccl_datatype_t ...ccl_type>
std::tuple<type_info<ccl_type>...> ccl_type_list()
{
     return {};
}

#endif