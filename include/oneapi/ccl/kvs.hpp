#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
namespace detail {
class environment;
}
namespace v1 {
class communicator;
}

class kvs_impl;

namespace v1 {

class CCL_API kvs_interface {
public:
    virtual ~kvs_interface() = default;

    virtual vector_class<char> get(const string_class& key) = 0;

    virtual void set(const string_class& key, const vector_class<char>& data) = 0;
};

class CCL_API kvs final : public kvs_interface {
public:
    static constexpr size_t address_max_size = 256;
    using address_type = array_class<char, address_max_size>;

    ~kvs() override;

    address_type get_address() const;

    vector_class<char> get(const string_class& key) override;

    void set(const string_class& key, const vector_class<char>& data) override;

private:
    friend class ccl::detail::environment;
    friend class ccl::v1::communicator;

    kvs(const kvs_attr& attr);
    kvs(const address_type& addr, const kvs_attr& attr);
    const kvs_impl& get_impl();

    address_type addr;
    unique_ptr_class<kvs_impl> pimpl;
};

} // namespace v1

using v1::kvs_interface;
using v1::kvs;

} // namespace ccl
