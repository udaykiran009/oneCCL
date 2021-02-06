#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "atl.h"
#include "comp/bf16/bf16_intrisics.hpp"
#include "comp/bf16/bf16_utils.hpp"
#include "comp/fp16/fp16_intrisics.hpp"
#include "comp/fp16/fp16_utils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define ATL_MPI_PM_KEY     "atl-mpi"
#define EP_IDX_MAX_STR_LEN 4

#define EP_IDX_KEY    "ep_idx"
#define NIC_IDX_KEY   "pref_close_nic"
#define NIC_COUNT_KEY "num_close_nics"

#define RET2ATL(ret) (ret != MPI_SUCCESS) ? ATL_STATUS_FAILURE : ATL_STATUS_SUCCESS

typedef enum { ATL_MPI_LIB_NONE, ATL_MPI_LIB_IMPI, ATL_MPI_LIB_MPICH } atl_mpi_lib_type_t;

typedef struct {
    atl_mpi_lib_type_t type;
    const char* name;

    /* string prefix before numerical version of library, mandatory */
    const char* version_prefix_1;

    /* string prefix before numerical version of library, following prefix_1, optional */
    const char* version_prefix_2;

    /* minimal expected version of library, mandatory */
    int min_version_value;

    /* string prefix before library kind, optional */
    const char* kind_prefix;

    /* library kind, optional */
    const char* kind_value;
} atl_mpi_lib_info_t;

#define MPI_LIB_INFO_MAX_COUNT 2

static atl_mpi_lib_info_t mpi_lib_infos[MPI_LIB_INFO_MAX_COUNT] = {
    { ATL_MPI_LIB_IMPI, "impi", "Intel(R) MPI Library", "", 2019, "library kind:", "release_mt" },
    { ATL_MPI_LIB_MPICH, "mpich", "MPICH Custom Information:", "drop", 34, "", "" }
};

#ifdef CCL_BF16_COMPILER
#define ATL_MPI_BF16
#endif /* CCL_BF16_COMPILER */

#ifdef CCL_FP16_COMPILER
#define ATL_MPI_FP16
#endif /* CCL_FP16_COMPILER */

typedef struct {
    // custom MPI operations for BF16
    MPI_Op sum_op;
    MPI_Op prod_op;
    MPI_Op min_op;
    MPI_Op max_op;
    // custom MPI dtype for BF16
    MPI_Datatype dtype;
    ccl_bf16_impl_type impl_type;
} atl_mpi_bf16_data_t;

typedef struct {
    // custom MPI operations for FP16
    MPI_Op sum_op;
    MPI_Op prod_op;
    MPI_Op min_op;
    MPI_Op max_op;
    // custom MPI dtype for FP16
    MPI_Datatype dtype;
    ccl_fp16_impl_type impl_type;
} atl_mpi_fp16_data_t;

typedef struct {
    atl_mpi_lib_type_t mpi_lib_type;
    atl_mpi_bf16_data_t bf16;
    atl_mpi_fp16_data_t fp16;
    atl_progress_mode_t progress_mode;
    int sync_coll;
    int extra_ep;
    int is_external_init;
    size_t nic_count;
} atl_mpi_global_data_t;

static atl_mpi_global_data_t global_data;

typedef enum { ATL_MPI_COMP_POSTED, ATL_MPI_COMP_COMPLETED } atl_mpi_comp_state_t;

typedef struct {
    MPI_Request native_req;
    atl_mpi_comp_state_t comp_state;
} atl_mpi_req_t;

typedef struct {
    atl_ctx_t ctx;
} atl_mpi_ctx_t;

typedef struct {
    atl_ep_t ep;
    MPI_Comm mpi_comm;

    /* dummy recv operation to ensure progress in atl_poll */
    atl_mpi_req_t dummy_req;
    MPI_Comm dummy_comm;
} atl_mpi_ep_t;

#define MPI_BFLOAT16 \
    ({ \
        CCL_THROW_IF_NOT(global_data.bf16.dtype != MPI_DATATYPE_NULL, \
                         "unsupported datatype: ATL_DTYPE_BF16"); \
        global_data.bf16.dtype; \
    })

#define MPI_FLOAT16 \
    ({ \
        CCL_THROW_IF_NOT(global_data.fp16.dtype != MPI_DATATYPE_NULL, \
                         "unsupported datatype: ATL_DTYPE_FP16"); \
        global_data.fp16.dtype; \
    })

// helpers: check contract
static inline void atl_mpi_check_op_params(void* in_buf,
                                           void* inout_buf,
                                           int* length,
                                           MPI_Datatype* datatype,
                                           const char* caller_func_name) {
    (void)datatype;
    CCL_THROW_IF_NOT(in_buf && inout_buf && length,
                     caller_func_name,
                     " requested, bad arguments: ",
                     in_buf,
                     " ",
                     inout_buf,
                     " ",
                     length);
}

static void atl_mpi_print_error(int error) __attribute__((unused));
static void atl_mpi_print_error(int error) {
    char str_error[MPI_MAX_ERROR_STRING];
    int result_len = MPI_MAX_ERROR_STRING;

    MPI_Error_string(error, str_error, &result_len);

    if (result_len > MPI_MAX_ERROR_STRING) {
        result_len = MPI_MAX_ERROR_STRING;
    }
    str_error[result_len - 1] = '\0';

    ccl_logger::format(std::cout, "MPI error: %s (%d)", str_error, error);
}

#ifdef ATL_MPI_BF16

static void BF16_INLINE_TARGET_ATTRIBUTE_ALL atl_mpi_bf16_base_op(void* in,
                                                                  void* inout,
                                                                  int* length,
                                                                  ccl_bf16_reduction_func_ptr op) {
    unsigned short* in_buf = (unsigned short*)in;
    unsigned short* inout_buf = (unsigned short*)inout;

    size_t len = *length;
    ccl_bf16_reduce_impl(in_buf, inout_buf, len, op, global_data.bf16.impl_type);
}

static void BF16_TARGET_ATTRIBUTE_ALL atl_mpi_bf16_sum_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_bf16_base_op(in, inout, length, &bf16_sum_wrap);
}

