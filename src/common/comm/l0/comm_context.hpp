namespace ccl
{

struct gpu_comm_attr;
struct communicator_interface;

class comm_group {
public:
    friend class environment;

    using context_t =
        typename unified_device_context_type::ccl_native_t;

    /**
     * Device Communicator creation API: single communicator creation, based on @device
     */
    template <
        class DeviceType,
        typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                int>::type = 0>
    communicator_t create_communicator(const DeviceType& device,
                                       device_comm_split_attr_t attr = device_comm_split_attr_t());

    /**
     * Created @device_comm_split_attr_t, which used to create device_communicators from @comm_group_t
     */
    device_comm_split_attr_t create_device_comm_split_attr();

    /**
     * Device Communicator creation API: single communicator creation, based on index @device_id
     */
    template <
        class DeviceType,
        typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                int>::type = 0>
    communicator_t create_communicator(DeviceType device_id,
                                       device_comm_split_attr_t attr = device_comm_split_attr_t());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template <template <class...> class Container, class Type>
    std::vector<communicator_t> create_communicators(
        const Container<Type>& device_ids,
        device_comm_split_attr_t attr = device_comm_split_attr_t());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices iterator @InputIt
     */
    template <class InputIt>
    std::vector<communicator_t> create_communicators(
        InputIt first,
        InputIt last,
        device_comm_split_attr_t attr = device_comm_split_attr_t());

    /**
     * Return device context allocated during group creation
     */
    device_context_native_const_reference_t get_context() const;

private:
    comm_group(ccl::shared_communicator_t comm,
               size_t current_device_group_size,
               size_t process_device_group_size);
    std::unique_ptr<gpu_comm_attr> pimpl;
};
}
