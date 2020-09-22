#ifndef SYCL_BASE_HPP
#define SYCL_BASE_HPP

/* sycl-specific base implementation and its help functions */
#include <iostream>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include <mpi.h>

#include <CL/sycl.hpp>
#include "base_utils.hpp"

#include "oneapi/ccl.hpp"

#define COUNT     size_t(10 * 1024 * 1024)
#define COLL_ROOT (0)

using namespace std;
using namespace cl::sycl;
using namespace cl::sycl::access;

/* help functions for sycl-specific base implementation */
inline bool has_gpu() {
    std::vector<cl::sycl::device> devices = cl::sycl::device::get_devices();
    for (const auto& device : devices) {
        if (device.is_gpu()) {
            return true;
        }
    }
    return false;
}

inline bool has_accelerator() {
    std::vector<cl::sycl::device> devices = cl::sycl::device::get_devices();
    for (const auto& device : devices) {
        if (device.is_accelerator()) {
            return true;
        }
    }
    return false;
}

inline int create_sycl_queue(int argc,
                             char** argv,
                             cl::sycl::queue& queue,
                             ccl_stream_type_t& stream_type) {
    stream_type = ccl_stream_cpu;
    auto exception_handler = [&](cl::sycl::exception_list elist) {
        for (std::exception_ptr const& e : elist) {
            try {
                std::rethrow_exception(e);
            }
            catch (cl::sycl::exception const& e) {
                std::cout << "failure" << std::endl;
                std::terminate();
            }
        }
    };

    std::unique_ptr<cl::sycl::device_selector> selector;
    if (argc >= 2) {
        if (strcmp(argv[1], "cpu") == 0) {
            selector.reset(new cl::sycl::cpu_selector());
            stream_type = ccl_stream_cpu;
        }
        else if (strcmp(argv[1], "gpu") == 0) {
            stream_type = ccl_stream_gpu;
            if (has_gpu()) {
                selector.reset(new cl::sycl::gpu_selector());
            }
            else if (has_accelerator()) {
                selector.reset(new cl::sycl::host_selector());
                std::cout
                    << "Accelerator is the first in device list, but unavailable for multiprocessing, host_selector has been created instead of default_selector."
                    << std::endl;
            }
            else {
                selector.reset(new cl::sycl::default_selector());
                std::cout
                    << "GPU is unavailable, default_selector has been created instead of gpu_selector."
                    << std::endl;
            }
        }
        else if (strcmp(argv[1], "host") == 0) {
            stream_type = ccl_stream_cpu;
            selector.reset(new cl::sycl::host_selector());
        }
        else if (strcmp(argv[1], "default") == 0) {
            stream_type = ccl_stream_cpu;
            if (!has_accelerator()) {
                selector.reset(new cl::sycl::default_selector());
            }
            else {
                selector.reset(new cl::sycl::host_selector());
                std::cout
                    << "Accelerator is the first in device list, but unavailable for multiprocessing, host_selector has been created instead of default_selector."
                    << std::endl;
            }
        }
        else {
            std::cerr << "Please provide device type: cpu | gpu | host | default " << std::endl;
            return -1;
        }
        queue = cl::sycl::queue(*selector, exception_handler);
        std::cout << "Provided device type " << argv[1] << "\nRunning on "
                  << queue.get_device().get_info<cl::sycl::info::device::name>() << "\n";
    }
    else {
        std::cerr << "Please provide device type: cpu | gpu | host | default " << std::endl;
        return -1;
    }
    return 0;
}

void handle_exception(cl::sycl::queue& q) {
    try {
        q.wait_and_throw();
    }
    catch (cl::sycl::exception const& e) {
        std::cout << "Caught synchronous SYCL exception:\n" << e.what() << std::endl;
    }
}

cl::sycl::usm::alloc usm_alloc_type_from_string(const std::string& str) {
    const std::map<std::string, cl::sycl::usm::alloc> names{ {
        { "HOST", cl::sycl::usm::alloc::host },
        { "DEVICE", cl::sycl::usm::alloc::device },
        { "SHARED", cl::sycl::usm::alloc::shared },
    } };

    auto it = names.find(str);
    if (it == names.end()) {
        std::stringstream ss;
        ss << "Invalid USM type requested: " << str << "\nSupported types are:\n";
        for (const auto& v : names) {
            ss << v.first << ", ";
        }
        throw std::runtime_error(ss.str());
    }
    return it->second;
}

template <class data_native_type, cl::sycl::usm::alloc... types>
struct usm_polymorphic_allocator {
    using native_type = data_native_type;
    using allocator_types = std::tuple<cl::sycl::usm_allocator<native_type, types>...>;
    using integer_usm_type = typename std::underlying_type<cl::sycl::usm::alloc>::type;
    using self_t = usm_polymorphic_allocator<data_native_type, types...>;

    usm_polymorphic_allocator(cl::sycl::queue& q)
            : allocators{ std::make_tuple(cl::sycl::usm_allocator<native_type, types>(q)...) } {}

    ~usm_polymorphic_allocator() {
        for (auto& v : memory_storage) {
            data_native_type* mem = v.first;
            deallocate(mem, v.second.size, v.second.type);
        }
    }

private:
    struct alloc_info {
        size_t size;
        cl::sycl::usm::alloc type;
    };
    std::map<data_native_type*, alloc_info> memory_storage;

    struct alloc_impl {
        alloc_impl(native_type** out_ptr, size_t count, cl::sycl::usm::alloc type, self_t* parent)
                : out_usm_memory_pointer(out_ptr),
                  size(count),
                  alloc_index(0),
                  requested_alloc_type(type),
                  owner(parent) {}

        template <class specific_allocator>
        void operator()(specific_allocator& al) {
            if (alloc_index++ == static_cast<integer_usm_type>(requested_alloc_type)) {
                *out_usm_memory_pointer = al.allocate(size);

                alloc_info info{ size, requested_alloc_type };
                owner->memory_storage.emplace(*out_usm_memory_pointer, info);
            }
        }
        native_type** out_usm_memory_pointer;
        size_t size{};
        int alloc_index{};
        cl::sycl::usm::alloc requested_alloc_type;
        self_t* owner;
    };

    struct dealloc_impl {
        dealloc_impl(native_type** in_ptr, size_t count, cl::sycl::usm::alloc type, self_t* parent)
                : in_usm_memory_pointer(in_ptr),
                  size(count),
                  alloc_index(0),
                  requested_alloc_type(type),
                  owner(parent) {}

        template <class specific_allocator>
        void operator()(specific_allocator& al) {
            if (alloc_index++ == static_cast<integer_usm_type>(requested_alloc_type)) {
                auto it = owner->memory_storage.find(*in_usm_memory_pointer);
                if (it == owner->memory_storage.end()) {
                    throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                             " - not owns memory object");
                }

                al.deallocate(*in_usm_memory_pointer, size);
                *in_usm_memory_pointer = nullptr;

                owner->memory_storage.erase(it);
            }
        }
        native_type** in_usm_memory_pointer;
        size_t size;
        int alloc_index;
        cl::sycl::usm::alloc requested_alloc_type;
        self_t* owner;
    };

public:
    allocator_types allocators;

    native_type* allocate(size_t size, cl::sycl::usm::alloc type) {
        native_type* ret = nullptr;
        ccl_tuple_for_each(allocators, alloc_impl{ &ret, size, type, this });
        return ret;
    }

    void deallocate(native_type* in_ptr, size_t size, cl::sycl::usm::alloc type) {
        ccl_tuple_for_each(allocators, dealloc_impl{ &in_ptr, size, type, this });
    }
};
#endif /* SYCL_BASE_HPP */
