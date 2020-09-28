#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_device_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_event_attr_ids.hpp"
#include "oneapi/ccl/ccl_event_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_event.hpp"

#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "oneapi/ccl/ccl_request.hpp"
#include "oneapi/ccl/ccl_communicator.hpp"

#include "common/comm/l0/comm_context_id.hpp"

namespace ccl {

class host_communicator;
struct gpu_comm_attr;
using shared_communicator_t = std::shared_ptr<host_communicator>;

class comm_group {
public:
    friend class environment;
    friend struct group_context;

    using context_t = typename unified_device_context_type::ccl_native_t;

    ~comm_group();
    /**
     * Device Communicator creation API: single communicator creation, based on @device
     */
    template <class DeviceType,
              typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                       ccl::device_index_type>::value,
                                      int>::type = 0>
    communicator create_communicator(const DeviceType& device,
                                            const comm_split_attr& attr = ccl_empty_attr());

    /**
     * Device Communicator creation API: single communicator creation, based on index @device_id
     */
    template <class DeviceType,
              typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                      int>::type = 0>
    communicator create_communicator(const DeviceType& device_id,
                                            const comm_split_attr& attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices iterator @InputIt
     */
    template <class InputIt>
    vector_class<communicator> create_communicators(
        InputIt first,
        InputIt last,
        comm_split_attr attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template <template <class...> class Container, class Type>
    vector_class<communicator> create_communicators(
        const Container<Type>& device_ids,
        comm_split_attr attr = ccl_empty_attr());

    /**
     * Return device context allocated during group creation
     */
    //device_context_native_const_reference_t get_context() const;

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