static void BF16_TARGET_ATTRIBUTE_ALL atl_mpi_bf16_prod_op(void* in,
                                                           void* inout,
                                                           int* length,
                                                           MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_bf16_base_op(in, inout, length, &bf16_prod_wrap);
}

static void BF16_TARGET_ATTRIBUTE_ALL atl_mpi_bf16_min_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_bf16_base_op(in, inout, length, &bf16_min_wrap);
}

static void BF16_TARGET_ATTRIBUTE_ALL atl_mpi_bf16_max_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_bf16_base_op(in, inout, length, &bf16_max_wrap);
}
#endif /* ATL_MPI_BF16 */

#ifdef ATL_MPI_FP16

static void FP16_INLINE_TARGET_ATTRIBUTE_ALL atl_mpi_fp16_base_op(void* in,
                                                                  void* inout,
                                                                  int* length,
                                                                  ccl_fp16_reduction_func_ptr op) {
    unsigned short* in_buf = (unsigned short*)in;
    unsigned short* inout_buf = (unsigned short*)inout;

    size_t len = *length;
    ccl_fp16_reduce_impl(in_buf, inout_buf, len, op, global_data.fp16.impl_type);
}

static void FP16_TARGET_ATTRIBUTE_ALL atl_mpi_fp16_sum_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_fp16_base_op(in, inout, length, &fp16_sum_wrap);
}

static void FP16_TARGET_ATTRIBUTE_ALL atl_mpi_fp16_prod_op(void* in,
                                                           void* inout,
                                                           int* length,
                                                           MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_fp16_base_op(in, inout, length, &fp16_prod_wrap);
}

static void FP16_TARGET_ATTRIBUTE_ALL atl_mpi_fp16_min_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_fp16_base_op(in, inout, length, &fp16_min_wrap);
}

static void FP16_TARGET_ATTRIBUTE_ALL atl_mpi_fp16_max_op(void* in,
                                                          void* inout,
                                                          int* length,
                                                          MPI_Datatype* datatype) {
    atl_mpi_check_op_params(in, inout, length, datatype, __FUNCTION__);
    atl_mpi_fp16_base_op(in, inout, length, &fp16_max_wrap);
}
#endif /* ATL_MPI_FP16 */

static int atl_mpi_bf16_init() {
    int ret = MPI_SUCCESS;

    global_data.bf16.dtype = MPI_DATATYPE_NULL;

    global_data.bf16.sum_op = MPI_OP_NULL;
    global_data.bf16.prod_op = MPI_OP_NULL;
    global_data.bf16.min_op = MPI_OP_NULL;
    global_data.bf16.max_op = MPI_OP_NULL;

    global_data.bf16.impl_type = ccl_bf16_get_impl_type();

    if (global_data.bf16.impl_type == ccl_bf16_compiler_none) {
        LOG_INFO("BF16: disabled on compiler level");
        return RET2ATL(ret);
    }
    else if (global_data.bf16.impl_type == ccl_bf16_hw_none) {
        LOG_INFO("BF16: disabled on hardware level");
        return RET2ATL(ret);
    }

#ifdef ATL_MPI_BF16

    // create custom MPI BF16 dtype
    ret = MPI_Type_contiguous(2, MPI_BYTE, &global_data.bf16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 dtype");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    ret = MPI_Type_commit(&global_data.bf16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot commit MPI BF16 type");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI BF16 summation op
    ret = MPI_Op_create(&atl_mpi_bf16_sum_op, 1, &global_data.bf16.sum_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 sum op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI BF16 production op
    ret = MPI_Op_create(&atl_mpi_bf16_prod_op, 1, &global_data.bf16.prod_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 prod op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI BF16 min op
    ret = MPI_Op_create(&atl_mpi_bf16_min_op, 1, &global_data.bf16.min_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 min op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI BF16 max op
    ret = MPI_Op_create(&atl_mpi_bf16_max_op, 1, &global_data.bf16.max_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 max op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

#endif /* ATL_MPI_BF16 */

    return RET2ATL(ret);
}

static void atl_mpi_bf16_finalize() {
    if (global_data.bf16.dtype != MPI_DATATYPE_NULL) {
        MPI_Type_free(&global_data.bf16.dtype);
    }

    if (global_data.bf16.sum_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.bf16.sum_op);
    }

    if (global_data.bf16.prod_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.bf16.prod_op);
    }

    if (global_data.bf16.min_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.bf16.min_op);
    }

    if (global_data.bf16.max_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.bf16.max_op);
    }
}

static int atl_mpi_fp16_init() {
    int ret = MPI_SUCCESS;

    global_data.fp16.dtype = MPI_DATATYPE_NULL;

    global_data.fp16.sum_op = MPI_OP_NULL;
    global_data.fp16.prod_op = MPI_OP_NULL;
    global_data.fp16.min_op = MPI_OP_NULL;
    global_data.fp16.max_op = MPI_OP_NULL;

    global_data.fp16.impl_type = ccl_fp16_get_impl_type();

    if (global_data.fp16.impl_type == ccl_fp16_compiler_none) {
        LOG_INFO("FP16: disabled on compiler level");
        return RET2ATL(ret);
    }
    else if (global_data.fp16.impl_type == ccl_fp16_hw_none) {
        LOG_INFO("FP16: disabled on hardware level");
        return RET2ATL(ret);
    }

#ifdef ATL_MPI_FP16

    // create custom MPI FP16 dtype
    ret = MPI_Type_contiguous(2, MPI_BYTE, &global_data.fp16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 dtype");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    ret = MPI_Type_commit(&global_data.fp16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot commit MPI FP16 type");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI FP16 summation op
    ret = MPI_Op_create(&atl_mpi_fp16_sum_op, 1, &global_data.fp16.sum_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 sum op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI FP16 production op
    ret = MPI_Op_create(&atl_mpi_fp16_prod_op, 1, &global_data.fp16.prod_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 prod op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI FP16 min op
    ret = MPI_Op_create(&atl_mpi_fp16_min_op, 1, &global_data.fp16.min_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 min op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

    // create custom MPI FP16 max op
    ret = MPI_Op_create(&atl_mpi_fp16_max_op, 1, &global_data.fp16.max_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 max op");
        atl_mpi_print_error(ret);
        return RET2ATL(ret);
    }

#endif /* ATL_MPI_FP16 */

    return RET2ATL(ret);
}

static void atl_mpi_fp16_finalize() {
    if (global_data.fp16.dtype != MPI_DATATYPE_NULL) {
        MPI_Type_free(&global_data.fp16.dtype);
    }

    if (global_data.fp16.sum_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.fp16.sum_op);
    }

    if (global_data.fp16.prod_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.fp16.prod_op);
    }

    if (global_data.fp16.min_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.fp16.min_op);
    }

    if (global_data.fp16.max_op != MPI_OP_NULL) {
        MPI_Op_free(&global_data.fp16.max_op);
    }
}

