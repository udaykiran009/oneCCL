#include "oneapi/ccl/aliases.hpp"
#include "oneapi/ccl/device_types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "oneapi/ccl/coll_attr.hpp"

#include "oneapi/ccl/stream_attr_ids.hpp"
#include "oneapi/ccl/stream_attr_ids_traits.hpp"
#include "oneapi/ccl/stream.hpp"

#include "oneapi/ccl/event.hpp"
#include "oneapi/ccl/communicator.hpp"

#include "common/comm/l0/comm_context_id.hpp"
#include "common/comm/comm_interface.hpp"

namespace ccl {
namespace detail {
class environment;
}

class host_communicator;
struct gpu_comm_attr;
using shared_communicator_t = std::shared_ptr<host_communicator>;

class comm_group {
public:
    friend class ccl::detail::environment;
    friend struct group_context;

    using context_t = typename unified_context_type::ccl_native_t;

    ~comm_group();
    /**
     * Device Communicator creation API: single communicator creation, based on @device
     */
    template <class DeviceType,
              class ContextType,
              typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                       ccl::device_index_type>::value,
                                      int>::type = 0>
    ccl::communicator_interface_ptr create_communicator_from_group(
        const DeviceType& device,
        const ContextType& context,
        const comm_split_attr& attr = ccl_empty_attr());

    /**
     * Device Communicator creation API: single communicator creation, based on index @device_id
     */
    template <class DeviceType,
              class ContextType,
              typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                      int>::type = 0>
    ccl::communicator_interface_ptr create_communicator_from_group(
        const DeviceType& device_id,
        const ContextType& context,
        const comm_split_attr& attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices iterator @InputIt
     */
    template <class InputIt, class ContextType>
    std::vector<communicator> create_communicators_group(InputIt first,
                                                         InputIt last,
                                                         const ContextType& context,
                                                         comm_split_attr attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template <template <class...> class Container, class Type, class ContextType>
    std::vector<communicator> create_communicators_group(const Container<Type>& device_ids,
                                                         const ContextType& context,
                                                         comm_split_attr attr = ccl_empty_attr());

    /**
     * Return device context allocated during group creation
     */
    //context_native_const_reference_t get_context() const;

    bool sync_group_size(size_t device_group_size);
    /*
    std::string to_string() const;
*/
    const group_unique_key& get_unique_id() const;

private:
    comm_group(ccl::shared_communicator_t comm,
               size_t current_device_group_size,
               size_t process_device_group_size,
               group_unique_key id);
    std::unique_ptr<gpu_comm_attr> pimpl;
};
} // namespace ccl
