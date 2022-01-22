#pragma once

#ifdef CCL_ENABLE_STUB_BACKEND

#include "comm/comm_interface.hpp"

namespace ccl {

class stub_kvs_impl;

class alignas(CACHELINE_SIZE) stub_comm : public ccl::comm_interface {
public:
    stub_comm(stub_comm& src) = delete;
    stub_comm(stub_comm&& src) = default;
    stub_comm& operator=(stub_comm& src) = delete;
    stub_comm& operator=(stub_comm&& src) = default;
    ~stub_comm();

    static stub_comm* create(device_t device,
                             context_t context,
                             size_t rank,
                             size_t size,
                             std::shared_ptr<ccl::kvs_interface> kvs_interface);

private:
    stub_comm(device_t device,
              context_t context,
              size_t rank,
              size_t size,
              std::shared_ptr<ccl::kvs> kvs,
              const ccl::stub_kvs_impl* kvs_impl);

public:
    int rank() const override {
        return comm_rank;
    }

    int size() const override {
        return comm_size;
    }

    device_ptr_t get_device() const override {
        return device_ptr;
    }

    context_ptr_t get_context() const override {
        return context_ptr;
    }

    // collectives operation declarations
    ccl::event barrier(const ccl::stream::impl_value_t& stream,
                       const ccl::barrier_attr& attr,
                       const ccl::vector_class<ccl::event>& deps = {}) override {
        return barrier_impl(stream, attr, deps);
    }

    ccl::event barrier_impl(const ccl::stream::impl_value_t& stream,
                            const ccl::barrier_attr& attr,
                            const ccl::vector_class<ccl::event>& deps = {});

    COMM_INTERFACE_COLL_DEFINITION__VOID_REQUIRED

    COMM_IMPL_DECLARATION_VOID_REQUIRED

    ccl::comm_interface_ptr split(const ccl::comm_split_attr& attr) override {
        return static_cast<ccl::comm_interface_ptr>(this);
    }

private:
    stub_comm* get_impl() {
        return this;
    }

    ccl::event process_stub_backend();

    device_ptr_t device_ptr;
    context_ptr_t context_ptr;

    size_t comm_rank;
    size_t comm_size;

    // while we only use the imple, keep the original object to avoid early destruction
    std::shared_ptr<ccl::kvs> kvs;
    const ccl::stub_kvs_impl* kvs_impl;
};

} // namespace ccl

#endif // CCL_ENABLE_STUB_BACKEND
