#include "native_device_api/l0/utils.hpp"
#include "native_device_api/l0/device.hpp"

namespace native {
namespace details {

adjacency_matrix::adjacency_matrix(std::initializer_list<typename base::value_type> init)
        : base(init) {}

cross_device_rating binary_p2p_rating_calculator(const native::ccl_device& lhs,
                                                 const native::ccl_device& rhs,
                                                 size_t weight) {
    return property_p2p_rating_calculator(lhs, rhs, 1);
}
} // namespace details
} // namespace native
