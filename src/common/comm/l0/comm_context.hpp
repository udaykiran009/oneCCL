#include "ccl_aliases.hpp"
#include "ccl_device_types.hpp"
#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

#include "ccl_request.hpp"
#include "ccl_device_communicator.hpp"

#include "common/comm/l0/comm_context_id.hpp"

class host_communicator;
namespace ccl
{

struct gpu_comm_attr;
using shared_communicator_t = std::shared_ptr<host_communicator>;

class comm_group {
public:
    friend class environment;
    friend class group_context;

    using context_t =
        typename unified_device_context_type::ccl_native_t;

    ~comm_group();
    /**
     * Device Communicator creation API: single communicator creation, based on @device
     */
    template <
        class DeviceType,
        typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                int>::type = 0>
    device_communicator create_communicator(const DeviceType& device,
                                       device_comm_split_attr_t attr = ccl_empty_attr());

    /**
     * Created @device_comm_split_attr_t, which used to create device_communicators from @comm_group_t
     */
    //device_comm_split_attr_t create_device_comm_split_attr();

    /**
     * Device Communicator creation API: single communicator creation, based on index @device_id
     */
    template <
        class DeviceType,
        typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                int>::type = 0>
    device_communicator create_communicator(DeviceType device_id,
                                       device_comm_split_attr_t attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template <template <class...> class Container, class Type>
    std::vector<device_communicator> create_communicators(
        const Container<Type>& device_ids,
        device_comm_split_attr_t attr = ccl_empty_attr());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices iterator @InputIt
     */
    template <class InputIt>
    std::vector<device_communicator> create_communicators(
        InputIt first,
        InputIt last,
        device_comm_split_attr_t attr = ccl_empty_attr());

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
}