static MPI_Datatype atl2mpi_dtype(atl_datatype_t dtype) {
    switch (dtype) {
        case ATL_DTYPE_INT8: return MPI_CHAR;
        case ATL_DTYPE_UINT8: return MPI_UNSIGNED_CHAR;
        case ATL_DTYPE_INT16: return MPI_INT16_T;
        case ATL_DTYPE_UINT16: return MPI_UINT16_T;
        case ATL_DTYPE_INT32: return MPI_INT;
        case ATL_DTYPE_UINT32: return MPI_UINT32_T;
        case ATL_DTYPE_INT64: return MPI_LONG_LONG;
        case ATL_DTYPE_UINT64: return MPI_UNSIGNED_LONG_LONG;
        case ATL_DTYPE_FLOAT16: return MPI_FLOAT16;
        case ATL_DTYPE_FLOAT32: return MPI_FLOAT;
        case ATL_DTYPE_FLOAT64: return MPI_DOUBLE;
        case ATL_DTYPE_BFLOAT16: return MPI_BFLOAT16;
        default: printf("unknown datatype: %d\n", dtype); exit(1);
    }
}

#ifdef ATL_MPI_BF16
static MPI_Op atl2mpi_op_bf16(atl_reduction_t rtype) {
    switch (rtype) {
        case ATL_REDUCTION_SUM: return global_data.bf16.sum_op;
        case ATL_REDUCTION_PROD: return global_data.bf16.prod_op;
        case ATL_REDUCTION_MIN: return global_data.bf16.min_op;
        case ATL_REDUCTION_MAX: return global_data.bf16.max_op;
        default: printf("unknown reduction type: %d\n", rtype); exit(1);
    }
}
#endif /* ATL_MPI_BF16 */

#ifdef ATL_MPI_FP16
static MPI_Op atl2mpi_op_fp16(atl_reduction_t rtype) {
    switch (rtype) {
        case ATL_REDUCTION_SUM: return global_data.fp16.sum_op;
        case ATL_REDUCTION_PROD: return global_data.fp16.prod_op;
        case ATL_REDUCTION_MIN: return global_data.fp16.min_op;
        case ATL_REDUCTION_MAX: return global_data.fp16.max_op;
        default: printf("unknown reduction type: %d\n", rtype); exit(1);
    }
}
#endif /* ATL_MPI_FP16 */

static MPI_Op atl2mpi_op(atl_reduction_t rtype, MPI_Datatype dtype) {
#ifdef ATL_MPI_BF16
    if (dtype == global_data.bf16.dtype)
        return atl2mpi_op_bf16(rtype);
#endif /* ATL_MPI_BF16 */

#ifdef ATL_MPI_FP16
    if (dtype == global_data.fp16.dtype)
        return atl2mpi_op_fp16(rtype);
#endif /* ATL_MPI_FP16 */

    (void)dtype;
    switch (rtype) {
        case ATL_REDUCTION_SUM: return MPI_SUM;
        case ATL_REDUCTION_PROD: return MPI_PROD;
        case ATL_REDUCTION_MIN: return MPI_MIN;
        case ATL_REDUCTION_MAX: return MPI_MAX;
        default: printf("unknown reduction type: %d\n", rtype); exit(1);
    }
}

