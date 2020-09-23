#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class CCL_API kvs_interface {
public:

    virtual ~kvs_interface() = default;

    virtual vector_class<char> get(const string_class& key) const = 0;

    virtual void set(const string_class& key, const vector_class<char>& data) const = 0;
};

class kvs_impl;
class CCL_API kvs final : public kvs_interface {
public:
    static constexpr size_t address_max_size = 256;
    using address_type = array_class<char, address_max_size>;

    ~kvs() override;

    address_type get_address() const;

    vector_class<char> get(const string_class& key) const override;

    void set(const string_class& key, const vector_class<char>& data) const override;

private:
    friend class environment;

    kvs();
    kvs(const address_type& addr);
    const kvs_impl& get_impl();

    address_type addr;
    unique_ptr_class<kvs_impl> pimpl;
};
} // namespace ccl
