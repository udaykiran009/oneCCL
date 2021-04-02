#pragma once

#include "common/comm/l0/context/scale/base/base_session.hpp"
#include "common/comm/l0/context/scale/base/base_session_key.hpp"

namespace native {
namespace observer {

class numa_session_iface : public base_session {
public:
    numa_session_iface(session_key key);

    virtual ~numa_session_iface() = default;

    size_t get_send_tag() const;
    const session_key& get_session_key() const;
    std::string to_string() const;

    virtual void produce_data(void** out_chunk, size_t& out_chunk_size) = 0;
    virtual void consume_data(size_t observer_domain_index,
                              void* in_chunk,
                              size_t in_chunk_size) = 0;
    virtual bool is_consumed() = 0;
    virtual bool is_produced() = 0;

private:
    size_t send_tag{};
    session_key sess_key;
};

/* High level session
 * Contains collective communication data
 */
template <ccl::device_topology_type class_id, class session_invoke_params>
struct numa_session : public numa_session_iface {
    using invoke_params_t = session_invoke_params;
    using kernel_params_t = typename invoke_params_t::kernel_params_t;
    using session_key_t = session_key;

    numa_session(producer_description& in_param,
                 size_t observer_domain_index,
                 size_t observer_domain_count,
                 const session_key_t& key)
            : numa_session_iface(key),
              in_kernel_params{ invoke_params_t::get_coll_type(),
                                get_params<kernel_params_t>::dtype,
                                get_params<kernel_params_t>::red },
              ctx_descr(),
              copy_immediate_list(std::move(in_param.immediate_list)) {
        ctx_descr.init(in_param.staged_buffer_elem_count,
                       observer_domain_index,
                       observer_domain_count,
                       in_param.context,
                       in_param.device);
    }

    context_descr<kernel_params_t>& get_ctx_descr() {
        return ctx_descr;
    }

    kernel_params_type& get_kernel_params() {
        return in_kernel_params;
    }

    virtual void prepare(size_t observer_domain_index,
                         size_t observer_domain_count,
                         void* type_erased_param) override {
        auto* out_param = static_cast<invoke_params_t*>(type_erased_param);
        ctx_descr.reset_counters(observer_domain_index, observer_domain_count);

        out_param->set_out_params(ctx_descr);
    }

    void produce_data(void** out_chunk, size_t& out_chunk_size) override {
        size_t old_consumed = get_ctx_descr().host_consumed_bytes;
        uint64_t total_produced = *get_ctx_descr().host_mem_producer_counter->get();

        size_t to_consume = total_produced - old_consumed;
        if (to_consume) {
            //fence
            LOG_TRACE(to_string(),
                      " - bytes produced: ",
                      total_produced,
                      ", previously bytes consumed: ",
                      old_consumed);
            std::atomic_thread_fence(std::memory_order::memory_order_seq_cst); // TODO: why?

            // do not read data here!
            *out_chunk = (static_cast<typename kernel_params_t::native_type*>(
                              static_cast<void*>(get_ctx_descr().host_mem_producer->get())) +
                          old_consumed);

            // update host_consumed_bytes
            get_ctx_descr().host_consumed_bytes += to_consume;
        }

        // TODO: set logging here
        out_chunk_size = to_consume;
    }

    void consume_data(size_t observer_domain_index, void* in_chunk, size_t in_chunk_size) override {
        /* TODO create event
         * ze_event_handle_t mem_event {};
         */

        void* device_consumer_total_memory = get_ctx_descr().dev_mem_consumer->get();
        auto device_consumer_ready_bytes = get_ctx_descr().dev_mem_consumer_counter->get();
        auto device_produced_bytes = get_ctx_descr().device_produced_bytes;

        // TODO: set logging here

        // copy buffer from host to device
        ze_result_t res = zeCommandListAppendMemoryCopy(
            copy_immediate_list.get(),
            (static_cast<typename kernel_params_t::native_type*>(device_consumer_total_memory) +
             device_produced_bytes),
            in_chunk,
            in_chunk_size * sizeof(typename kernel_params_t::native_type),
            /*mem_event*/ nullptr,
            0,
            nullptr);
        if (res != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string(
                    "cannot append copy NUMA host to device memory for partial result, error: ") +
                native::to_string(res));
        }
        device_produced_bytes += in_chunk_size;
        get_ctx_descr().device_produced_bytes = device_produced_bytes;

        // TODO: set logging here
        // copy size from host to device
        res = zeCommandListAppendMemoryCopy(copy_immediate_list.get(),
                                            device_consumer_ready_bytes,
                                            &device_produced_bytes,
                                            sizeof(device_produced_bytes),
                                            nullptr,
                                            0,
                                            /*&mem_event*/ nullptr);
        if (res != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string(
                    "cannot append copy NUMA host to device memory for ready bytes, error: ") +
                native::to_string(res));
        }
    }

    bool is_consumed() override {
        return get_ctx_descr().device_produced_bytes == get_ctx_descr().host_consumed_bytes;
    }

    bool is_produced() override {
        return get_ctx_descr().host_expected_bytes == get_ctx_descr().host_consumed_bytes;
    }

private:
    kernel_params_type in_kernel_params;
    context_descr<kernel_params_t> ctx_descr;
    ccl_device::device_cmd_list copy_immediate_list;
};

} // namespace observer
} // namespace native