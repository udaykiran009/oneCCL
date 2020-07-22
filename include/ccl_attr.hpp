#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class comm_split_attr_impl;
class device_comm_split_attr_impl;
/**
 * Class @c comm_split_attr allows to configure host communicator split parameters
 */
class comm_split_attr : public pointer_on_impl<comm_split_attr, comm_split_attr_impl> {
public:
    using impl_value_t =
        typename pointer_on_impl<comm_split_attr, comm_split_attr_impl>::impl_value_t;

    friend class device_comm_split_attr;
    friend struct communicator_interface_dispatcher;
    friend class environment;

    virtual ~comm_split_attr() noexcept;

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <comm_split_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <comm_split_attr_id attrId>
    const typename comm_split_attributes_traits<attrId>::type& get_value() const;

protected:
    comm_split_attr(const comm_split_attr& src);

private:
    comm_split_attr(impl_value_t&& impl);
};

using comm_split_attr_t = std::shared_ptr<comm_split_attr>;

#ifdef MULTI_GPU_SUPPORT

/**
 * Class @c device_comm_split_attr allows to configure device communicator split parameters
 */
class device_comm_split_attr
        : public comm_split_attr, public pointer_on_impl<device_comm_split_attr,
                                                        device_comm_split_attr_impl> {
public:
    friend class comm_group;
    friend struct communicator_interface_dispatcher;

    using impl_value_t =
        typename pointer_on_impl<device_comm_split_attr, device_comm_split_attr_impl>::impl_value_t;
    ~device_comm_split_attr() noexcept;

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <device_comm_split_attr_id attrId,
              class Value,
              class = typename std::enable_if<
                  std::is_same<typename ccl_device_attributes_traits<attrId>::type,
                               Value>::value>::type>
    Value set_value(Value&& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <device_comm_split_attr_id attrId>
    const typename ccl_device_attributes_traits<attrId>::type& get_value() const;

private:
    device_comm_split_attr(impl_value_t&& impl);
};

using device_comm_split_attr_t = std::shared_ptr<device_comm_split_attr>;

#endif /* MULTI_GPU_SUPPORT */

} // namespace ccl
