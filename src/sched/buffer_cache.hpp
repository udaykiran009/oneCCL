#pragma once

#include <unordered_map>
#include <vector>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#include "common/utils/hash.hpp"
#endif // CCL_ENABLE_SYCL

#include "common/utils/spinlock.hpp"

namespace ccl {

class regular_buffer_cache;
#ifdef CCL_ENABLE_SYCL
class sycl_buffer_cache;
#endif // CCL_ENABLE_SYCL

class buffer_cache {
public:
    buffer_cache(size_t instance_count)
            : reg_buffers(instance_count)
#ifdef CCL_ENABLE_SYCL
              ,
              sycl_buffers(instance_count)
#endif // CCL_ENABLE_SYCL
    {
    }
    buffer_cache(const buffer_cache&) = delete;
    buffer_cache& operator=(const buffer_cache&) = delete;
    ~buffer_cache();

    void get(size_t idx, size_t bytes, void** pptr);

    void push(size_t idx, size_t bytes, void* ptr);

#ifdef CCL_ENABLE_SYCL
    void get(size_t idx, size_t bytes, const sycl::context& ctx, void** pptr);

    void push(size_t idx, size_t bytes, const sycl::context& ctx, void* ptr);
#endif // CCL_ENABLE_SYCL

    using lock_t = ccl_spinlock;

private:
    std::vector<regular_buffer_cache> reg_buffers;
#ifdef CCL_ENABLE_SYCL
    std::vector<sycl_buffer_cache> sycl_buffers;
#endif // CCL_ENABLE_SYCL
};

class regular_buffer_cache {
public:
    regular_buffer_cache() = default;
    ~regular_buffer_cache();

    void clear();
    void get(size_t bytes, void** pptr);
    void push(size_t bytes, void* ptr);

private:
    buffer_cache::lock_t guard{};

    using key_t = size_t;
    using value_t = void*;
    std::unordered_multimap<key_t, value_t> cache;
};

#ifdef CCL_ENABLE_SYCL
class sycl_buffer_cache {
public:
    sycl_buffer_cache() = default;
    ~sycl_buffer_cache();

    void clear();
    void get(size_t bytes, const sycl::context& ctx, void** pptr);
    void push(size_t bytes, const sycl::context& ctx, void* ptr);

private:
    buffer_cache::lock_t guard{};

    using key_t = typename std::tuple<size_t, sycl::context>;
    using value_t = void*;
    std::unordered_multimap<key_t, value_t, ccl::utils::tuple_hash> cache;
};
#endif // CCL_ENABLE_SYCL

} // namespace ccl
