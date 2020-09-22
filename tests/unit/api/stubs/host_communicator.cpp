#include "host_communicator.hpp"

class ccl_comm;
namespace ccl {
host_communicator::host_communicator(std::shared_ptr<ccl_comm> impl) : comm_impl(impl) {}

size_t host_communicator::rank() const {
    return 0;
}

size_t host_communicator::size() const {
    return 1;
}

void host_communicator::barrier_impl() {}

ccl::request host_communicator::barrier_impl(const barrier_attr& attr) {
    return {};
}
} // namespace ccl
