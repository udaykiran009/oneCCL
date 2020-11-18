#pragma once
#include <initializer_list>
#include <atomic>

#include "sched/entry/l0/l0_entry.hpp"

//TODO L0 Workaround

namespace native {
template <class native_type, class gpu_comm_impl, ccl::group_split_type topology>
class l0_alltoallv_typed_entry : public base_gpu_entry<native_type,
                                                       gpu_comm_impl,
                                                       topology,
                                                       ccl::device_topology_type::ring,
                                                       ccl_coll_alltoallv> {
public:
    friend class ccl_gpu_comm;
    friend class ccl_virtual_gpu_comm;

    using base = base_gpu_entry<native_type,
                                gpu_comm_impl,
                                topology,
                                ccl::device_topology_type::ring,
                                ccl_coll_alltoallv>;
    using base::parent_communicator;
    using base::comm_addr;
    using base::req;
    using base::status;
    using base::launch_args;
    using base::kernel_router;
    using base::get_ctx;
    using base::get_local_kernel;
    using kernel_main_typed = ring_alltoallv_kernel<native_type>;
    using kernel_ipc_typed = ring_alltoallv_ipc<native_type>;

    using income_data_flag_gpu_type =
        typename std::remove_pointer<typename kernel_main_typed::income_data_flag_arg_type>::type;
    using ready_to_recv_flag_gpu_type =
        typename std::remove_pointer<typename kernel_main_typed::ready_to_recv_flag_arg_type>::type;

    using recv_counts_typed_entry_type = typename std::remove_pointer<
        typename kernel_main_typed::recv_elem_counts_buf_arg_type>::type;
    using recv_offsets_typed_entry_type = typename std::remove_pointer<
        typename kernel_main_typed::recv_elem_offsets_buf_arg_type>::type;

    using proxy_size_flag_gpu_type =
        typename std::remove_pointer<typename kernel_main_typed::proxy_size_flag_arg_type>::type;

    using send_counts_typed_entry_type =
        typename std::remove_pointer<typename kernel_main_typed::send_buf_size_arg_type>::type;
    using send_offsets_typed_entry_type = typename std::remove_pointer<
        typename kernel_main_typed::send_elem_offsets_buf_arg_type>::type;

    static constexpr const char* class_name() noexcept {
        return "L0_ALLTOALLV_TYPED";
    }

    static constexpr ccl_coll_type type() noexcept {
        return ccl_coll_alltoallv;
    }

    l0_alltoallv_typed_entry() = delete;
    l0_alltoallv_typed_entry(
        ccl_sched* sched,
        std::shared_ptr<gpu_comm_impl> comm,
        specific_indexed_device_storage& available_devices,
        ccl_driver_context_ptr in_ctx,
        const ccl_buffer send_buf,
        const size_t* send_counts,
        ccl_buffer recv_buf,
        const size_t* recv_counts,
        std::shared_ptr<ccl_stream> device_stream = std::shared_ptr<ccl_stream>())
            : base(sched,
                   comm,
                   in_ctx,
                   send_buf,
                   ccl::native_type_info<native_type>::dtype,
                   device_stream),
              temp_buffer(
                  this->template alloc_memory_wrap(typename kernel_main_typed::tmp_recv_buf_arg{},
                                                   parent_communicator,
                                                   512,
                                                   get_ctx())),
              // left_wrote_to_me_flag
              income_data_flag(this->template alloc_memory_wrap(
                  typename kernel_main_typed::income_data_flag_arg{},
                  parent_communicator,
                  1,
                  get_ctx())),
              // ready_to_recv_flag_arg
              ready_to_recv_flag(this->template alloc_memory_wrap(
                  typename kernel_main_typed::ready_to_recv_flag_arg{},
                  parent_communicator,
                  1,
                  get_ctx())),
              proxy_size_flag_entry(this->template alloc_memory_wrap(
                  typename kernel_main_typed::proxy_size_flag_arg{},
                  parent_communicator,
                  1,
                  get_ctx())),
              recv_counts_buf(parent_communicator->get_device()
                                  .template alloc_memory<recv_counts_typed_entry_type>(
                                      512,
                                      sizeof(recv_counts_typed_entry_type),
                                      get_ctx())),
              recv_offsets_buf(parent_communicator->get_device()
                                   .template alloc_memory<recv_offsets_typed_entry_type>(
                                       comm_addr.size,
                                       sizeof(recv_offsets_typed_entry_type),
                                       get_ctx())),
              send_counts_buf(parent_communicator->get_device()
                                  .template alloc_memory<recv_counts_typed_entry_type>(
                                      512,
                                      sizeof(recv_counts_typed_entry_type),
                                      get_ctx())),
              send_offsets_buf(parent_communicator->get_device()
                                   .template alloc_memory<send_offsets_typed_entry_type>(
                                       comm_addr.size,
                                       sizeof(send_offsets_typed_entry_type),
                                       get_ctx()))

    {
        // copy recv_buf into recv_buf_entry
        recv_buf_entry = recv_buf;

        // same as parent_communicator->template
        //                    get_comm_data<base::get_topology(),
        //                    base::get_topology_class()>().size;
        int local_topology_size = comm_addr.size;
        std::vector<size_t> recv_offsets_v(local_topology_size, 0);

        for (int idx = 0; idx < local_topology_size; idx++) {
            if (idx > 0)
                recv_offsets_v[idx] += recv_offsets_v[idx - 1] + recv_counts[idx - 1];
        }

        std::vector<size_t> send_offsets_v(local_topology_size, 0);
        for (int idx = 0; idx < local_topology_size; idx++) {
            if (idx > 0)
                send_offsets_v[idx] += send_offsets_v[idx - 1] + send_counts[idx - 1];
        }
        // recv
        recv_counts_buf.enqueue_write_sync(recv_counts, local_topology_size);
        recv_offsets_buf.enqueue_write_sync(recv_offsets_v);
        // send
        send_counts_buf.enqueue_write_sync(send_counts, local_topology_size);
        send_offsets_buf.enqueue_write_sync(send_offsets_v);
        // flag
        proxy_size_flag_entry.enqueue_write_sync({ (int)0 });

        int next_rank = (comm_addr.rank + 1) % comm_addr.size;
        kernel_router = base::template create_kernel_router_for_rank<
            l0_alltoallv_typed_entry<native_type, gpu_comm_impl, topology>>(
            *this, next_rank, available_devices);

        ENTRY_LOG_DEBUG("Init phase of current entry for ext_rank:", next_rank);

        // Once we filled our local parameters, we go wait for another entry to set its
        // parameters so we can use them
        this->set_state(gpu_entry_state::created);
    }

    ~l0_alltoallv_typed_entry() {
        // TODO: remove the memory once the entry is destroyed if it's not cleared automatically
        // TODO: should we destroy handles here?
    }

    void start() override {
        LOG_DEBUG(class_name(), " entry req ", &req, ", rank: ", comm_addr.to_string());

        //Create base primitives
        base::start();

        auto& main_entry_function = get_local_kernel();

        auto recv_buf_ptr = reinterpret_cast<native_type*>(recv_buf_entry.get_ptr());
        // auto send_counts_ptr = reinterpret_cast<size_t*>(send_counts_entry.get_ptr());
        //create implementation specified primitives
        main_entry_function.template set_args<typename kernel_main_typed::tmp_recv_buf_arg,
                                              typename kernel_main_typed::income_data_flag_arg,
                                              typename kernel_main_typed::ready_to_recv_flag_arg,
                                              typename kernel_main_typed::recv_buf_arg,
                                              typename kernel_main_typed::recv_elem_counts_buf_arg,
                                              typename kernel_main_typed::recv_elem_offsets_buf_arg,
                                              typename kernel_main_typed::proxy_size_flag_arg,
                                              typename kernel_main_typed::send_buf_size_arg>(
            temp_buffer.get(),
            income_data_flag.get(),
            ready_to_recv_flag.get(),
            recv_buf_ptr,
            recv_counts_buf.get(),
            recv_offsets_buf.get(),
            proxy_size_flag_entry.get(),
            send_counts_buf.get());

        // Once we filled our local parameters, we go wait for another entry to set its
        // parameters so we can use them
        this->set_state(gpu_entry_state::wait_for_entry);

        //make sure, that kernel ready for launch
        this->submit_for_execution();
        status = ccl_sched_entry_status_started;
    }

    const char* name() const override {
        return class_name();
    }

    std::vector<ccl_device::device_ipc_memory_handle> get_ipc_data() override {
        ccl_device& owned_device = parent_communicator->get_device();

        //TODO
        std::vector<ccl_device::device_ipc_memory_handle> ret;
        ret.reserve(3);
        ret.push_back(owned_device.create_ipc_memory_handle(income_data_flag.get(), get_ctx()));
        ret.push_back(owned_device.create_ipc_memory_handle(ready_to_recv_flag.get(), get_ctx()));
        return ret;
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        base::dump_detail(str);
    }

private:
    ccl_device::device_memory<native_type> temp_buffer;
    ccl_device::device_memory<income_data_flag_gpu_type> income_data_flag;
    ccl_device::device_memory<ready_to_recv_flag_gpu_type> ready_to_recv_flag;
    ccl_device::device_memory<proxy_size_flag_gpu_type> proxy_size_flag_entry;
    ccl_buffer recv_buf_entry;
    ccl_device::device_memory<recv_counts_typed_entry_type> recv_counts_buf;
    ccl_device::device_memory<recv_offsets_typed_entry_type> recv_offsets_buf;
    ccl_device::device_memory<send_counts_typed_entry_type> send_counts_buf;
    ccl_device::device_memory<send_offsets_typed_entry_type> send_offsets_buf;
    std::shared_ptr<ccl_context> ctx;

public:
    bool execute(kernel_main_typed& main_entry_function, kernel_main_typed& right_kernel) {
        //Check argument binding in kernels for next rank
        bool is_right_kernel_ready =
            right_kernel.template test_args<typename kernel_main_typed::tmp_recv_buf_arg,
                                            typename kernel_main_typed::income_data_flag_arg,
                                            typename kernel_main_typed::ready_to_recv_flag_arg,
                                            typename kernel_main_typed::proxy_size_flag_arg>();
        if (is_right_kernel_ready) {
            //TODO do not get arguments sequencially - use array version instead
            typename kernel_main_typed::tmp_recv_buf_arg::return_t right_tmp_recv_buf_arg =
                right_kernel.template get_arg<typename kernel_main_typed::tmp_recv_buf_arg>();
            typename kernel_main_typed::income_data_flag_arg::return_t right_income_data_flag_arg =
                right_kernel.template get_arg<typename kernel_main_typed::income_data_flag_arg>();
            typename kernel_main_typed::ready_to_recv_flag_arg::return_t
                right_ready_to_recv_flag_arg =
                    right_kernel
                        .template get_arg<typename kernel_main_typed::ready_to_recv_flag_arg>();

            typename kernel_main_typed::proxy_size_flag_arg::return_t right_proxy_size_flag_arg =
                right_kernel.template get_arg<typename kernel_main_typed::proxy_size_flag_arg>();

            ENTRY_LOG_DEBUG("Bind final arguments for kernel: ", kernel_main_typed::name());
            ENTRY_LOG_TRACE("Args: \n{ ",
                            right_tmp_recv_buf_arg.first,
                            ", ",
                            right_tmp_recv_buf_arg.second,
                            "}\n",
                            "{ ",
                            right_income_data_flag_arg.first,
                            ", ",
                            right_income_data_flag_arg.second,
                            "}\n",
                            "{ ",
                            right_ready_to_recv_flag_arg.first,
                            ", ",
                            right_ready_to_recv_flag_arg.second,
                            "}\n");

            //TODO register argument for current device kernel: use array-version
            main_entry_function
                .template set_args<typename kernel_main_typed::right_tmp_recv_buf_arg,
                                   typename kernel_main_typed::right_income_data_flag_arg,
                                   typename kernel_main_typed::right_ready_to_recv_flag_arg,
                                   typename kernel_main_typed::right_proxy_size_flag_arg>(
                    right_tmp_recv_buf_arg.second,
                    right_income_data_flag_arg.second,
                    right_ready_to_recv_flag_arg.second,
                    right_proxy_size_flag_arg.second);
            ENTRY_LOG_DEBUG("Final Function: ", main_entry_function.to_string());
        }
        return is_right_kernel_ready;
    }

    bool execute(kernel_main_typed& main_entry_function, kernel_ipc_typed& right_kernel) {
        bool is_right_kernel_ready = false;
        /* TODO UNSUPPORTED

        //Check argument binding in kernels for next rank
        bool is_right_kernel_ready =
            right_kernel.template test_args< //typename kernel_ipc_typed::right_output_buf_arg,
                typename kernel_ipc_typed::income_data_flag_arg,
                typename kernel_ipc_typed::ready_to_recv_flag_arg>();
        if (is_right_kernel_ready) {
            //TODO do not get arguments sequencially - use array version instead
            typename kernel_main_typed::right_output_buf_arg::return_t right_output_buf_arg =
                right_kernel.template get_arg<typename kernel_ipc_typed::right_output_buf_arg>();
            typename kernel_main_typed::income_data_flag_arg::return_t right_income_data_flag_arg =
                right_kernel.template get_arg<typename kernel_ipc_typed::income_data_flag_arg>();
            typename kernel_main_typed::ready_to_recv_flag_arg::return_t
                right_ready_to_recv_flag_arg =
                    right_kernel
                        .template get_arg<typename kernel_ipc_typed::ready_to_recv_flag_arg>();

            LOG_DEBUG("entry: ",
                      class_name(),
                      ", rank: ",
                      comm_addr.to_string(),
                      ", bind elapsed arguments for kernel: ",
                      kernel_main_typed::name());
            LOG_TRACE("Args: \n{ ",
                      right_output_buf_arg.first,
                      ", ",
                      right_output_buf_arg.second,
                      "}\n",
                      "{ ",
                      right_income_data_flag_arg.first,
                      ", ",
                      right_income_data_flag_arg.second,
                      "}\n",
                      "{ ",
                      right_ready_to_recv_flag_arg.first,
                      ", ",
                      right_ready_to_recv_flag_arg.second,
                      "}\n");

            //TODO register argument for current device kernel: user array version
            main_entry_function
                .template set_args< //typename kernel_main_typed::right_output_buf_arg,
                    typename kernel_main_typed::right_income_data_flag_arg,
                    typename kernel_main_typed::right_ready_to_recv_flag_arg>(
                    //right_output_buf_arg.second,
                    right_income_data_flag_arg.second,
                    right_ready_to_recv_flag_arg.second);
            LOG_TRACE("Set right_output_buf_arg",
                      "Set right_income_data_flag_arg",
                      "Set right_ready_to_recv_flag_arg");
            LOG_DEBUG("entry: ",
                      class_name(),
                      ", rank: ",
                      comm_addr.to_string(),
                      ". Function: ",
                      main_entry_function.to_string());
        }*/
        return is_right_kernel_ready;
    }
};
} // namespace native
