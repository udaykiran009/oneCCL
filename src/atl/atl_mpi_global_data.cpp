#ifdef CCL_ENABLE_MPI

#include "atl_mpi_global_data.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"

void check_op_params(void* in_buf,
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

#ifdef ATL_MPI_FP16

void FP16_INLINE_TARGET_ATTRIBUTE_ALL fp16_base_op(void* in,
                                                   void* inout,
                                                   int* length,
                                                   ccl::reduction op) {
    unsigned short* in_buf = (unsigned short*)in;
    unsigned short* inout_buf = (unsigned short*)inout;

    size_t len = *length;
    ccl_fp16_reduce_impl(in_buf, inout_buf, len, op);
}

void FP16_TARGET_ATTRIBUTE_ALL fp16_sum_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    fp16_base_op(in, inout, length, ccl::reduction::sum);
}

void FP16_TARGET_ATTRIBUTE_ALL fp16_prod_op(void* in,
                                            void* inout,
                                            int* length,
                                            MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    fp16_base_op(in, inout, length, ccl::reduction::prod);
}

void FP16_TARGET_ATTRIBUTE_ALL fp16_min_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    fp16_base_op(in, inout, length, ccl::reduction::min);
}

void FP16_TARGET_ATTRIBUTE_ALL fp16_max_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    fp16_base_op(in, inout, length, ccl::reduction::max);
}
#endif // ATL_MPI_FP16

#ifdef ATL_MPI_BF16

void BF16_INLINE_TARGET_ATTRIBUTE_ALL bf16_base_op(void* in,
                                                   void* inout,
                                                   int* length,
                                                   ccl::reduction op) {
    unsigned short* in_buf = (unsigned short*)in;
    unsigned short* inout_buf = (unsigned short*)inout;

    size_t len = *length;
    ccl_bf16_reduce_impl(in_buf, inout_buf, len, op);
}

void BF16_TARGET_ATTRIBUTE_ALL bf16_sum_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    bf16_base_op(in, inout, length, ccl::reduction::sum);
}

void BF16_TARGET_ATTRIBUTE_ALL bf16_prod_op(void* in,
                                            void* inout,
                                            int* length,
                                            MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    bf16_base_op(in, inout, length, ccl::reduction::prod);
}

void BF16_TARGET_ATTRIBUTE_ALL bf16_min_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    bf16_base_op(in, inout, length, ccl::reduction::min);
}

void BF16_TARGET_ATTRIBUTE_ALL bf16_max_op(void* in,
                                           void* inout,
                                           int* length,
                                           MPI_Datatype* datatype) {
    check_op_params(in, inout, length, datatype, __FUNCTION__);
    bf16_base_op(in, inout, length, ccl::reduction::max);
}
#endif // ATL_MPI_BF16

void atl_mpi_global_data::print_error(int error) {
    char str_error[MPI_MAX_ERROR_STRING];
    int result_len = MPI_MAX_ERROR_STRING;

    MPI_Error_string(error, str_error, &result_len);

    if (result_len > MPI_MAX_ERROR_STRING) {
        result_len = MPI_MAX_ERROR_STRING;
    }
    str_error[result_len - 1] = '\0';

    ccl_logger::format(std::cout, "MPI error: %s (%d)", str_error, error);
}

atl_status_t atl_mpi_global_data::set_impi_env(const atl_attr_t& attr,
                                               const atl_mpi_lib_attr_t& lib_attr) {
    char ep_count_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(ep_count_str, MPI_MAX_INFO_VAL, "%zu", get_ep_count(attr));

    if (attr.in.ep_count)
        setenv("I_MPI_OFI_ISEND_INJECT_THRESHOLD", "0", 0);

#ifdef CCL_ENABLE_SYCL
    setenv("I_MPI_SHM_CMA", "0", 0);
    if (attr.in.enable_hmem && lib_attr.hmem) {
        setenv("I_MPI_OFFLOAD", "2", 0);
        setenv("I_MPI_OFFLOAD_TOPOLIB", "l0", 0);
        setenv("I_MPI_OFFLOAD_QUEUE_CACHE", "1", 0);
        setenv("I_MPI_OFFLOAD_LIST_CACHE", "1", 0);
        setenv("I_MPI_OFFLOAD_MEMCPY_KIND", "blocked", 0);
        if (attr.in.ep_count > 1) {
            /* try to set global lock level before vci level
               because setenv is invoked with overwrite=0 */
            setenv("I_MPI_THREAD_LOCK_LEVEL", "global", 0);
        }
    }
#endif // CCL_ENABLE_SYCL

    setenv("I_MPI_THREAD_SPLIT", "1", 0);
    setenv("I_MPI_THREAD_RUNTIME", "generic", 0);
    setenv("I_MPI_THREAD_MAX", ep_count_str, 0);
    setenv("I_MPI_THREAD_ID_KEY", EP_IDX_KEY, 0);
    setenv("I_MPI_THREAD_LOCK_LEVEL", "vci", 0);

    return ATL_STATUS_SUCCESS;
}

