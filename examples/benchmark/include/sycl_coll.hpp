#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>

#include "coll.hpp"

#ifdef CCL_ENABLE_SYCL

#include <CL/sycl.hpp>

using namespace sycl;
using namespace sycl::access;

std::vector<cl::sycl::queue> get_sycl_queues(std::vector<size_t>& ranks) {

    /* select requested platform by SYCL_BE: L0 or OpenCL */
    std::vector<cl::sycl::device> all_devices =
        cl::sycl::device::get_devices(info::device_type::gpu);
    std::vector<cl::sycl::device> platform_devices;
    std::vector<cl::sycl::device> selected_devices;
    std::vector<cl::sycl::queue> selected_queues;
    std::string backend;

    if (getenv("SYCL_BE") == nullptr) {
        backend = "Level-Zero";
    }
    else if (getenv("SYCL_BE") != nullptr) {
        if (std::strcmp(getenv("SYCL_BE"), "PI_LEVEL_ZERO") == 0) {
            backend = "Level-Zero";
        }
        else if (std::strcmp(getenv("SYCL_BE"), "PI_OPENCL") == 0) {
            backend = "OpenCL";
        }
        else {
            throw std::runtime_error("invalid backend: " + std::string(getenv("SYCL_BE")));
        }
    }

    /* filter by platform */
    for (const auto& device : all_devices) {
        auto platform = device.get_platform();
        auto platform_name = platform.get_info<cl::sycl::info::platform::name>();
        std::size_t found = platform_name.find(backend);
        if (found != std::string::npos)
            platform_devices.push_back(device);
    }

    if (platform_devices.size() <= 0) {
        throw ccl::exception("no selected device found for SYCL backend: " + backend);
    }

    /* filter by sub-devices */
    for (const auto& device : platform_devices) {

        size_t max_sub_devices = device.template get_info<cl::sycl::info::device::partition_max_sub_devices>();
        printf("max_sub_devices %zu\n", max_sub_devices);

        std::vector<sycl::device> sub_devices;
        try {
            sub_devices =
                device.create_sub_devices<cl::sycl::info::partition_property::partition_by_affinity_domain>(
                    info::partition_affinity_domain::next_partitionable);
            printf("created sub_devices\n");

            for (const auto& sub_device : sub_devices) {
                ASSERT(sub_device.template get_info<cl::sycl::info::device::parent_device>() == device,
                    "unexpected sub-device's parent");

                max_sub_devices = sub_device.template get_info<cl::sycl::info::device::partition_max_sub_devices>();
                printf("max_sub_devices for child: %zu\n", max_sub_devices);
            }

            sub_devices = std::vector<cl::sycl::device>(1, device);
            printf("partition property is not supported, but create_sub_devices doesn't throw exception yet\n"
                "use parent_device as sub_device\n");
        }
        catch (std::exception& e) {
            sub_devices = std::vector<cl::sycl::device>(1, device);
            printf("partition property is not supported, use parent_device as sub_device\n");
        }

        printf("sub_devices size %zu\n", sub_devices.size());
        selected_devices.insert(selected_devices.end(), sub_devices.begin(), sub_devices.end());
    }

    printf("selected_devices count %zu\n", selected_devices.size());

    for (auto rank : ranks) {
        size_t idx = rank % selected_devices.size();
        auto device = selected_devices[idx];
        std::cout << "\nrunning on: " << device.get_info<cl::sycl::info::device::name>()
                  << ", device index: " << idx << "\n";
        selected_queues.push_back(cl::sycl::queue(device));
    }

    return selected_queues;
}

usm::alloc usm_alloc_type_from_string(const std::string& str) {
    const std::map<std::string, usm::alloc> names{ {
        { "host", usm::alloc::host },
        { "device", usm::alloc::device },
        { "shared", usm::alloc::shared },
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

template <typename T>
struct buf_allocator {

    const size_t alignment = 64;

    buf_allocator(queue& q)
        : q(q)
    {}

    ~buf_allocator() {
        for (auto& ptr : memory_storage) {
            cl::sycl::free(ptr, q);
        }
    }

    T* allocate(size_t count, usm::alloc alloc_type) {
        T* ptr = nullptr;
        if (alloc_type == usm::alloc::host)
            ptr = aligned_alloc_host<T>(alignment, count, q);
        else if (alloc_type == usm::alloc::device)
            ptr = aligned_alloc_device<T>(alignment, count, q);
        else if (alloc_type == usm::alloc::shared)
            ptr = aligned_alloc_shared<T>(alignment, count, q);
        else
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + "unexpected alloc_type");

        auto it = memory_storage.find(ptr);
        if (it != memory_storage.end()) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                        " - allocator already owns this pointer");
        }
        memory_storage.insert(ptr);

        auto pointer_type = cl::sycl::get_pointer_type(ptr, q.get_context());
        ASSERT(pointer_type == alloc_type, "pointer_type %d doesn't match with requested %d",
            pointer_type, alloc_type);

        return ptr;
    }

    void deallocate(T* ptr) {
        auto it = memory_storage.find(ptr);
        if (it == memory_storage.end()) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                        " - allocator doesn't own this pointer");
        }
        free(ptr, q);
        memory_storage.erase(it);
    }

    sycl::queue q;
    std::set<T*> memory_storage;
};

