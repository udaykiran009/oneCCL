#pragma once
#include <atomic>
#include <map>
#include <memory>
#include "common/comm/l0/context/scaling_ctx/observer_session_key.hpp"
#include "common/comm/l0/modules/supported_modules.hpp"

namespace native {
namespace observer {

/* Low levels session
 * contains raw data for net operations
 */
class session {
public:
    session();
    virtual ~session() = default;

    virtual void prepare(size_t observer_domain_index,
                         size_t observer_domain_count,
                         void* type_erased_param) = 0;

    size_t get_send_tag() const;
    std::string to_string() const;

    size_t produce_data(void** out_chunk, size_t& out_chunk_size);
    bool consume_data(size_t observer_domain_index, void* in_chunk, size_t in_chunk_size);

private:
    size_t send_tag{};

    // low level data
    void* host_producer_memory;
    counter_type* host_producer_ready_bytes;
    size_t host_consumed_bytes;
    size_t host_expected_bytes;

    void* device_consumer_total_memory;
    counter_type* device_consumer_ready_bytes;
    size_t device_produced_bytes;

    ze_command_list_handle_t copy_immediate_list;
};

struct session_notification {
    session_notification(void* addr, size_t size_bytes)
            : host_src_ptr(addr),
              src_size_bytes(size_bytes) {}
    void* host_src_ptr;
    size_t src_size_bytes;
};

using shared_session_ptr = std::shared_ptr<session>;

/* High level session
 * Contains collective communication data
 */
template <ccl_coll_type coll_type, class native_data_type, ccl::device_topology_type class_id>
struct typed_session : public session {
    typed_session(producer_description& in_param,
                  size_t observer_domain_index,
                  size_t observer_domain_count) {
        params.init(in_param.staged_buffer_elem_count,
                    observer_domain_index,
                    observer_domain_count,
                    in_param.context,
                    in_param.device);
    }

    const context_description<coll_type, native_data_type>& get_context_description() const {
        return params;
    }

    void prepare(size_t observer_domain_index,
                 size_t observer_domain_count,
                 void* type_erased_param) override {
        auto* out_param =
            static_cast<invoke_params<coll_type, native_data_type>*>(type_erased_param);
        params.reset_staged_counters(observer_domain_index, observer_domain_count);

        out_param->set_out_params(params);
    }

private:
    context_description<coll_type, native_data_type> params;
};

// session owner
// TODO not thread-safe
struct session_table {
    using session_key_t = session_key;

    template <ccl::device_topology_type class_id, class invoke_params_type>
    std::shared_ptr<session> create_session(const session_key_t& key,
                                            invoke_params_type& params,
                                            size_t observer_domain_index,
                                            size_t observer_domain_count) {
        using specific_session = typed_session<invoke_params_type::get_coll_type(),
                                               typename invoke_params_type::native_t,
                                               class_id>;
        auto sess = std::make_shared<specific_session>(
            params.get_producer_params(), observer_domain_index, observer_domain_count);

        params.set_out_params(sess->get_context_description());
        sessions.emplace(key, sess);

        return sess;
    }

    std::string to_string() const;
    std::map<session_key_t, shared_session_ptr> sessions{};

    static size_t get_unique_tag();
};

using shared_session_table_ptr = std::shared_ptr<session_table>;
} // namespace observer
} // namespace native
