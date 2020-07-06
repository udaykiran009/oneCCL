#pragma once

/* TODO: add op/coll attributes */

namespace ccl
{

/**
 * Class @c comm_split_attr allows to configure host communicator split parameters
 */
class comm_split_attr
{
public:
    friend class device_comm_split_attr;
    friend struct communicator_interface_dispatcher;
    friend class environment;

    virtual ~comm_split_attr() noexcept;

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template<comm_split_attributes attrId,
             class Value,
             class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template<comm_split_attributes attrId>
    const typename comm_split_attributes_traits<attrId>::type& get_value() const;

protected:
    comm_split_attr(const comm_split_attr& src);

private:
    /* TODO:  remove ccl_host_comm_attr_t ? */
    //comm_split_attr(const ccl_host_comm_attr_t &core = ccl_host_comm_attr_t());
    std::unique_ptr<comm_split_attr_impl> pimpl;
};

using comm_split_attr_t = std::shared_ptr<comm_split_attr>;

#ifdef MULTI_GPU_SUPPORT

/**
 * Class @c device_comm_split_attr allows to configure device communicator split parameters
 */
class device_comm_split_attr : public comm_split_attr
{
public:
    friend class comm_group;
    friend struct communicator_interface_dispatcher;

    ~device_comm_split_attr() noexcept;

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template<device_comm_split_attributes attrId,
             class Value,
             class = typename std::enable_if<std::is_same<typename device_comm_split_attributes_traits<attrId>::type, Value>::value>::type>
    Value set_value(Value&& v);

    /**
     * Get specific attribute value by @attrId
     */
    template<device_comm_split_attributes attrId>
    const typename device_comm_split_attributes_traits<attrId>::type& get_value() const;

private:
    //device_comm_split_attr(const comm_split_attr& src);
    std::unique_ptr<device_comm_split_attr_impl> pimpl;
};

using device_comm_split_attr_t = std::shared_ptr<device_comm_split_attr>;

#endif /* MULTI_GPU_SUPPORT */

} // namespace ccl