/* sycl-specific base implementation */
template <class Dtype, class strategy>
struct sycl_base_coll : base_coll, private strategy {
    using coll_strategy = strategy;

    template <class... Args>
    sycl_base_coll(bench_init_attr init_attr,
                   size_t sbuf_multiplier,
                   size_t rbuf_multiplier,
                   Args&&... args)
            : base_coll(init_attr),
              coll_strategy(std::forward<Args>(args)...) {

        auto& transport = transport_data::instance();
        auto streams = transport.get_streams();

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {

                allocators.push_back(buf_allocator<Dtype>(streams[rank_idx].get_native()));

                auto& allocator = allocators[rank_idx];

                sycl::usm::alloc usm_alloc_type;
                auto bench_alloc_type = base_coll::get_sycl_usm_type();
                if (bench_alloc_type == SYCL_USM_SHARED)
                    usm_alloc_type = usm::alloc::shared;
                else if (bench_alloc_type == SYCL_USM_DEVICE)
                    usm_alloc_type = usm::alloc::device;
                else
                    ASSERT(0, "unexpected bench_alloc_type %d", bench_alloc_type);

                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    send_bufs[idx][rank_idx] = allocator.allocate(
                        base_coll::get_max_elem_count() * sbuf_multiplier, usm_alloc_type);
                    recv_bufs[idx][rank_idx] = allocator.allocate(
                        base_coll::get_max_elem_count() * rbuf_multiplier, usm_alloc_type);
                }

                single_send_buf[rank_idx] = allocator.allocate(
                    base_coll::get_single_buf_max_elem_count() * sbuf_multiplier, usm_alloc_type);
                single_recv_buf[rank_idx] = allocator.allocate(
                    base_coll::get_single_buf_max_elem_count() * rbuf_multiplier, usm_alloc_type);
            }
            else {
                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    send_bufs[idx][rank_idx] =
                        new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * sbuf_multiplier);
                    recv_bufs[idx][rank_idx] =
                        new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * rbuf_multiplier);
                }

                single_send_buf[rank_idx] = new cl::sycl::buffer<Dtype, 1>(
                    base_coll::get_single_buf_max_elem_count() * sbuf_multiplier);

                single_recv_buf[rank_idx] = new cl::sycl::buffer<Dtype, 1>(
                    base_coll::get_single_buf_max_elem_count() * rbuf_multiplier);
            }
        }
    }

    sycl_base_coll(bench_init_attr init_attr) : sycl_base_coll(init_attr, 1, 1) {}

    virtual ~sycl_base_coll() {

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_BUF) {
                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    delete static_cast<sycl_buffer_t<Dtype>*>(send_bufs[idx][rank_idx]);
                    delete static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[idx][rank_idx]);
                }
                delete static_cast<sycl_buffer_t<Dtype>*>(single_send_buf[rank_idx]);
                delete static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf[rank_idx]);
            }
        }
    }

    const char* name() const noexcept override {
        return coll_strategy::class_name();
    }

    virtual void prepare(size_t elem_count) override {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            prepare_internal(elem_count,
                                   comms[rank_idx],
                                   streams[rank_idx],
                                   rank_idx);
        }
    }

    virtual void finalize(size_t elem_count) override {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            finalize_internal(elem_count,
                                    comms[rank_idx],
                                    streams[rank_idx],
                                    rank_idx);
        }
    }

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_exec_attr& attr,
                       req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {
                coll_strategy::start_internal(
                    comms[rank_idx],
                    count,
                    static_cast<Dtype*>(send_bufs[buf_idx][rank_idx]),
                    static_cast<Dtype*>(recv_bufs[buf_idx][rank_idx]),
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
            else {
                sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(send_bufs[buf_idx][rank_idx]));
                sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[buf_idx][rank_idx]));
                coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
                    comms[rank_idx],
                    count,
                    send_buf,
                    recv_buf,
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
        }
    }

    virtual void start_single(size_t count,
                              const bench_exec_attr& attr,
                              req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {
                coll_strategy::start_internal(
                    comms[rank_idx],
                    count,
                    static_cast<Dtype*>(single_send_buf[rank_idx]),
                    static_cast<Dtype*>(single_recv_buf[rank_idx]),
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
            else
            {
                sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_send_buf[rank_idx]));
                sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf[rank_idx]));
                coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
                    comms[rank_idx],
                    count,
                    send_buf,
                    recv_buf,
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
        }
    }

    ccl::datatype get_dtype() const override final {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }

private:
    std::vector<buf_allocator<Dtype>> allocators;
};

#endif /* CCL_ENABLE_SYCL */