size_t atl_mpi_global_data::get_ep_count(const atl_attr_t& attr) {
    size_t mpi_ep_count = attr.in.ep_count;
    if (attr.in.enable_extra_ep)
        mpi_ep_count += attr.in.enable_extra_ep;
    return mpi_ep_count;
}

atl_mpi_global_data::atl_mpi_lib_attr_t atl_mpi_global_data::get_lib_attr() {
    atl_mpi_lib_attr_t lib_attr = { ATL_MPI_LIB_NONE, 0 };

    char mpi_version[MPI_MAX_LIBRARY_VERSION_STRING] = { 0 };
    int mpi_version_len = -1, i;
    const atl_mpi_lib_info_t* final_info = NULL;

    /* can be called before MPI_Init */
    int ret = MPI_Get_library_version(mpi_version, &mpi_version_len);

    if ((ret != MPI_SUCCESS) || (mpi_version_len < 0) ||
        (mpi_version_len > MPI_MAX_LIBRARY_VERSION_STRING)) {
        LOG_WARN("can not retrieve MPI version, mpi_version_len ", mpi_version_len, ", ret", ret);
        return lib_attr;
    }

    /* remove trailing spaces at the end for more compact log */
    while (strlen(mpi_version) && isspace(mpi_version[strlen(mpi_version) - 1]))
        mpi_version[strlen(mpi_version) - 1] = '\0';

    LOG_DEBUG("MPI version: ", mpi_version);

    /* for filtering */
    char* lib_type_env = getenv("CCL_ATL_MPI");

    for (i = 0; i < MPI_LIB_INFO_MAX_COUNT; i++) {
        const atl_mpi_lib_info_t* info = &(mpi_lib_infos[i]);

        if (info->type == ATL_MPI_LIB_NONE)
            continue;

        if (lib_type_env) {
            if (strcmp(lib_type_env, info->name)) {
                LOG_DEBUG("library ", info->name, " is filtered out by user input ", lib_type_env);
                continue;
            }
            else {
                LOG_DEBUG("use lib_type = ", lib_type_env, " because it is requested explicitly");
            }
        }

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

            if (info->kind_prefix && info->kind_value) {
                const char* kind_substr = mpi_version;

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
                    }
                }
                else {
                    LOG_DEBUG("MPI version is high enough, but kind_prefix (",
                              info->kind_prefix,
                              ") can not be found",
                              " treat this like expected kind (",
                              info->kind_value,
                              ") was found");
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
                      ")");

            lib_attr.type = final_info->type;
            lib_attr.hmem = (final_info->min_hmem_version_value >= version_value) ? 1 : 0;

            break;
        }
    }

    if (final_info) {
        LOG_DEBUG("MPI library type: ", final_info->name);
    }
    else {
        LOG_DEBUG("MPI library type: none");
    }

    return lib_attr;
}

