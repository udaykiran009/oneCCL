#include <cstdint>
#include <numeric>

#include "coll/coll.hpp"
#include "coll/coll_check.hpp"
#include "common/env/env.hpp"
#include "common/global/global.hpp"

#ifdef CCL_ENABLE_SYCL
std::string ccl_usm_type_to_str(sycl::usm::alloc type) {
    switch (type) {
        case sycl::usm::alloc::host: return "host";
        case sycl::usm::alloc::device: return "device";
        case sycl::usm::alloc::shared: return "shared";
        case sycl::usm::alloc::unknown: return "unknown";
        default: CCL_THROW("unexpected USM type: ", static_cast<int>(type));
    }
}

std::string ccl_sycl_device_to_str(const sycl::device& dev) {
    if (dev.is_host()) {
        return "host";
    }
    else if (dev.is_cpu()) {
        return "cpu";
    }
    else if (dev.is_gpu()) {
        return "gpu";
    }
    else if (dev.is_accelerator()) {
        return "accel";
    }
    else {
        CCL_THROW("unexpected device type");
    }
}

void ccl_check_usm_pointers(const ccl_coll_param& param) {
    auto bufs = param.get_all_non_zero_bufs();
    if (bufs.empty()) {
        return;
    }

    auto dev = param.stream->get_native_stream().get_device();
    auto ctx = param.stream->get_native_stream().get_context();

    std::set<sycl::usm::alloc> usm_types;
    for (size_t idx = 0; idx < bufs.size(); idx++) {
        usm_types.insert(sycl::get_pointer_type(bufs[idx], ctx));
    }

    if (usm_types.size() != 1) {
        auto first_usm_type = *usm_types.begin();
        auto second_usm_type = *(++usm_types.begin());
        CCL_THROW("coll: ",
                  ccl_coll_type_to_str(param.ctype),
                  ", mixed USM pointer types (",
                  ccl_usm_type_to_str(first_usm_type),
                  ", ",
                  ccl_usm_type_to_str(second_usm_type),
                  ") within single operation are not supported, ",
                  "device type: ",
                  ccl_sycl_device_to_str(dev));
    }

    sycl::usm::alloc usm_type = *usm_types.begin();
    bool is_valid_type = true;

    if ((usm_type == sycl::usm::alloc::host) && (dev.is_gpu() || dev.is_accelerator()))
        is_valid_type = false;

    if ((usm_type == sycl::usm::alloc::device) && !(dev.is_gpu() || dev.is_accelerator()))
        is_valid_type = false;

    if (usm_type == sycl::usm::alloc::unknown)
        is_valid_type = false;

    LOG_DEBUG("coll: ",
              ccl_coll_type_to_str(param.ctype),
              ", USM pointer type: ",
              ccl_usm_type_to_str(usm_type),
              ", device type: ",
              ccl_sycl_device_to_str(dev));

    CCL_THROW_IF_NOT(is_valid_type,
                     "coll: ",
                     ccl_coll_type_to_str(param.ctype),
                     " - invalid USM pointer type: ",
                     ccl_usm_type_to_str(usm_type),
                     " for device type: ",
                     ccl_sycl_device_to_str(dev));
}
#endif // CCL_ENABLE_SYCL

void ccl_coll_validate_user_input(const ccl_coll_param& param, const ccl_coll_attr& attr) {
    CCL_THROW_IF_NOT(ccl::global_data::env().atl_transport == ccl_atl_ofi || !(attr.reduction_fn),
                     "custom reduction is supported for OFI transport only");

    CCL_THROW_IF_NOT(ccl_datatype_storage::is_predefined_datatype(param.dtype.idx()) ||
                         ccl::global_data::env().atl_transport == ccl_atl_ofi,
                     "custom datatype is supported for OFI transport only");

    CCL_THROW_IF_NOT((param.ctype != ccl_coll_allreduce && param.ctype != ccl_coll_reduce &&
                      param.ctype != ccl_coll_sparse_allreduce) ||
                         ccl_datatype_storage::is_predefined_datatype(param.dtype.idx()) ||
                         attr.reduction_fn,
                     "custom datatype requires custom reduction");

    CCL_THROW_IF_NOT(param.ctype == ccl_coll_allreduce ||
                         !(attr.prologue_fn || attr.epilogue_fn || attr.reduction_fn),
                     "prologue/epilogue/custom reduction is supported for allreduce only");

    CCL_THROW_IF_NOT(param.ctype == ccl_coll_allgatherv || !(attr.vector_buf),
                     "vector buffer is supported for allgatherv only");

    if (param.ctype == ccl_coll_sparse_allreduce) {
        CCL_THROW_IF_NOT(
            ccl::global_data::env().sparse_allreduce_algo_raw != "mask" || !(attr.reduction_fn),
            "mask algorithm for sparse_allreduce does not support custom reduction");

        CCL_THROW_IF_NOT(
            (attr.sparse_allreduce_completion_fn || attr.sparse_allreduce_alloc_fn) &&
                !(reinterpret_cast<uintptr_t>(attr.sparse_allreduce_completion_fn) &
                  reinterpret_cast<uintptr_t>(attr.sparse_allreduce_alloc_fn)),
            "sparse_allreduce requires completion callback only or allocation callback only");
    }

    if (param.dtype.idx() == ccl::datatype::float16) {
        CCL_THROW_IF_NOT(ccl::global_data::env().fp16_impl_type != ccl_fp16_no_compiler_support,
                         "FP16 datatype is requested but not supported by CCL compiler");
        CCL_THROW_IF_NOT(ccl::global_data::env().fp16_impl_type != ccl_fp16_no_hardware_support,
                         "FP16 datatype is requested but not supported by hardware");
    }

    if (param.dtype.idx() == ccl::datatype::bfloat16) {
        CCL_THROW_IF_NOT(ccl::global_data::env().bf16_impl_type != ccl_bf16_no_compiler_support,
                         "BF16 datatype is requested but not supported by CCL compiler");
        CCL_THROW_IF_NOT(ccl::global_data::env().bf16_impl_type != ccl_bf16_no_hardware_support,
                         "BF16 datatype is requested but not supported by hardware");
    }

    if (param.ctype == ccl_coll_bcast || param.ctype == ccl_coll_reduce) {
        CCL_THROW_IF_NOT(param.root < param.comm->size(),
                         "unexpected root ",
                         param.root,
                         ", comm size ",
                         param.comm->size());
    }

    if (param.stream) {
#ifdef CCL_ENABLE_SYCL
        /* SYCL specific validation */

        /* TODO: compare stream dev/ctx and comm dev/ctx */
        // sycl::device stream_dev = param.stream->get_native().get_context();
        // sycl::device stream_ctx = param.stream->get_native().get_device();

        if (!attr.is_sycl_buffer) {
            /* check whether USM pointers have expected type */
            ccl_check_usm_pointers(param);
        }
#endif // CCL_ENABLE_SYCL
    }
}
