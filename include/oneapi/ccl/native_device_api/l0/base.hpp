#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ze_api.h>

#ifndef UT
//#include "oneapi/ccl/types.hpp"
//#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/lp_types.hpp"
#endif

namespace native {
/**
 * Base RAII L0 handles wrappper
 * support serialize/deserialize concept
 */

template <class handle_type, class resource_owner, class cl_context>
class cl_base {
public:
    friend resource_owner;
    using self_t = cl_base<handle_type, resource_owner, cl_context>;
    using handle_t = handle_type;
    using owner_t = resource_owner;
    using owner_ptr_t = std::weak_ptr<resource_owner>;
    using context_t = cl_context;
    using context_ptr_t = std::weak_ptr<cl_context>;

    cl_base(cl_base&& src) noexcept;
    cl_base& operator=(cl_base&& src) noexcept;
    ~cl_base() noexcept;

    // getter/setters
    std::shared_ptr<self_t> get_instance_ptr();

    handle_t release();
    handle_t& get() noexcept;
    const handle_t& get() const noexcept;
    handle_t* get_ptr() noexcept;
    const handle_t* get_ptr() const noexcept;
    const owner_ptr_t get_owner() const;
    const context_ptr_t get_ctx() const;

    // serialization/deserialization
    static constexpr size_t get_size_for_serialize();

    template <class... helpers>
    size_t serialize(std::vector<uint8_t>& out, size_t from_pos, const helpers&... args) const;

    template <class type, class... helpers>
    static std::shared_ptr<type> deserialize(const uint8_t** data,
                                             size_t& size,
                                             std::shared_ptr<cl_context> ctx,
                                             helpers&... args);

protected:
    cl_base(handle_t h, owner_ptr_t parent, context_ptr_t ctx);

    handle_t handle;

private:
    owner_ptr_t owner;
    context_ptr_t context;
};

template <class value_type>
using indexed_storage = std::multimap<uint32_t, value_type>;
std::ostream& operator<<(std::ostream& out, const ccl::device_index_type& index);
} // namespace native
