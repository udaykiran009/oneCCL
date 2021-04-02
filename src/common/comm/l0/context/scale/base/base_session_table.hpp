#pragma once
#include <atomic>
#include <map>
#include <memory>
#include "common/comm/l0/context/scale/base/base_session_key.hpp"
#include "common/comm/l0/context/scale/base/base_session.hpp"
#include "common/comm/l0/modules/supported_modules.hpp"

namespace native {
namespace observer {

// session owner, not thread-safe
template <class session_interface>
struct session_table {
    using session_key_t = session_key;
    using session_interface_t = session_interface;
    using session_interface_ptr_t = std::shared_ptr<session_interface_t>;

    template <template <ccl::device_topology_type, class...> class specific_session,
              ccl::device_topology_type class_id,
              class invoke_params_type>
    session_interface_ptr_t create_session(const session_key_t& key,
                                           invoke_params_type& params,
                                           size_t observer_domain_index,
                                           size_t observer_domain_count) {
        using specific_session_impl = specific_session<class_id, invoke_params_type>;

        static_assert(std::is_base_of<session_interface_t, specific_session_impl>::value,
                      "Relationship IS-A `specific_session` for `session_interface_t` failed");

        auto sess = std::make_shared<specific_session_impl>(
            params.get_producer_params(), observer_domain_index, observer_domain_count, key);

        params.set_out_params(sess->get_ctx_descr());
        sessions.emplace(key, sess);

        return sess;
    }

    size_t get_unique_tag() {
        static std::atomic<size_t> tag_counter{ 1 };
        return tag_counter.fetch_add(1);
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "sessions count: " << sessions.size() << std::endl;
        for (const auto& val : sessions) {
            ss << "[" << val.first << ", " << reinterpret_cast<void*>(val.second.get()) << "]\n"
               << val.second->to_string() << std::endl;
        }
        return ss.str();
    }

    std::map<session_key_t, session_interface_ptr_t> sessions{};
};
} // namespace observer
} //namespace native