atl_mpi_lib_type_t atl_mpi_get_lib_type() {
    atl_mpi_lib_type_t lib_type = ATL_MPI_LIB_NONE;
    char mpi_version[MPI_MAX_LIBRARY_VERSION_STRING] = { 0 };
    int mpi_version_len = -1, i;
    atl_mpi_lib_info_t* final_info = NULL;

    /* request IMPI level append library kind into MPI_Get_library_version output */
    setenv("I_MPI_INFO_LIBRARY_KIND", "1", 0);

    /* can be called before MPI_Init */
    int ret = MPI_Get_library_version(mpi_version, &mpi_version_len);

    if ((ret != MPI_SUCCESS) || (mpi_version_len < 0) ||
        (mpi_version_len > MPI_MAX_LIBRARY_VERSION_STRING)) {
        LOG_WARN("can not retrieve MPI version, mpi_version_len ", mpi_version_len, ", ret", ret);
        return ATL_MPI_LIB_NONE;
    }

    /* remove trailing spaces at the end for more compact log */
    while (strlen(mpi_version) && isspace(mpi_version[strlen(mpi_version) - 1]))
        mpi_version[strlen(mpi_version) - 1] = '\0';

    LOG_INFO("MPI version: ", mpi_version);

    for (i = 0; i < MPI_LIB_INFO_MAX_COUNT; i++) {
        atl_mpi_lib_info_t* info = &(mpi_lib_infos[i]);

        CCL_THROW_IF_NOT(info->version_prefix_1, "empty version_prefix_1");
        CCL_THROW_IF_NOT(info->min_version_value >= 0, "unexpected minimal version");

        const char* version_substr = NULL;
        if ((version_substr = strstr(mpi_version, info->version_prefix_1))) {
            version_substr += strlen(info->version_prefix_1);
            LOG_DEBUG("version_substr: ", version_substr);

            if (info->version_prefix_2) {
                version_substr = strstr(version_substr, info->version_prefix_2);
                if (!version_substr) {
                    LOG_DEBUG("can't find version_prefix_2 ", info->version_prefix_2);
                    continue;
                }
                version_substr += strlen(info->version_prefix_2);
                LOG_DEBUG("version_substr: ", version_substr);
            }

            int version_value = (version_substr) ? atoi(version_substr) : -1;
            LOG_DEBUG("MPI numerical version: ", version_value);

            if (version_value < info->min_version_value) {
                LOG_WARN("loaded MPI doesn't match with expected version, "
                         "consider to switch to ",
                         info->version_prefix_1,
                         " ",
                         (info->version_prefix_2 ? info->version_prefix_2 : ""),
                         info->min_version_value,
                         " (min) ",
                         (info->kind_value ? info->kind_value : ""),
                         "\n");
                continue;
            }
            else if (info->kind_prefix && info->kind_value) {
                const char* kind_substr = mpi_version;

                /* skip WARNING for IMPI which has high enough version but doesn't support library kind yet */
                if ((kind_substr = strstr(kind_substr, info->kind_prefix))) {
                    kind_substr += strlen(info->kind_prefix);
                    while ((isspace(*kind_substr)) &&
                           (kind_substr < (mpi_version + mpi_version_len)))
                        kind_substr++;

                    LOG_DEBUG("kind_substr: ", kind_substr);

                    if (strncmp(kind_substr, info->kind_value, strlen(info->kind_value))) {
                        LOG_WARN("loaded MPI version (",
                                 version_value,
                                 ") ",
                                 "is higher or equal to minimal expected version (",
                                 info->min_version_value,
                                 ") ",
                                 "but kind (",
                                 kind_substr,
                                 ") doesn't match with expected kind (",
                                 info->kind_value,
                                 "), "
                                 "consider to switch to ",
                                 info->version_prefix_1,
                                 " ",
                                 (info->version_prefix_2 ? info->version_prefix_2 : ""),
                                 info->min_version_value,
                                 " (min version) ",
                                 (info->kind_value ? info->kind_value : ""),
                                 "\n");
                        continue;
                    }
                }

                final_info = info;
                LOG_DEBUG("set lib_type = ",
                          info->name,
                          " because "
                          "version (",
                          version_value,
                          ") is higher or equal to minimal expected version (",
                          info->min_version_value,
                          ") "
                          "and kind matches with expected kind");
                break;
            }
        }
    }

    /* user input has higher priority */
    char* lib_type_env = NULL;
    if ((lib_type_env = getenv("CCL_ATL_MPI_LIB_TYPE")) != NULL) {
        final_info = NULL;
        for (i = 0; i < MPI_LIB_INFO_MAX_COUNT; i++) {
            atl_mpi_lib_info_t* info = &(mpi_lib_infos[i]);

            if (!strcmp(lib_type_env, info->name)) {
                final_info = info;
                LOG_DEBUG("set lib_type = ", lib_type_env, " because it is requested explicitly");
                break;
            }
        }
    }

    if (final_info) {
        LOG_INFO("MPI library type: ", final_info->name);
        lib_type = final_info->type;
    }
    else {
        LOG_INFO("MPI library type: none");
        lib_type = ATL_MPI_LIB_NONE;
    }

    return lib_type;
}

size_t atl_mpi_get_ep_count(const atl_attr_t& attr) {
    size_t mpi_ep_count = attr.ep_count;
    if (attr.extra_ep)
        mpi_ep_count += attr.extra_ep;
    return mpi_ep_count;
}

