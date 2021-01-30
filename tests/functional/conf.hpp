#pragma once

#include <map>
#include <vector>

#include "oneapi/ccl.hpp"

#define max_test_count() \
    (ORDER_LAST * ORDER_LAST * CMPT_LAST * SNCT_LAST * DT_LAST * ST_LAST * RT_LAST * BC_LAST * \
     CT_LAST * PT_LAST * PTYPE_LAST * ETYPE_LAST)

#define POST_AND_PRE_INCREMENTS_DECLARE(EnumName) \
    EnumName& operator++(EnumName& orig); \
    EnumName operator++(EnumName& orig, int);

#define POST_AND_PRE_INCREMENTS(EnumName, LAST_ELEM) \
    EnumName& operator++(EnumName& orig) { \
        if (orig != LAST_ELEM) \
            orig = static_cast<EnumName>(orig + 1); \
        return orig; \
    } \
    EnumName operator++(EnumName& orig, int) { \
        EnumName rVal = orig; \
        ++orig; \
        return rVal; \
    }

#define SOME_VALUE (0xdeadbeef)
#define ROOT_RANK  (0)

typedef enum { PT_IN = 0, PT_OOP = 1, PT_LAST } ccl_place_type;
extern ccl_place_type first_ccl_place_type;
extern ccl_place_type last_ccl_place_type;
extern std::map<int, const char*> ccl_place_type_str;

typedef enum { ST_SMALL = 0, ST_MEDIUM = 1, ST_LARGE = 2, ST_LAST } ccl_size_type;
extern ccl_size_type first_ccl_size_type;
extern ccl_size_type last_ccl_size_type;
extern std::map<int, const char*> ccl_size_type_str;
extern std::map<int, size_t> ccl_size_type_values;

typedef enum { BC_SMALL = 0, BC_LARGE = 1, BC_LAST } ccl_buffer_count;
extern ccl_buffer_count first_ccl_buffer_count;
extern ccl_buffer_count last_ccl_buffer_count;
extern std::map<int, const char*> ccl_buffer_count_str;
extern std::map<int, size_t> ccl_buffer_count_values;

typedef enum { CMPT_WAIT = 0, CMPT_TEST = 1, CMPT_LAST } ccl_completion_type;
extern ccl_completion_type first_ccl_completion_type;
extern ccl_completion_type last_ccl_completion_type;
extern std::map<int, const char*> ccl_completion_type_str;

typedef enum {
    PTYPE_NULL = 0,
#ifdef TEST_CCL_CUSTOM_PROLOG
    PTYPE_T_TO_2X = 1,
    PTYPE_T_TO_CHAR = 2,
#endif
    PTYPE_LAST
} ccl_prolog_type;
extern ccl_prolog_type first_ccl_prolog_type;
extern ccl_prolog_type last_ccl_prolog_type;
extern std::map<int, const char*> ccl_prolog_type_str;

typedef enum {
    ETYPE_NULL = 0,
#ifdef TEST_CCL_CUSTOM_EPILOG
    ETYPE_T_TO_2X = 1,
    ETYPE_CHAR_TO_T = 2,
#endif
    ETYPE_LAST
} ccl_epilog_type;
extern ccl_epilog_type first_ccl_epilog_type;
extern ccl_epilog_type last_ccl_epilog_type;
extern std::map<int, const char*> ccl_epilog_type_str;

typedef enum {
    DT_INT8 = 0,
    DT_UINT8,
    DT_INT16,
    DT_UINT16,
    DT_INT32,
    DT_UINT32,
    DT_INT64,
    DT_UINT64,
    DT_FLOAT16,
    DT_FLOAT32,
    DT_FLOAT64,
    DT_BFLOAT16,

    DT_LAST
} ccl_data_type;
extern ccl_data_type first_ccl_data_type;
extern ccl_data_type last_ccl_data_type;
extern std::map<int, const char*> ccl_data_type_str;
extern std::map<int, ccl::datatype> ccl_datatype_values;

typedef enum {
    RT_SUM = 0,
    RT_PROD = 1,
    RT_MIN = 2,
    RT_MAX = 3,
#ifdef TEST_CCL_CUSTOM_REDUCE
    RT_CUSTOM = 4,
    RT_CUSTOM_NULL = 5,
#endif
    RT_LAST
} ccl_reduction_type;
extern ccl_reduction_type first_ccl_reduction_type;
extern ccl_reduction_type last_ccl_reduction_type;
extern std::map<int, const char*> ccl_reduction_type_str;
extern std::map<int, ccl::reduction> ccl_reduction_values;

typedef enum { CT_CACHE_0 = 0, CT_CACHE_1 = 1, CT_LAST } ccl_cache_type;
extern ccl_cache_type first_ccl_cache_type;
extern ccl_cache_type last_ccl_cache_type;
extern std::map<int, const char*> ccl_cache_type_str;
extern std::map<int, int> ccl_cache_type_values;

typedef enum { SNCT_SYNC_0 = 0, SNCT_SYNC_1 = 1, SNCT_LAST } ccl_sync_type;
extern ccl_sync_type first_ccl_sync_type;
extern ccl_sync_type last_ccl_sync_type;
extern std::map<int, const char*> ccl_sync_type_str;
extern std::map<int, int> ccl_sync_type_values;

typedef enum { ORDER_DIRECT = 0, ORDER_INDIRECT = 1, ORDER_RANDOM = 2, ORDER_LAST } ccl_order_type;
extern ccl_order_type first_ccl_order_type;
extern ccl_order_type last_ccl_order_type;
extern std::map<int, const char*> ccl_order_type_str;
extern std::map<int, int> ccl_order_type_values;

POST_AND_PRE_INCREMENTS_DECLARE(ccl_place_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_size_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_completion_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_data_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_reduction_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_cache_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_sync_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_order_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_buffer_count);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_prolog_type);
POST_AND_PRE_INCREMENTS_DECLARE(ccl_epilog_type);

struct ccl_test_conf {
    ccl_place_type place_type;
    ccl_cache_type cache_type;
    ccl_sync_type sync_type;
    ccl_size_type size_type;
    ccl_completion_type completion_type;
    ccl_reduction_type reduction;
    ccl_data_type datatype;
    ccl_order_type complete_order_type;
    ccl_order_type start_order_type;
    ccl_buffer_count buffer_count;
    ccl_prolog_type prolog_type;
    ccl_epilog_type epilog_type;
};

extern std::vector<ccl_test_conf> test_params;

std::ostream& operator<<(std::ostream& stream, ccl_test_conf const& conf);

size_t get_ccl_elem_count(ccl_test_conf& test_conf);
size_t get_ccl_buffer_count(ccl_test_conf& test_conf);
ccl::datatype get_ccl_lib_datatype(const ccl_test_conf& test_conf);
ccl::reduction get_ccl_lib_reduction(const ccl_test_conf& test_conf);
size_t calculate_test_count();
void init_test_params();
void print_err_message(char* err_message, std::ostream& output);