int atl_mpi_global_data::bf16_init() {
    if (ccl::global_data::env().bf16_impl_type <= ccl_bf16_no_hardware_support) {
        return ATL_STATUS_SUCCESS;
    }

#ifdef ATL_MPI_BF16

    int ret = MPI_SUCCESS;
    // create custom MPI BF16 dtype
    ret = MPI_Type_contiguous(2, MPI_BYTE, &bf16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 dtype");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    ret = MPI_Type_commit(&bf16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot commit MPI BF16 type");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI BF16 summation op
    ret = MPI_Op_create(&bf16_sum_op, 1, &bf16.sum_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 sum op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI BF16 production op
    ret = MPI_Op_create(&bf16_prod_op, 1, &bf16.prod_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 prod op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI BF16 min op
    ret = MPI_Op_create(&bf16_min_op, 1, &bf16.min_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 min op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI BF16 max op
    ret = MPI_Op_create(&bf16_max_op, 1, &bf16.max_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI BF16 max op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

#endif // ATL_MPI_BF16

    return ATL_STATUS_SUCCESS;
}

void atl_mpi_global_data::bf16_finalize() {
    if (bf16.dtype != MPI_DATATYPE_NULL) {
        MPI_Type_free(&bf16.dtype);
    }

    if (bf16.sum_op != MPI_OP_NULL) {
        MPI_Op_free(&bf16.sum_op);
    }

    if (bf16.prod_op != MPI_OP_NULL) {
        MPI_Op_free(&bf16.prod_op);
    }

    if (bf16.min_op != MPI_OP_NULL) {
        MPI_Op_free(&bf16.min_op);
    }

    if (bf16.max_op != MPI_OP_NULL) {
        MPI_Op_free(&bf16.max_op);
    }
}

int atl_mpi_global_data::fp16_init() {
    if (ccl::global_data::env().fp16_impl_type <= ccl_fp16_no_hardware_support) {
        return ATL_STATUS_SUCCESS;
    }

#ifdef ATL_MPI_FP16

    int ret = MPI_SUCCESS;

    // create custom MPI FP16 dtype
    ret = MPI_Type_contiguous(2, MPI_BYTE, &fp16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 dtype");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    ret = MPI_Type_commit(&fp16.dtype);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot commit MPI FP16 type");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI FP16 summation op
    ret = MPI_Op_create(&fp16_sum_op, 1, &fp16.sum_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 sum op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI FP16 production op
    ret = MPI_Op_create(&fp16_prod_op, 1, &fp16.prod_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 prod op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI FP16 min op
    ret = MPI_Op_create(&fp16_min_op, 1, &fp16.min_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 min op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

    // create custom MPI FP16 max op
    ret = MPI_Op_create(&fp16_max_op, 1, &fp16.max_op);
    if (ret != MPI_SUCCESS) {
        LOG_ERROR("cannot create MPI FP16 max op");
        print_error(ret);
        return ATL_STATUS_FAILURE;
    }

#endif // ATL_MPI_FP16

    return ATL_STATUS_SUCCESS;
}

void atl_mpi_global_data::fp16_finalize() {
    if (fp16.dtype != MPI_DATATYPE_NULL) {
        MPI_Type_free(&fp16.dtype);
    }

    if (fp16.sum_op != MPI_OP_NULL) {
        MPI_Op_free(&fp16.sum_op);
    }

    if (fp16.prod_op != MPI_OP_NULL) {
        MPI_Op_free(&fp16.prod_op);
    }

    if (fp16.min_op != MPI_OP_NULL) {
        MPI_Op_free(&fp16.min_op);
    }

    if (fp16.max_op != MPI_OP_NULL) {
        MPI_Op_free(&fp16.max_op);
    }
}

atl_status_t atl_mpi_global_data::check_impi_env(const atl_attr_t& attr) {
    char* ep_count_env = getenv("I_MPI_THREAD_MAX");
    if (!ep_count_env)
        return ATL_STATUS_FAILURE;
    if (atoi(ep_count_env) != (int)(get_ep_count(attr)))
        return ATL_STATUS_FAILURE;

    if (!getenv("I_MPI_ROOT")) {
        atl_mpi_lib_type_t type = ATL_MPI_LIB_IMPI;
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

    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_global_data::update_global_data(atl_attr_t* attr) {
    if (mpi_lib_attr.type == ATL_MPI_LIB_NONE)
        mpi_lib_attr = get_lib_attr();

    extra_ep = attr->in.enable_extra_ep;

    mnic_type = attr->in.mnic_type;
    if (mpi_lib_attr.type != ATL_MPI_LIB_MPICH) {
        /* only MPICH supports multi-NIC */
        mnic_type = ATL_MNIC_NONE;
    }

    if (mnic_type == ATL_MNIC_LOCAL) {
        mnic_count = get_nic_count(LOCAL_NIC_COUNT_KEY);
    }
    else if (mnic_type == ATL_MNIC_GLOBAL) {
        mnic_count = get_nic_count(GLOBAL_NIC_IDX_KEY);
    }
    else if (mnic_type == ATL_MNIC_NONE) {
        mnic_count = 1;
    }
    mnic_count = std::min(mnic_count, attr->in.mnic_count);
    mnic_count = std::min(mnic_count, attr->in.ep_count);
    mnic_count = std::max(mnic_count, (size_t)(1));

    if (bf16_init() == ATL_STATUS_FAILURE) {
        bf16_finalize();
        return ATL_STATUS_FAILURE;
    }

    if (fp16_init() == ATL_STATUS_FAILURE) {
        fp16_finalize();
        return ATL_STATUS_FAILURE;
    }
    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_global_data::set_mpich_env(const atl_attr_t& attr) {
    char ep_count_str[MPI_MAX_INFO_VAL] = { 0 };
    snprintf(ep_count_str, MPI_MAX_INFO_VAL, "%zu", get_ep_count(attr));

    setenv("MPIR_CVAR_CH4_MT_MODEL", "direct", 0);
    setenv("MPIR_CVAR_CH4_NUM_VCIS", ep_count_str, 0);
    setenv("MPIR_CVAR_CH4_OFI_MAX_VCIS", ep_count_str, 0);
    setenv("MPIR_COMM_HINT_VCI", EP_IDX_KEY, 0);

    if (attr.in.mnic_type != ATL_MNIC_NONE) {
        setenv("MPIR_CVAR_CH4_OFI_ENABLE_NIC_SELECTION", "1", 0);
    }

    auto& env = ccl::global_data::env();
    if (env.log_level >= ccl_log_level::info) {
        setenv("MPIR_CVAR_CH4_RUNTIME_CONF_DEBUG", "1", 0);
        setenv("MPIR_CVAR_CH4_OFI_CAPABILITY_SETS_DEBUG", "1", 0);
        setenv("MPIR_CVAR_CH4_OFI_DUMP_NIC_SETTINGS", "1", 0);
    }

    setenv("FI_PSM2_DELAY", "0", 0);
    setenv("FI_PSM2_TIMEOUT", "0", 0);
    setenv("FI_PSM2_NAME_SERVER", "0", 0);
    setenv("HFI_NO_CPUAFFINITY", "1", 0);

    return ATL_STATUS_SUCCESS;
}

/* set these knobs without detection of MPI library type */
atl_status_t atl_mpi_global_data::set_base_env(const atl_attr_t& attr) {
    setenv("PSM2_MULTI_EP", "1", 0);
    setenv("FI_OFI_RXM_USE_HASH", "0", 0);

#ifdef CCL_ENABLE_SYCL
    setenv("FI_SHM_DISABLE_CMA", "1", 0);
#endif // CCL_ENABLE_SYCL

    setenv("MPIR_CVAR_DEFAULT_THREAD_LEVEL", "MPI_THREAD_MULTIPLE", 0);

    /* request IMPI level append library kind into MPI_Get_library_version output */
    setenv("I_MPI_INFO_LIBRARY_KIND", "1", 0);

    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_global_data::set_env(const atl_attr_t& attr) {
    if (mpi_lib_attr.type != ATL_MPI_LIB_NONE) {
        /* library type was already detected and env was set, make sanity check */
        if (mpi_lib_attr.type == ATL_MPI_LIB_IMPI) {
            return check_impi_env(attr);
        }
        else if (mpi_lib_attr.type == ATL_MPI_LIB_MPICH) {
            return check_mpich_env(attr);
        }
        return ATL_STATUS_SUCCESS;
    }

    set_base_env(attr);

    mpi_lib_attr = get_lib_attr();

    if (mpi_lib_attr.type == ATL_MPI_LIB_NONE) {
        return ATL_STATUS_SUCCESS;
    }

    if (mpi_lib_attr.type == ATL_MPI_LIB_IMPI) {
        set_impi_env(attr, mpi_lib_attr);
        check_impi_env(attr);
    }
    else if (mpi_lib_attr.type == ATL_MPI_LIB_MPICH) {
        set_mpich_env(attr);
        check_mpich_env(attr);
    }

    int is_mpi_inited = 0;
    MPI_Initialized(&is_mpi_inited);
    if (is_mpi_inited) {
        LOG_WARN("MPI was initialized externally, CCL-MPI specific environment is ignored");
    }
    else {
        LOG_DEBUG("set CCL-MPI specific environment");
    }

    return ATL_STATUS_SUCCESS;
}

atl_status_t atl_mpi_global_data::check_mpich_env(const atl_attr_t& attr) {
    char* ep_count_env = getenv("MPIR_CVAR_CH4_OFI_MAX_VCIS");
    if (!ep_count_env)
        return ATL_STATUS_FAILURE;
    if (atoi(ep_count_env) != (int)(get_ep_count(attr)))
        return ATL_STATUS_FAILURE;
    return ATL_STATUS_SUCCESS;
}

#ifdef ATL_MPI_BF16
MPI_Op atl_mpi_global_data::atl2mpi_op_bf16(atl_reduction_t rtype) {
    switch (rtype) {
        case ATL_REDUCTION_SUM: return bf16.sum_op;
        case ATL_REDUCTION_PROD: return bf16.prod_op;
        case ATL_REDUCTION_MIN: return bf16.min_op;
        case ATL_REDUCTION_MAX: return bf16.max_op;
        default: printf("unknown reduction type: %d\n", rtype); exit(1);
    }
}
#endif // ATL_MPI_BF16

#ifdef ATL_MPI_FP16
MPI_Op atl_mpi_global_data::atl2mpi_op_fp16(atl_reduction_t rtype) {
    switch (rtype) {
        case ATL_REDUCTION_SUM: return fp16.sum_op;
        case ATL_REDUCTION_PROD: return fp16.prod_op;
        case ATL_REDUCTION_MIN: return fp16.min_op;
        case ATL_REDUCTION_MAX: return fp16.max_op;
        default: printf("unknown reduction type: %d\n", rtype); exit(1);
    }
}
#endif // ATL_MPI_FP16

void atl_mpi_global_data::print_log_info() {
    if (ctx_count == 1) {
        LOG_INFO("atl-mpi-global:")
        LOG_INFO("  is_external_init: ", is_external_init);
        LOG_INFO("  mpi_lib_attr.type: ", mpi_lib_infos[mpi_lib_attr.type].name);
        LOG_INFO("  mpi_lib_attr.hmem: ", mpi_lib_attr.hmem);
        LOG_INFO("  extra_ep: ", extra_ep);
        LOG_INFO("  mnic_type: ", mnic_type);
        if (mnic_type != ATL_MNIC_NONE)
            LOG_INFO("  mnic_count: ", mnic_count);
    }
}
// TODO: move it in one place(same struct and func in atl_mail.cpp)
typedef struct atl_mpi_comm_info {
    int found;
    MPI_Comm comm;
    char key[MPI_MAX_INFO_KEY];
    char value[MPI_MAX_INFO_VAL];

    atl_mpi_comm_info() {
        found = 0;
        comm = MPI_COMM_WORLD;
        memset(key, 0, MPI_MAX_INFO_KEY);
        memset(value, 0, MPI_MAX_INFO_VAL);
    }
} atl_mpi_comm_info_t;

atl_mpi_comm_info_t atl_mpi_get_comm_info1(MPI_Comm comm, const char* key) {
    MPI_Info info;
    atl_mpi_comm_info_t res;
    res.comm = comm;
    snprintf(res.key, MPI_MAX_INFO_KEY, "%s", key);

    MPI_Comm_get_info(res.comm, &info);
    MPI_Info_get(info, key, MPI_MAX_INFO_VAL, res.value, &res.found);
    MPI_Info_free(&info);

    return res;
}

size_t atl_mpi_global_data::get_nic_count(const char* nic_count_key) {
    size_t count = 1;
    atl_mpi_comm_info_t info = atl_mpi_get_comm_info1(MPI_COMM_WORLD, nic_count_key);
    CCL_THROW_IF_NOT(info.found, "MPI comm key ", nic_count_key, " was not set");

    count = atoi(info.value);
    if (count <= 0) {
        count = 1;
    }

    return count;
}

#endif