atl_status_t atl_mpi_set_base_env(const atl_attr_t& attr) {
    setenv("PSM2_MULTI_EP", "1", 0);
    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_set_impi_env(const atl_attr_t& attr) {
    char ep_count_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(ep_count_str, MPI_MAX_INFO_VAL, "%zu", atl_mpi_get_ep_count(attr));

    setenv("MPIR_CVAR_DEFAULT_THREAD_LEVEL", "MPI_THREAD_MULTIPLE", 0);
    setenv("I_MPI_THREAD_SPLIT", "1", 0);
    setenv("I_MPI_THREAD_RUNTIME", "generic", 0);
    setenv("I_MPI_THREAD_MAX", ep_count_str, 0);
    setenv("I_MPI_THREAD_ID_KEY", EP_IDX_KEY, 0);
    setenv("I_MPI_THREAD_LOCK_LEVEL", "vci", 0);

    if (attr.enable_shm)
        setenv("I_MPI_FABRICS", "shm:ofi", 0);
    else
        setenv("I_MPI_FABRICS", "ofi", 0);

    if (attr.ep_count)
        setenv("I_MPI_OFI_ISEND_INJECT_THRESHOLD", "0", 0);

    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_set_mpich_env(const atl_attr_t& attr) {
    char ep_count_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(ep_count_str, MPI_MAX_INFO_VAL, "%zu", atl_mpi_get_ep_count(attr));

    setenv("MPIR_CVAR_CH4_MT_MODEL", "direct", 0);
    setenv("MPIR_CVAR_CH4_OFI_MAX_VCIS", ep_count_str, 0);
    setenv("MPIR_COMM_HINT_ASYNC_PROG_ID", EP_IDX_KEY, 0);

    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_set_env(const atl_attr_t& attr) {
    if (global_data.mpi_lib_type != ATL_MPI_LIB_NONE) {
        /* env was already set */
        return ATL_STATUS_SUCCESS;
    }

    atl_mpi_lib_type_t type = atl_mpi_get_lib_type();

    if (type == ATL_MPI_LIB_NONE) {
        /* nothing to do */
        return ATL_STATUS_SUCCESS;
    }

    atl_mpi_set_base_env(attr);

    if (type == ATL_MPI_LIB_IMPI) {
        if (!getenv("I_MPI_ROOT")) {
            LOG_ERROR("CCL/MPI uses ",
                      mpi_lib_infos[type].version_prefix_1,
                      " but I_MPI_ROOT is not set. ",
                      "Please source ",
                      mpi_lib_infos[type].kind_value,
                      " version of ",
                      mpi_lib_infos[type].version_prefix_1,
                      " (",
                      mpi_lib_infos[type].min_version_value,
                      " or higher version).");
            return ATL_STATUS_FAILURE;
        }
        atl_mpi_set_impi_env(attr);
    }
    else if (type == ATL_MPI_LIB_MPICH) {
        atl_mpi_set_mpich_env(attr);
    }

    int is_mpi_inited = 0;
    MPI_Initialized(&is_mpi_inited);
    if (is_mpi_inited) {
        LOG_WARN("MPI was initialized externally, CCL/MPI specific enviroment is ignored");
    }
    else {
        LOG_INFO("set CCL/MPI specific enviroment");
    }

    global_data.mpi_lib_type = type;

    return ATL_STATUS_SUCCESS;
}

size_t atl_mpi_get_nic_count() {
    if (global_data.mpi_lib_type != ATL_MPI_LIB_MPICH)
        return 1;

    int flag, count;
    MPI_Info info;
    char nic_count_str[MPI_MAX_INFO_VAL] = { 0 };

    MPI_Comm_get_info(MPI_COMM_WORLD, &info);
    MPI_Info_get(info, NIC_COUNT_KEY, MPI_MAX_INFO_VAL, nic_count_str, &flag);

    if (!flag) {
        LOG_WARN(NIC_COUNT_KEY, " hint does not exist, using nic_count = 1");
        count = 1;
    }
    else {
        count = atoi(nic_count_str);
        if (count <= 0)
            count = 1;
    }

    MPI_Info_free(&info);
    return count;
}

void atl_mpi_check_hint(MPI_Comm comm, const char* hint, const char* expected_value) {
    int flag;
    MPI_Info info;
    char read_value[MPI_MAX_INFO_VAL] = { 0 };

    MPI_Comm_get_info(comm, &info);
    MPI_Info_get(info, hint, MPI_MAX_INFO_VAL, read_value, &flag);
    MPI_Info_free(&info);

    CCL_THROW_IF_NOT(flag, "MPI comm hint ", hint, " was not set");
    CCL_THROW_IF_NOT(!strcmp(read_value, expected_value),
                     "MPI comm hint ",
                     hint,
                     ": expected: ",
                     expected_value,
                     ", read: ",
                     read_value);
}

void atl_mpi_check_ep_idx_hint(MPI_Comm comm, size_t idx) {
    if (global_data.mpi_lib_type == ATL_MPI_LIB_NONE)
        return;

    char idx_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(idx_str, MPI_MAX_INFO_VAL, "%zu", idx);
    atl_mpi_check_hint(comm, EP_IDX_KEY, idx_str);
}

void atl_mpi_check_nic_idx_hint(MPI_Comm comm, size_t idx) {
    if (global_data.mpi_lib_type != ATL_MPI_LIB_MPICH)
        return;

    char idx_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(idx_str, MPI_MAX_INFO_VAL, "%zu", idx);
    atl_mpi_check_hint(comm, NIC_IDX_KEY, idx_str);
}

#ifdef ENABLE_DEBUG
inline void atl_mpi_check_ep(atl_ep_t* ep) {
    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_check_ep_idx_hint(mpi_ep->mpi_comm, ep->idx);
}
#else
#define atl_mpi_check_ep(ep)
#endif

static atl_status_t atl_mpi_finalize(atl_ctx_t* ctx) {
    int ret = MPI_SUCCESS;

    atl_mpi_ctx_t* mpi_ctx = container_of(ctx, atl_mpi_ctx_t, ctx);

    atl_ep_t** eps = ctx->eps;

    int is_mpi_finalized = 0;
    MPI_Finalized(&is_mpi_finalized);

    if (!is_mpi_finalized) {
        for (size_t i = 0; i < ctx->ep_count; i++) {
            atl_mpi_ep_t* mpi_ep = container_of(eps[i], atl_mpi_ep_t, ep);

            if (mpi_ep) {
                if (global_data.progress_mode == ATL_PROGRESS_POLL) {
                    MPI_Cancel(&(mpi_ep->dummy_req.native_req));
                    MPI_Comm_free(&mpi_ep->dummy_comm);
                }
                MPI_Comm_free(&mpi_ep->mpi_comm);
                free(mpi_ep);
            }
        }

        atl_mpi_bf16_finalize();
        atl_mpi_fp16_finalize();

        if (!global_data.is_external_init) {
            ret = MPI_Finalize();
        }
        else
            LOG_DEBUG("MPI_Init has been called externally, skip MPI_Finalize");
    }
    else
        LOG_DEBUG("MPI_Finalize has been already called");

    free(eps);
    free(mpi_ctx);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_mr_reg(atl_ctx_t* ctx, const void* buf, size_t len, atl_mr_t** mr) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_mpi_mr_dereg(atl_ctx_t* ctx, atl_mr_t* mr) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_mpi_ep_send(atl_ep_t* ep,
                                    const void* buf,
                                    size_t len,
                                    int dst_proc_idx,
                                    uint64_t tag,
                                    atl_req_t* req) {
    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Isend(
        buf, len, MPI_CHAR, dst_proc_idx, (int)tag, mpi_ep->mpi_comm, &mpi_req->native_req);

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_recv(atl_ep_t* ep,
                                    void* buf,
                                    size_t len,
                                    int src_proc_idx,
                                    uint64_t tag,
                                    atl_req_t* req) {
    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Irecv(
        buf, len, MPI_CHAR, src_proc_idx, (int)tag, mpi_ep->mpi_comm, &mpi_req->native_req);

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_probe(atl_ep_t* ep,
                                     int src_proc_idx,
                                     uint64_t tag,
                                     int* found,
                                     size_t* recv_len) {
    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);

    int flag = 0, len = 0, ret;
    MPI_Status status;

    ret = MPI_Iprobe(src_proc_idx, tag, mpi_ep->mpi_comm, &flag, &status);
    if (flag) {
        MPI_Get_count(&status, MPI_BYTE, &len);
    }

    if (found)
        *found = flag;
    if (recv_len)
        *recv_len = len;

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_allgatherv(atl_ep_t* ep,
                                          const void* send_buf,
                                          size_t send_len,
                                          void* recv_buf,
                                          const int* recv_lens,
                                          const int* offsets,
                                          atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    if (global_data.sync_coll) {
        ret = MPI_Allgatherv((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                             send_len,
                             MPI_CHAR,
                             recv_buf,
                             recv_lens,
                             offsets,
                             MPI_CHAR,
                             mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Iallgatherv((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                              send_len,
                              MPI_CHAR,
                              recv_buf,
                              recv_lens,
                              offsets,
                              MPI_CHAR,
                              mpi_ep->mpi_comm,
                              &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_allreduce(atl_ep_t* ep,
                                         const void* send_buf,
                                         void* recv_buf,
                                         size_t count,
                                         atl_datatype_t dtype,
                                         atl_reduction_t op,
                                         atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op, mpi_dtype);

    if (global_data.sync_coll) {
        ret = MPI_Allreduce((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                            recv_buf,
                            count,
                            mpi_dtype,
                            mpi_op,
                            mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Iallreduce((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                             recv_buf,
                             count,
                             mpi_dtype,
                             mpi_op,
                             mpi_ep->mpi_comm,
                             &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_alltoall(atl_ep_t* ep,
                                        const void* send_buf,
                                        void* recv_buf,
                                        size_t len,
                                        atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    if (global_data.sync_coll) {
        ret = MPI_Alltoall((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                           len,
                           MPI_CHAR,
                           recv_buf,
                           len,
                           MPI_CHAR,
                           mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ialltoall((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                            len,
                            MPI_CHAR,
                            recv_buf,
                            len,
                            MPI_CHAR,
                            mpi_ep->mpi_comm,
                            &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_alltoallv(atl_ep_t* ep,
                                         const void* send_buf,
                                         const int* send_lens,
                                         const int* send_offsets,
                                         void* recv_buf,
                                         const int* recv_lens,
                                         const int* recv_offsets,
                                         atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    if (global_data.sync_coll) {
        ret = MPI_Alltoallv((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                            send_lens,
                            send_offsets,
                            MPI_CHAR,
                            recv_buf,
                            recv_lens,
                            recv_offsets,
                            MPI_CHAR,
                            mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ialltoallv((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                             send_lens,
                             send_offsets,
                             MPI_CHAR,
                             recv_buf,
                             recv_lens,
                             recv_offsets,
                             MPI_CHAR,
                             mpi_ep->mpi_comm,
                             &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_barrier(atl_ep_t* ep, atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    if (global_data.sync_coll) {
        ret = MPI_Barrier(mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ibarrier(mpi_ep->mpi_comm, &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_bcast(atl_ep_t* ep,
                                     void* buf,
                                     size_t len,
                                     int root,
                                     atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    if (global_data.sync_coll) {
        ret = MPI_Bcast(buf, len, MPI_CHAR, root, mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ibcast(buf, len, MPI_CHAR, root, mpi_ep->mpi_comm, &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_reduce(atl_ep_t* ep,
                                      const void* send_buf,
                                      void* recv_buf,
                                      size_t count,
                                      int root,
                                      atl_datatype_t dtype,
                                      atl_reduction_t op,
                                      atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    int my_proc_idx = ep->ctx->coord.global_idx;
    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op, mpi_dtype);

    if (global_data.sync_coll) {
        ret = MPI_Reduce(
            (send_buf && (send_buf == recv_buf) && (root == my_proc_idx)) ? MPI_IN_PLACE : send_buf,
            recv_buf,
            count,
            mpi_dtype,
            mpi_op,
            root,
            mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ireduce(
            (send_buf && (send_buf == recv_buf) && (root == my_proc_idx)) ? MPI_IN_PLACE : send_buf,
            recv_buf,
            count,
            mpi_dtype,
            mpi_op,
            root,
            mpi_ep->mpi_comm,
            &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_reduce_scatter(atl_ep_t* ep,
                                              const void* send_buf,
                                              void* recv_buf,
                                              size_t recv_count,
                                              atl_datatype_t dtype,
                                              atl_reduction_t op,
                                              atl_req_t* req) {
    int ret = MPI_SUCCESS;

    atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op, mpi_dtype);

    if (global_data.sync_coll) {
        ret =
            MPI_Reduce_scatter_block((send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
                                     recv_buf,
                                     recv_count,
                                     mpi_dtype,
                                     mpi_op,
                                     mpi_ep->mpi_comm);
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
        mpi_req->native_req = MPI_REQUEST_NULL;
    }
    else {
        ret = MPI_Ireduce_scatter_block(
            (send_buf && (send_buf == recv_buf)) ? MPI_IN_PLACE : send_buf,
            recv_buf,
            recv_count,
            mpi_dtype,
            mpi_op,
            mpi_ep->mpi_comm,
            &mpi_req->native_req);
        mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    }

    atl_mpi_check_ep(ep);

    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_read(atl_ep_t* ep,
                                    void* buf,
                                    size_t len,
                                    atl_mr_t* mr,
                                    uint64_t addr,
                                    uintptr_t r_key,
                                    int dst_proc_idx,
                                    atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_mpi_ep_write(atl_ep_t* ep,
                                     const void* buf,
                                     size_t len,
                                     atl_mr_t* mr,
                                     uint64_t addr,
                                     uintptr_t r_key,
                                     int dst_proc_idx,
                                     atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_mpi_ep_wait(atl_ep_t* ep, atl_req_t* req) {
    int ret;
    MPI_Status status;
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    ret = MPI_Wait(&mpi_req->native_req, &status);
    mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
    return RET2ATL(ret);
}

static atl_status_t atl_mpi_ep_wait_all(atl_ep_t* ep, atl_req_t* reqs, size_t count) {
    return ATL_STATUS_UNSUPPORTED;
}

static inline atl_status_t atl_mpi_ep_progress(atl_ep_t* ep, atl_mpi_req_t* req) {
    int flag = 0;
    int ret = MPI_Test(&req->native_req, &flag, MPI_STATUS_IGNORE);

    if (flag) {
        req->comp_state = ATL_MPI_COMP_COMPLETED;
    }

    return RET2ATL(ret);
}

static inline atl_status_t atl_mpi_ep_poll(atl_ep_t* ep) {
    if (global_data.progress_mode == ATL_PROGRESS_POLL) {
        atl_mpi_ep_t* mpi_ep = container_of(ep, atl_mpi_ep_t, ep);
        atl_mpi_ep_progress(ep, &(mpi_ep->dummy_req));
    }

    return ATL_STATUS_SUCCESS;
}

static atl_status_t atl_mpi_ep_check(atl_ep_t* ep, int* is_completed, atl_req_t* req) {
    CCL_THROW_IF_NOT(is_completed);

    atl_status_t status = ATL_STATUS_SUCCESS;

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);

    *is_completed = (mpi_req->comp_state == ATL_MPI_COMP_COMPLETED);
    if (*is_completed) {
        return ATL_STATUS_SUCCESS;
    }

    status = atl_mpi_ep_progress(ep, mpi_req);
    *is_completed = (mpi_req->comp_state == ATL_MPI_COMP_COMPLETED);

    return status;
}

static atl_ops_t atl_mpi_ops = {
    .finalize = atl_mpi_finalize,
};

static atl_mr_ops_t atl_mpi_mr_ops = {
    .mr_reg = atl_mpi_mr_reg,
    .mr_dereg = atl_mpi_mr_dereg,
};

static atl_p2p_ops_t atl_mpi_ep_p2p_ops = {
    .send = atl_mpi_ep_send,
    .recv = atl_mpi_ep_recv,
    .probe = atl_mpi_ep_probe,
};

static atl_coll_ops_t atl_mpi_ep_coll_ops = { .allgatherv = atl_mpi_ep_allgatherv,
                                              .allreduce = atl_mpi_ep_allreduce,
                                              .alltoall = atl_mpi_ep_alltoall,
                                              .alltoallv = atl_mpi_ep_alltoallv,
                                              .barrier = atl_mpi_ep_barrier,
                                              .bcast = atl_mpi_ep_bcast,
                                              .reduce = atl_mpi_ep_reduce,
                                              .reduce_scatter = atl_mpi_ep_reduce_scatter };

static atl_rma_ops_t atl_mpi_ep_rma_ops = {
    .read = atl_mpi_ep_read,
    .write = atl_mpi_ep_write,
};

static atl_comp_ops_t atl_mpi_ep_comp_ops = { .wait = atl_mpi_ep_wait,
                                              .wait_all = atl_mpi_ep_wait_all,
                                              .cancel = NULL,
                                              .poll = atl_mpi_ep_poll,
                                              .check = atl_mpi_ep_check };

static atl_status_t atl_mpi_ep_init(atl_mpi_ctx_t* mpi_ctx, size_t idx, atl_ep_t** ep) {
    int ret;
    ssize_t mpi_ep_idx = idx;
    size_t nic_idx = (idx % global_data.nic_count);

    char nic_idx_str[MPI_MAX_INFO_VAL] = { 0 };
    char mpi_ep_idx_str[MPI_MAX_INFO_VAL] = { 0 };

    atl_mpi_ep_t* mpi_ep = (atl_mpi_ep_t*)calloc(1, sizeof(atl_mpi_ep_t));
    if (!mpi_ep)
        return ATL_STATUS_FAILURE;

    ret = MPI_Comm_dup(MPI_COMM_WORLD, &mpi_ep->mpi_comm);
    if (ret)
        goto err_ep_dup;

    MPI_Info info;
    MPI_Info_create(&info);

    /* set NIC index */
    snprintf(nic_idx_str, MPI_MAX_INFO_VAL, "%zu", nic_idx);
    MPI_Info_set(info, NIC_IDX_KEY, nic_idx_str);

    /* set EP index */
    if (global_data.extra_ep)
        mpi_ep_idx += global_data.extra_ep;
    snprintf(mpi_ep_idx_str, MPI_MAX_INFO_VAL, "%zu", mpi_ep_idx);
    MPI_Info_set(info, EP_IDX_KEY, mpi_ep_idx_str);

    MPI_Comm_set_info(mpi_ep->mpi_comm, info);

    if (global_data.progress_mode == ATL_PROGRESS_POLL) {
        ret = MPI_Comm_dup(MPI_COMM_WORLD, &mpi_ep->dummy_comm);
        if (ret)
            goto err_ep_dup;
        MPI_Comm_set_info(mpi_ep->dummy_comm, info);
        MPI_Irecv(NULL, 0, MPI_CHAR, 0, 0, mpi_ep->dummy_comm, &(mpi_ep->dummy_req.native_req));

        atl_mpi_check_nic_idx_hint(mpi_ep->dummy_comm, nic_idx);
        atl_mpi_check_ep_idx_hint(mpi_ep->dummy_comm, mpi_ep_idx);
    }

    MPI_Info_free(&info);

    atl_mpi_check_nic_idx_hint(mpi_ep->mpi_comm, nic_idx);
    atl_mpi_check_ep_idx_hint(mpi_ep->mpi_comm, mpi_ep_idx);

    LOG_DEBUG("atl_ep_idx ", idx, ", mpi_ep_idx ", mpi_ep_idx, ", nic_idx ", nic_idx);

    *ep = &mpi_ep->ep;
    (*ep)->idx = idx;
    (*ep)->ctx = &mpi_ctx->ctx;
    (*ep)->p2p_ops = &atl_mpi_ep_p2p_ops;
    (*ep)->coll_ops = &atl_mpi_ep_coll_ops;
    (*ep)->rma_ops = &atl_mpi_ep_rma_ops;
    (*ep)->comp_ops = &atl_mpi_ep_comp_ops;

    return ATL_STATUS_SUCCESS;

err_ep_dup:
    free(mpi_ep);
    return RET2ATL(ret);
}

static atl_status_t atl_mpi_init(int* argc,
                                 char*** argv,
                                 atl_attr_t* attr,
                                 atl_ctx_t** out_ctx,
                                 const char* main_addr) {
    CCL_THROW_IF_NOT((sizeof(atl_mpi_req_t) <= sizeof(atl_req_t) - offsetof(atl_req_t, internal)),
                     "unexpected offset: atl_mpi_request size ",
                     sizeof(atl_mpi_req_t),
                     ", atl_request size ",
                     sizeof(atl_req_t),
                     ", expected offset ",
                     offsetof(atl_req_t, internal));

    int ret = MPI_SUCCESS;
    size_t i;
    int is_tag_ub_set = 0;
    void* tag_ub_ptr = NULL;
    int required_thread_level = MPI_THREAD_MULTIPLE, provided_thread_level;
    int is_mpi_inited = 0;

    atl_mpi_ctx_t* mpi_ctx = (atl_mpi_ctx_t*)calloc(1, sizeof(atl_mpi_ctx_t));
    if (!mpi_ctx)
        return ATL_STATUS_FAILURE;

    atl_ctx_t* ctx = &(mpi_ctx->ctx);

    if (atl_mpi_set_env(*attr)) {
        goto err_init;
    }

    global_data.sync_coll = attr->sync_coll;
    global_data.extra_ep = attr->extra_ep;
    global_data.nic_count = atl_mpi_get_nic_count();

    MPI_Initialized(&is_mpi_inited);

    if (!is_mpi_inited) {
        global_data.is_external_init = 0;
        ret = MPI_Init_thread(argc, argv, required_thread_level, &provided_thread_level);
        if (provided_thread_level < required_thread_level) {
            LOG_ERROR("unexpected MPI thread level: requested ",
                      required_thread_level,
                      ", provided ",
                      provided_thread_level);
            goto err_init;
        }
    }
    else {
        global_data.is_external_init = 1;
        LOG_DEBUG("MPI was initialized externaly");
        MPI_Query_thread(&provided_thread_level);
        if (provided_thread_level < required_thread_level) {
            LOG_ERROR("MPI was initialized externaly but with unexpected thread level: "
                      "requested ",
                      required_thread_level,
                      ", provided ",
                      provided_thread_level);
            goto err_init;
        }
    }

    if (ret)
        goto err_init;

    if (atl_mpi_bf16_init() == ATL_STATUS_FAILURE) {
        atl_mpi_bf16_finalize();
        goto err_init;
    }

    if (atl_mpi_fp16_init() == ATL_STATUS_FAILURE) {
        atl_mpi_fp16_finalize();
        goto err_init;
    }

    atl_proc_coord_t* coord;
    coord = &(ctx->coord);

    MPI_Comm_rank(MPI_COMM_WORLD, (int*)&(coord->global_idx));
    MPI_Comm_size(MPI_COMM_WORLD, (int*)&(coord->global_count));

    MPI_Comm local_comm;
    MPI_Comm_split_type(
        MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, coord->global_count, MPI_INFO_NULL, &local_comm);
    MPI_Comm_rank(local_comm, (int*)&(coord->local_idx));
    MPI_Comm_size(local_comm, (int*)&(coord->local_count));
    MPI_Comm_free(&local_comm);

    ctx->ops = &atl_mpi_ops;
    ctx->mr_ops = &atl_mpi_mr_ops;
    ctx->ep_count = attr->ep_count;
    ctx->eps = (atl_ep_t**)calloc(1, sizeof(void*) * attr->ep_count);
    if (!ctx->eps)
        goto err_after_init;
    ctx->is_resize_enabled = 0;

    global_data.progress_mode = ATL_PROGRESS_CHECK;

    char* progress_mode_env;
    progress_mode_env = getenv(ATL_PROGRESS_MODE_ENV);
    if (progress_mode_env) {
        global_data.progress_mode = (atl_progress_mode_t)atoi(progress_mode_env);
    }

    if (coord->global_idx == 0) {
        LOG_INFO("progress_mode ", global_data.progress_mode);
        LOG_INFO("sync_coll ", global_data.sync_coll);
        LOG_INFO("extra_ep ", global_data.extra_ep);
        LOG_INFO("nic_count ", global_data.nic_count);
    }

    for (i = 0; i < attr->ep_count; i++) {
        ret = atl_mpi_ep_init(mpi_ctx, i, &(ctx->eps[i]));
        if (ret)
            goto err_ep_dup;
    }

    *out_ctx = &mpi_ctx->ctx;

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &tag_ub_ptr, &is_tag_ub_set);

    attr->tag_bits = 32;
    attr->max_tag = (is_tag_ub_set) ? *((int*)tag_ub_ptr) : 0;
    attr->enable_rma = 0;
    attr->max_order_waw_size = 0;

    return ATL_STATUS_SUCCESS;

err_ep_dup:
    for (i = 0; i < attr->ep_count; i++) {
        atl_mpi_ep_t* mpi_ep = container_of(ctx->eps[i], atl_mpi_ep_t, ep);

        if (ctx->eps[i] && mpi_ep) {
            if (global_data.progress_mode == ATL_PROGRESS_POLL) {
                MPI_Cancel(&(mpi_ep->dummy_req.native_req));
                MPI_Comm_free(&mpi_ep->dummy_comm);
            }
            MPI_Comm_free(&mpi_ep->mpi_comm);
        }
    }
    free(ctx->eps);

err_after_init:
    atl_mpi_bf16_finalize();
    atl_mpi_fp16_finalize();
    if (!global_data.is_external_init) {
        MPI_Finalize();
    }

err_init:
    free(mpi_ctx);
    return ATL_STATUS_FAILURE;
}

atl_status_t atl_mpi_main_addr_reserve(char* main_addr) {
    return ATL_STATUS_UNSUPPORTED;
}

ATL_MPI_INI {
    atl_transport->name = "mpi";
    atl_transport->init = atl_mpi_init;
    atl_transport->reserve_addr = atl_mpi_main_addr_reserve;
    return ATL_STATUS_SUCCESS;
}
#ifdef __cplusplus
}
#endif