#pragma once
#include "oneapi/ccl/types.hpp"
#include "common/datatype/datatype.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/native_device_api/l0/primitives.hpp"
#include "common/comm/l0/modules/kernel_functions.hpp"

#include "oneapi/ccl.hpp"

#include "comp/comp.hpp"
#include "common/comm/l0/devices/devices_declaration.hpp"
#include "sched/entry/coll/direct/base_coll_entry.hpp"

#include "common/comm/l0/modules_connector.hpp"
#include "common/global/global.hpp"
#include "common/stream/stream.hpp"
//TODO L0 Workaround
#include <unistd.h>
static std::mutex global_fence_mutex;

#define ENTRY_LOG_TRACE(...) \
    if (unlikely(logger.get_log_level() >= ccl_log_level::TRACE)) { \
        do { \
            std::stringstream ss; \
            this->dump_detail(ss); \
            logger.trace("|TRACE| ", \
                         basedir_static(__FILE__), \
                         ":", \
                         __LINE__, \
                         "  ", \
                         ss.str(), \
                         " - ", \
                         ##__VA_ARGS__); \
        } while (0); \
    }

#define ENTRY_LOG_DEBUG(...) \
    if (unlikely(logger.get_log_level() >= ccl_log_level::DEBUG)) { \
        do { \
            std::stringstream ss; \
            this->dump_detail(ss); \
            logger.debug("|DEBUG| ", \
                         basedir_static(__FILE__), \
                         ":", \
                         __LINE__, \
                         "  ", \
                         ss.str(), \
                         " - ", \
                         ##__VA_ARGS__); \
        } while (0); \
    }

#define ENTRY_LOG_INFO(...) \
    if (unlikely(logger.get_log_level() >= ccl_log_level::INFO)) { \
        do { \
            std::stringstream ss; \
            this->dump_detail(ss); \
            logger.info("|INFO| ", \
                        basedir_static(__FILE__), \
                        ":", \
                        __LINE__, \
                        "  ", \
                        ss.str(), \
                        " - ", \
                        ##__VA_ARGS__); \
        } while (0); \
    }

// This is a internal gpu entry state to keep track of the progress
// filling and submitting a kernel as well as its execution
enum class gpu_entry_state {
    // default state
    initial,
    // Entry is fully constructed
    created,
    // Local parameters are filled and the entry is waiting for
    // parameters from the neighbour entry. No further progress
    // until these parameters are received. After that
    // the entry appends it's kernel. After that the kernel is
    // submited to the queue(only one rank does that in case of
    // virtual device.
    wait_for_entry,
    // The kernel is submited and the entry is waiting for kernel
    // completion by checking fence status.
    wait_for_completion,
    // Execution is done, it's possible to reuse the entry by
    // moving the entry to created state
    completed,
    // Last element in the enum, not used as state
    last
};

inline std::string to_string(gpu_entry_state state) {
    return utils::enum_to_str<static_cast<int>(gpu_entry_state::last)>{
        "initial", "created", "wait_for_entry", "wait_for_completion", "completed"
    }
        .choose(state);
}

namespace native {
template <class native_type,
          class gpu_comm_impl,
          ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          ccl_coll_type type_op>
class base_gpu_entry : public sched_entry {
public:
    using gpu_comm = gpu_comm_impl;
    using processing_type = native_type;
    using kernel_main_typed =
        typename gpu_comm::template gpu_kernel_t<type_op, group_id, class_id, native_type>;
    // using kernel_ipc_typed = ring_allreduce_ipc<native_type>;

    template <class elem_t>
    using device_memory = memory<elem_t, ccl_device, ccl_context>;

    friend class ccl_gpu_comm;
    friend class ccl_virtual_gpu_comm;
    static constexpr const char *class_name() noexcept {
        return ccl_coll_type_to_str(type_op);
    }
    static constexpr ccl_coll_type type() noexcept {
        return type_op;
    }

    static constexpr ccl::group_split_type get_topology() {
        return group_id;
    }

    static constexpr ccl::device_topology_type get_topology_class() {
        return class_id;
    }

    base_gpu_entry() = delete;
    base_gpu_entry(ccl_sched *sched,
                   std::shared_ptr<gpu_comm> comm,
                   ccl_driver_context_ptr in_ctx,
                   const ccl_buffer send_buf,
                   ccl::datatype dtype_in,
                   std::shared_ptr<ccl_stream> &stream)
            : sched_entry(sched),
              parent_communicator(comm),
              comm_addr(parent_communicator
                            ->template get_comm_data<get_topology(), get_topology_class()>()),
              send_buf(send_buf),
              dtype(dtype_in),
              device_stream(stream),
              ctx(in_ctx),
              entry_state(gpu_entry_state::initial) {
        // TODO: remove once all the child entries are refactored to not
        // use fence field directly
        fence = get_fence();
    }

    virtual ~base_gpu_entry() {}

    virtual void start() override {
        ccl_device &device = parent_communicator->get_device();
        {
            //TODO make check, that device_stream belong to the device
            auto queue_prop = ccl_device::get_default_queue_desc();
            auto &cmd_queue = device.get_cmd_queue(queue_prop, ctx);

            auto fence = parent_communicator->get_fence(get_queue(), get_ctx());

            ENTRY_LOG_DEBUG("start base entry initialization, ctx: ",
                            ctx.get(),
                            ", queue: ",
                            cmd_queue.get(),
                            ", fence: ",
                            fence.get());
        }
        //else
        //{
        //zeCommandListReset(copy_send_cmd_list->handle); // ready for command
        //zeCommandListReset(copy_recv_cmd_list->handle); // ready for command
        //zeCommandListReset(exec_cmd_list->handle); // ready for command
        //}

        //set kernel args for main kernel on current device
        kernel_main_typed &main_entry_function =
            parent_communicator->template register_entry<native_type, group_id, class_id>(*this);

        auto send_buf_ptr = reinterpret_cast<native_type *>(send_buf.get_ptr());

        main_entry_function.template set_args<typename kernel_main_typed::common_entry_buf_arg>(
            send_buf_ptr);

        /*ze_result_t result = zeCommandListAppendLaunchKernel(exec_cmd_list->handle, main_entry_function.handle, &launch_args, nullptr, 0, nullptr);
        if(result != ZE_RESULT_SUCCESS)
        {
            LOG_ERROR("zeCommandListAppendLaunchKernel failed, error: ", to_string(result));
            throw std::runtime_error("zeCommandListAppendLaunchKernel failed");
        }

        / * result = zeCommandListClose(exec_cmd_list->handle);
        if(result != ZE_RESULT_SUCCESS)
        {
            LOG_ERROR("zeCommandListClose failed, error: ", to_string(result));
            throw std::runtime_error("zeCommandListClose failed");
        }*/

        //make sure, that kernel ready for launch

        status = ccl_sched_entry_status_started;
        ENTRY_LOG_DEBUG("started");
    }

    bool submit_for_execution() {
        ready_to_exec = finalize_entry();
        ENTRY_LOG_TRACE("submission result: ", ready_to_exec);
        return ready_to_exec;
    }

    virtual void update() override {
        if (!ready_to_exec) {
            submit_for_execution();
        }
        else {
            //wait execution
            ccl_device &device = parent_communicator->get_device();
            auto queue_prop = ccl_device::get_default_queue_desc();
            auto &cmd_queue = device.get_cmd_queue(queue_prop, ctx);

            ENTRY_LOG_TRACE(" waiting for finished execution, queue: ", cmd_queue.get());

            ze_result_t ret = get_fence_impl().query_status();
            ENTRY_LOG_TRACE(
                "Fence query status: ", native::to_string(ret), ", queue: ", cmd_queue.get());
            if (ret == ZE_RESULT_SUCCESS) {
                this->set_state(gpu_entry_state::completed);

                // Once all the ranks in the group got the notification, reset the state for further launches
                reset_state();

                status = ccl_sched_entry_status_complete;
                ENTRY_LOG_DEBUG(" Completed on queue: ", cmd_queue.get());
            }
            else if (ret == ZE_RESULT_NOT_READY) {
                // Just return in case if the kernel is not ready yet, will check again on the next iteration
                return;
            }
        }
    }

    virtual const char *name() const override {
        return class_name();
    }

    ccl_device::device_queue &get_queue() const {
        ccl_device &device = parent_communicator->get_device();
        return device.get_cmd_queue(ccl_device::get_default_queue_desc(), ctx);
    }

    ze_fence_handle_t get_fence() {
        return get_fence_impl().get();
    }

protected:
    virtual bool finalize_entry() = 0;
    virtual void dump_detail(std::stringstream &str) const override {
        ccl_logger::format(str, "{", name(), ", addr: ", comm_addr.to_string(), "}");
    }

    void reset_state() {
        // Reset the state of the used handles
        get_fence_impl().reset();
        parent_communicator->get_cmd_list(get_ctx()).reset();
    }

protected:
    ccl_driver_context_ptr get_ctx() const {
        return ctx;
    }

    template <class options>
    ze_device_mem_alloc_desc_t get_mem_descr(options opt) {
        ze_device_mem_alloc_desc_t mem_descr = ccl_device::get_default_mem_alloc_desc();
        // Explicitly reset flags to avoid potential conflicts with the default value
        mem_descr.flags = 0;
        mem_descr.flags |= (opt.is_uncached() ? ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED
                                              : ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED);

        return mem_descr;
    }

    // Wrapper to handle memory allocation with different options
    template <class kernel_arg,
              // Get the actual underlying type to specify the type for the allocation
              class arg_type = typename std::remove_pointer<typename kernel_arg::arg_type>::type>
    device_memory<arg_type> alloc_memory_wrap(const kernel_arg &arg,
                                              std::shared_ptr<gpu_comm> parent_communicator,
                                              size_t cnt,
                                              std::shared_ptr<ccl_context> ctx) {
        auto mem_descr = get_mem_descr(typename kernel_arg::options_t{});
        auto memory = parent_communicator->get_device().template alloc_memory<arg_type>(
            cnt, sizeof(arg_type), ctx, mem_descr);

        LOG_DEBUG("Allocation memory by default: ",
                  kernel_arg::index,
                  ", ctx: ",
                  (void *)ctx.get(),
                  ", memory: ",
                  (void *)memory.get(),
                  ", mem_descr: ",
                  native::to_string(mem_descr));

        return memory;
    }

    std::shared_ptr<gpu_comm> parent_communicator;
    topology_addr<group_id, class_id> comm_addr;
    ccl_buffer send_buf;
    ccl::datatype dtype;
    atl_req_t req{};
    std::shared_ptr<ccl_stream> device_stream;
    // GPU
    bool ready_to_exec = false;
    ze_fence_handle_t fence;

    auto get_fence_impl() -> decltype(parent_communicator->get_fence(get_queue(), get_ctx())) {
        return parent_communicator->get_fence(get_queue(), get_ctx());
    }

    void set_state(gpu_entry_state new_state) noexcept {
        ENTRY_LOG_DEBUG(
            "switching entry state from ", to_string(entry_state), " to ", to_string(new_state));
        entry_state = new_state;
    }

    gpu_entry_state get_state() const noexcept {
        return entry_state;
    }

    //TODO
    ze_group_count_t launch_args = { 1, 1, 1 };

    template <class executor>
    static std::unique_ptr<base_connector_interface<kernel_main_typed>>
    create_kernel_router_for_rank(executor &exec,
                                  int next_rank,
                                  specific_indexed_device_storage &group_devices) {
        std::unique_ptr<base_connector_interface<kernel_main_typed>> kernel_router;
        while (!kernel_router) {
            //Gather data from in-process GPU
            auto &map_devices = std::get<ccl_gpu_comm::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if (it == map_devices.end()) {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_gpu_comm> gpu = it->second;
            kernel_main_typed &right_main_func =
                gpu->get_gpu_kernel<type(), get_topology(), get_topology_class(), native_type>();

            //communicate with real device
            kernel_router.reset(
                new kernel_connector<kernel_main_typed, executor, kernel_main_typed>(
                    exec, right_main_func));
        }

        while (!kernel_router) {
            //concurrent GPU
            using thread_group_comm_device = ccl_thread_comm<ccl_gpu_comm>;
            native::indexed_device_container<thread_group_comm_device> &map_devices =
                std::get<thread_group_comm_device::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if (it == map_devices.end()) {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_thread_comm<ccl_gpu_comm>> gpu = it->second;

            /*std::shared_ptr<thread_group_comm_device> gpu = map_devices.find(next_rank);
            if(gpu == nullptr)
            {
                break; // not ready yet!
            }*/
            kernel_main_typed &right_main_func =
                gpu->get_gpu_kernel<type(), get_topology(), get_topology_class(), native_type>();

            //communicate with real device from another thread
            kernel_router.reset(
                new kernel_connector<kernel_main_typed, executor, kernel_main_typed>(
                    exec, right_main_func));
        }

        while (!kernel_router) {
            //concurrent GPU
            using thread_group_comm_device = ccl_thread_comm<ccl_virtual_gpu_comm>;
            native::indexed_device_container<thread_group_comm_device> &map_devices =
                std::get<thread_group_comm_device::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if (it == map_devices.end()) {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_thread_comm<ccl_virtual_gpu_comm>> gpu = it->second;
            /*
            std::shared_ptr<thread_group_comm_device> gpu = map_devices.find(next_rank);
            if(gpu == nullptr)
            {
                break; // not ready yet!
            }*/
            kernel_main_typed &right_main_func =
                gpu->get_gpu_kernel<type(), get_topology(), get_topology_class(), native_type>();

            //communicate with virtual device from another thread
            kernel_router.reset(
                new kernel_connector<kernel_main_typed, executor, kernel_main_typed>(
                    exec, right_main_func));
        }
        /*
        while(!kernel_router)
        {
            //ipc-source GPU
            using thread_group_comm_device = ccl_ipc_source_gpu_comm<ccl_gpu_comm>;
            native::indexed_device_container<thread_group_comm_device>& map_devices = std::get<thread_group_comm_device::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if(it == map_devices.end())
            {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_ipc_source_gpu_comm<ccl_gpu_comm>> gpu = it->second;
            kernel_main_typed &right_main_func = gpu->get_gpu_kernel<type(), native_type>();

            //communicate with real device from another thread
            kernel_router.reset(new router<kernel_main_typed,
                                           executor,
                                           kernel_main_typed>(exec, right_main_func));
        }

        while(!kernel_router)
        {
            //concurrent GPU
            using thread_group_comm_device = ccl_ipc_source_gpu_comm<ccl_virtual_gpu_comm>;
            native::indexed_device_container<thread_group_comm_device>& map_devices = std::get<thread_group_comm_device::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if(it == map_devices.end())
            {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_ipc_source_gpu_comm<ccl_virtual_gpu_comm>> gpu = it->second;
            kernel_main_typed &right_main_func = gpu->get_gpu_kernel<type(), native_type>();

            //communicate with virtual device from another thread
            kernel_router.reset(new router<kernel_main_typed,
                                           executor,
                                           kernel_main_typed>(exec, right_main_func));
        }
*/
        while (!kernel_router) {
            //Virtual GPU
            auto &map_devices = std::get<ccl_virtual_gpu_comm::type_idx()>(group_devices);
            auto it = map_devices.find(next_rank);
            if (it == map_devices.end()) {
                break; // not ready yet!
            }
            std::shared_ptr<ccl_virtual_gpu_comm> gpu = it->second;
            kernel_main_typed &right_main_func =
                gpu->get_gpu_kernel<type(), get_topology(), get_topology_class(), native_type>();
            kernel_router.reset(
                new kernel_connector<kernel_main_typed, executor, kernel_main_typed>(
                    exec, right_main_func));
        }

        // TODO: check for launching ipc kernel
        // while (!kernel_router) {
        //     //gather data for ipc-GPU
        //     auto &map_devices = std::get<ccl_ipc_gpu_comm::type_idx()>(group_devices);
        //     auto it = map_devices.find(next_rank);
        //     if (it == map_devices.end()) {
        //         break; // not ready yet!
        //     }
        //     std::shared_ptr<ccl_ipc_gpu_comm> gpu = it->second;
        //     kernel_ipc_typed &right_main_func =
        //         gpu->get_gpu_kernel<type(), get_topology(), get_topology_class(), native_type>();
        //     kernel_router.reset(new kernel_connector<kernel_main_typed, executor, kernel_ipc_typed>(
        //         exec, right_main_func));
        // }

        //sanity
        if (!kernel_router) {
            LOG_ERROR("Cannot bind communicators in group for next rank: ", next_rank);
        }
        return kernel_router;
    }

    std::unique_ptr<base_connector_interface<kernel_main_typed>> kernel_router;

private:
    ccl_driver_context_ptr ctx;
    // Internal gpu entry state to keep track of kernel status, it's not directly related to status field
    gpu_entry_state entry_state;
};

} // namespace native
