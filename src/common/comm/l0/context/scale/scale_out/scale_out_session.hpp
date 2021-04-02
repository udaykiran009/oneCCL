#pragma once

#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/event.hpp"
#include "common/comm/l0/context/scale/base/base_session.hpp"
#include "common/comm/l0/context/scale/numa/numa_session.hpp"

namespace ccl {
class host_communicator;
}

namespace native {
namespace observer {

class scale_out_session_iface : public base_session {
public:
    scale_out_session_iface() = default;
    virtual ~scale_out_session_iface() = default;

    size_t get_send_tag() const;
    std::string to_string() const;

    virtual void produce_data(std::shared_ptr<ccl::host_communicator>& comm) = 0;
    virtual void consume_data(size_t observer_domain_index,
                              std::shared_ptr<ccl::host_communicator>& comm) = 0;
    virtual bool is_consumed() = 0;
    virtual bool is_produced() = 0;

private:
    size_t send_tag{};
};

struct session_notification_handle {
    using notification_handle_t = ccl::event;
    //using notification_handle_ptr_t = std::unique_ptr<notification_handle_t>;

    //TODO use custom allocator instead vector
    std::vector<uint8_t> output_buffer;
    notification_handle_t op_handle;
    //TODO
    // because notification_handle_t class interface do not provide distinction
    // between canceled and finished use special flag is denoted extended state of op_handle.
    // USE event_impl pointer instead! Fix host_communicator to return event_impl!
    bool op_handle_ready;
};

struct ccl_worker_adapter {
    static void submit_coll_work(std::shared_ptr<ccl::host_communicator>& comm,
                                 const session_notification& in,
                                 session_notification_handle& out,
                                 kernel_params_type& kernel_params);
};

template <ccl::device_topology_type class_id, class session_invoke_params>
struct scale_out_session : public scale_out_session_iface {
    using base_t = scale_out_session_iface;
    using invoke_params_t = session_invoke_params;
    using kernel_params_t = typename invoke_params_t::kernel_params_t;
    using native_type = typename kernel_params_t::native_type;
    using session_key_t = session_key;

    scale_out_session(producer_description& in_param,
                      size_t observer_domain_index,
                      size_t observer_domain_count,
                      const session_key_t& key)
            : base_t(),
              proxy_session(in_param, observer_domain_index, observer_domain_count, key) {
        //TODO use `session_invoke_params` information to calculate possible `pending_notifications` reserve
        // based on chunk size
        pending_notifications.reserve(16);
    }

    context_descr<kernel_params_t>& get_ctx_descr() {
        return proxy_session.get_ctx_descr();
    }

    void prepare(size_t observer_domain_index,
                 size_t observer_domain_count,
                 void* type_erased_param) override {
        proxy_session.prepare(observer_domain_index, observer_domain_count, type_erased_param);

        auto* out_param = static_cast<invoke_params_t*>(type_erased_param);

        // allocate cpu gw staging slots
        pending_notifications.clear();

        (void)out_param;
    }

    void produce_data(std::shared_ptr<ccl::host_communicator>& comm) override {
        void* partial_chunk = nullptr;
        size_t partial_chunk_size = 0;

        // get own device partial chunk data
        proxy_session.produce_data(&partial_chunk, partial_chunk_size);
        if (partial_chunk_size > 0) {
            // notify other scaleout actors in other processes about partial my result
            session_notification notif(partial_chunk, partial_chunk_size);
            session_notification_handle handle;

            ccl_worker_adapter::submit_coll_work(
                comm, notif, handle, proxy_session.get_kernel_params());

            pending_notifications.push_back(std::move(handle));
        }
    }

    void consume_data(size_t observer_domain_index,
                      std::shared_ptr<ccl::host_communicator>& comm) override {
        for (auto it = pending_notifications.begin(); it != pending_notifications.end(); ++it) {
            if (it->op_handle_ready) { // notice: not thread-safe

                if (it->op_handle.test()) {
                    proxy_session.consume_data(observer_domain_index,
                                               it->output_buffer.data(),
                                               it->output_buffer.size() * sizeof(native_type));

                    // notice: not thread-safe
                    it->op_handle_ready = false;
                }
                else {
                    //TODO collectives on CPU side are processing sequencially
                    // if the first handle is not ready yet, then skip following handles
                    break;
                }
            }
        }
    }

    bool is_consumed() override {
        return proxy_session.is_consumed();
    }

    bool is_produced() override {
        return proxy_session.is_produced();
    }

private:
    void notify_data();
    numa_session<class_id, invoke_params_t> proxy_session;

    std::vector<session_notification_handle> pending_notifications;
};
} // namespace observer
} // namespace native