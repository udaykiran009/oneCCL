#include <sstream>

#include "common/comm/l0/context/scale/base/base_session.hpp"

namespace native {
namespace observer {

session::session()
        : host_producer_memory(nullptr),
          host_producer_ready_bytes(nullptr),
          host_consumed_bytes(0),
          host_expected_bytes(0),

          device_consumer_total_memory(nullptr),
          device_consumer_ready_bytes(nullptr),
          device_produced_bytes(0),
          copy_immediate_list() {}

std::string session::to_string() const {
    return {};
}

size_t session::get_send_tag() const {
    return send_tag;
}

size_t session::produce_data(void** out_chunk, size_t& out_chunk_size) {
    return {};
}

bool session::consume_data(size_t observer_domain_index, void* in_chunk, size_t in_chunk_size) {
    return true;
}

size_t session_table::get_unique_tag() {
    return {};
}

std::string session_table::to_string() const {
    return {};
}
} // namespace observer
} // namespace native
