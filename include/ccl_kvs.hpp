#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class kvs_interface {
public:
    virtual vector_class<char> get(const string_class& prefix, const string_class& key) const = 0;

    virtual void set(const string_class& prefix,
                     const string_class& key,
                     const vector_class<char>& data) const = 0;

    virtual ~kvs_interface() = default;
};

class kvs_impl;
class kvs final : public kvs_interface {
public:
    static constexpr size_t addr_max_size = 256;
    using addr_t = array_class<char, addr_max_size>;

    const addr_t& get_addr() const;

    ~kvs() override;

    vector_class<char> get(const string_class& prefix, const string_class& key) const override;

    void set(const string_class& prefix,
             const string_class& key,
             const vector_class<char>& data) const override;

private:
    friend class environment;

    kvs();
    kvs(const addr_t& addr);

    unique_ptr_class<kvs_impl> pimpl;
};
}
