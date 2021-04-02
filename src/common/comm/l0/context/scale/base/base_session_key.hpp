#pragma once
#include <functional>
#include <string>
#include <vector>
#include "oneapi/ccl.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"
#include "coll/algorithms/algorithms_enum.hpp"
#include "common/comm/l0/modules/kernel_params.hpp"

namespace native {
namespace observer {
using counter_type = uint64_t;

struct producer_description {
    size_t rank;
    size_t comm_size;
    counter_type staged_buffer_elem_count;

    std::shared_ptr<ccl_context> context;
    ccl_device& device;
    ccl_device::device_cmd_list immediate_list; //TODO make persisten
};

struct kernel_params_type {
    ccl_coll_type coll_type;
    ccl::datatype data_type;
    ccl::reduction red_type;
};

template <class kernel_params>
struct get_params {
    constexpr static ccl::datatype dtype =
        ccl::native_type_info<typename kernel_params::native_type>::dtype;
    constexpr static ccl::reduction red = ccl::reduction::custom;
};

template <class native_type, ccl_coll_reduction red_type>
struct get_params<kernel_reduction_params_traits<native_type, red_type>> {
    constexpr static ccl::datatype dtype = ccl::native_type_info<native_type>::dtype;
    constexpr static ccl::reduction red = static_cast<ccl::reduction>(red_type);
};

template <class kernel_params>
struct context_descr {
    using native_data_t = typename kernel_params::native_type;

    using host_mem_ptr_t = ccl_context::host_memory_ptr<native_data_t>;
    using host_mem_ptr_cntr_t = ccl_context::host_memory_ptr<counter_type>;
    using dev_mem_ptr_t = ccl_device::device_memory_ptr<native_data_t>;
    using dev_mem_ptr_cntr_t = ccl_device::device_memory_ptr<counter_type>;

    // produced by kernel
    host_mem_ptr_t host_mem_producer;
    host_mem_ptr_cntr_t host_mem_producer_counter;
    size_t host_consumed_bytes;
    size_t host_expected_bytes;

    // consumed by kernel
    dev_mem_ptr_t dev_mem_consumer;
    dev_mem_ptr_cntr_t dev_mem_consumer_counter;
    size_t device_produced_bytes;

    // (TODO consider using 'recv_buff' from collective entry)
    // to reduce copy iterations
    // TODO: rename
    dev_mem_ptr_cntr_t producer_aggregated_memory_offset;

    void init_host_dev_fields() {
        host_mem_producer = nullptr;
        host_mem_producer_counter = nullptr;
        host_consumed_bytes = 0;
        host_expected_bytes = 0;

        dev_mem_consumer = nullptr;
        dev_mem_consumer_counter = nullptr;
        device_produced_bytes = 0;
    }

    void init(size_t staged_buffer_elem_count,
              size_t observer_domain_index,
              size_t observer_domain_count,
              std::shared_ptr<ccl_context>& context,
              ccl_device& device) {
        // set all fields by 0
        init_host_dev_fields();

        /* HOST */
        // create staged mem in host context (Host memory allocation descriptor)
        ze_host_mem_alloc_desc_t host_descr = ccl_context::get_default_host_alloc_desc();
        host_descr.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;

        // host mem buf
        host_mem_producer = context->template alloc_memory<native_data_t>(
            staged_buffer_elem_count,
            /*TODO use page size*/ sizeof(native_data_t),
            host_descr);

        // create staged mem counter in host context (host mem buf counter)
        host_mem_producer_counter = context->template alloc_memory<counter_type>(
            1, /*TODO use page size*/ sizeof(counter_type), host_descr);

        host_expected_bytes = staged_buffer_elem_count;

        /* DEVICE */
        ze_device_mem_alloc_desc_t mem_descr = ccl_device::get_default_mem_alloc_desc();

        // create total aggregated memory in device context
        mem_descr.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
        dev_mem_consumer = device.template alloc_memory_ptr<native_data_t>(
            staged_buffer_elem_count * observer_domain_count,
            sizeof(native_data_t),
            context,
            mem_descr);

        // create offset in device context
        mem_descr.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED;
        producer_aggregated_memory_offset = device.template alloc_memory_ptr<counter_type>(
            1, sizeof(counter_type), context, mem_descr);

        // create aggregated counter in device context
        dev_mem_consumer_counter = device.template alloc_memory_ptr<counter_type>(
            1, sizeof(counter_type), context, mem_descr);

        /* COUNTERS */
        reset_counters(observer_domain_index, observer_domain_count);
    }

    void reset_counters(size_t observer_domain_index, size_t observer_domain_count) {
        counter_type filled_counter_value = 0;

        host_mem_producer_counter->enqueue_write_sync(&filled_counter_value, 1);

        filled_counter_value = observer_domain_index * host_mem_producer->count();

        producer_aggregated_memory_offset->enqueue_write_sync(&filled_counter_value, 1);

        filled_counter_value = 0;
        dev_mem_consumer_counter->enqueue_write_sync(&filled_counter_value, 1);
    }
};

template <ccl_coll_type coll_type, class kernel_params>
struct invoke_params {
    using kernel_params_t = kernel_params;

    static constexpr ccl_coll_type get_coll_type() {
        return coll_type;
    }

    invoke_params(producer_description&& in_producer_params)
            : in_params(std::move(in_producer_params)),
              in_kernel_params{ coll_type,
                                get_params<kernel_params>::dtype,
                                get_params<kernel_params>::red },
              out_params(),
              valid(false) {}

    void set_out_params(const context_descr<kernel_params>& src) {
        out_params = src;
        valid = true;
    }

    bool is_valid() const {
        return valid;
    }

    const producer_description& get_producer_params() const {
        return in_params;
    }

    producer_description& get_producer_params() {
        return in_params;
    }

    const context_descr<kernel_params>& get_ctx_params() const {
        if (!is_valid()) {
            throw std::runtime_error("observer invocation params are not ready");
        }
        return out_params;
    }

private:
    producer_description in_params;
    kernel_params_type in_kernel_params;
    context_descr<kernel_params> out_params;
    bool valid;
};

struct session_key {
    using hash_core_t = size_t;

    friend std::ostream& operator<<(std::ostream& out, const session_key& key) {
        out << key.to_string();
        return out;
    }

    template <class T>
    session_key(const T* src) : hash(std::hash<const T*>{}(src)) {}

    bool operator<(const session_key& other) const noexcept;

    std::string to_string() const;

private:
    hash_core_t hash;
};
} // namespace observer
} // namespace native
