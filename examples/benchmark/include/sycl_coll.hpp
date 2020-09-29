#pragma once

#include "coll.hpp"

#ifdef CCL_ENABLE_SYCL

void set_pinning(const ccl::communicator& comm) {
    // select requested platform by SYCL_BE: L0 or OpenCL
    std::vector<cl::sycl::device> all_devices =
        cl::sycl::device::get_devices(info::device_type::gpu);
    std::vector<cl::sycl::device> selected_devices;
    std::string backend;
    if (getenv("SYCL_BE") == nullptr) {
        backend = "Level-Zero";
    }
    else if (getenv("SYCL_BE") != nullptr) {
        if (std::strcmp(getenv("SYCL_BE"), "PI_LEVEL0") == 0) {
            backend = "Level-Zero";
        }
        else if (std::strcmp(getenv("SYCL_BE"), "PI_OPENCL") == 0) {
            backend = "OpenCL";
        }
        else {
            throw std::runtime_error("invalid backend: " + std::string(getenv("SYCL_BE")));
        }
    }

    for (const auto& device : all_devices) {
        auto platform = device.get_platform();
        auto platform_name = platform.get_info<cl::sycl::info::platform::name>();
        std::size_t found = platform_name.find(backend);
        if (found != std::string::npos)
            selected_devices.push_back(device);
    }
    if (selected_devices.size() <= 0) {
        throw ccl::exception("No selected device found.");
    }
    size_t idx = comm.rank() % selected_devices.size();
    auto selected_device = selected_devices[idx];
    sycl_queue = cl::sycl::queue(selected_device);
    std::cout << "\nRunning on: " << selected_device.get_info<cl::sycl::info::device::name>()
              << " for device id: " << idx << "\n";
}

/* sycl-specific base implementation */
template <class Dtype, class strategy>
struct sycl_base_coll : base_coll, private strategy, device_specific_data {
    using coll_strategy = strategy;

    template <class... Args>
    sycl_base_coll(bench_init_attr init_attr,
                   size_t sbuf_multiplier,
                   size_t rbuf_multiplier,
                   Args&&... args)
            : base_coll(init_attr),
              coll_strategy(std::forward<Args>(args)...) {
        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
            send_bufs[idx] =
                new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * sbuf_multiplier);
            recv_bufs[idx] =
                new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * rbuf_multiplier);
        }

        single_send_buf = new cl::sycl::buffer<Dtype, 1>(
            base_coll::get_single_buf_max_elem_count() * sbuf_multiplier);

        single_recv_buf = new cl::sycl::buffer<Dtype, 1>(
            base_coll::get_single_buf_max_elem_count() * rbuf_multiplier);
    }

    sycl_base_coll(bench_init_attr init_attr) : sycl_base_coll(init_attr, 1, 1) {}

    virtual ~sycl_base_coll() {
        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
            delete static_cast<sycl_buffer_t<Dtype>*>(send_bufs[idx]);
            delete static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[idx]);
        }
        delete static_cast<sycl_buffer_t<Dtype>*>(single_send_buf);
        delete static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf);
    }

    const char* name() const noexcept override {
        return coll_strategy::class_name();
    }

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_exec_attr& attr,
                       req_list_t& reqs) override {
        sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(send_bufs[buf_idx]));
        sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[buf_idx]));
        coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
            comm(),
            count,
            send_buf,
            recv_buf,
            attr,
            reqs,
            stream(),
            coll_strategy::get_op_attr(attr));
    }

    virtual void start_single(size_t count,
                              const bench_exec_attr& attr,
                              req_list_t& reqs) override {
        sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_send_buf));
        sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf));
        coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
            comm(),
            count,
            send_buf,
            recv_buf,
            attr,
            reqs,
            stream(),
            coll_strategy::get_op_attr(attr));
    }

    ccl::datatype get_dtype() const override final {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }

    /* global communicator & stream for all cpu collectives */
    static ccl::communicator& comm() {
        if (!device_specific_data::comm_ptr) {
        }
        return *device_specific_data::comm_ptr;
    }

    static ccl::stream& stream() {
        if (!device_specific_data::stream_ptr) {
        }
        return *device_specific_data::stream_ptr;
    }
};
#endif /* CCL_ENABLE_SYCL */
