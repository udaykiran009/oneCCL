#pragma once

#include "coll/coll_check.hpp"
#include "sched/entry/copy/copy_helper.hpp"
#include "sched/entry/entry.hpp"

#ifdef CCL_ENABLE_SYCL
#include "sched/entry/gpu/ze_primitives.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include <CL/sycl/backend/level_zero.hpp>
#include <CL/sycl.hpp>
#endif // CCL_ENABLE_SYCL

using namespace ccl;

#ifdef CCL_ENABLE_SYCL
using namespace ccl::ze;
#endif // CCL_ENABLE_SYCL

enum class copy_type : int { regular = 0, sycl = 1, ze = 2 };

struct copy_attr {
    int peer_rank;
    copy_direction direction;
    size_t offset;

    copy_attr() : peer_rank(copy_helper::invalid_rank), direction(copy_direction::h2h), offset(0) {}

    copy_attr(int pr, copy_direction cd, size_t o) : peer_rank(pr), direction(cd), offset(o) {}
};

class copy_entry : public sched_entry,
                   public postponed_fields<copy_entry,
                                           ccl_sched_entry_field_in_buf,
                                           ccl_sched_entry_field_cnt,
                                           ccl_sched_entry_field_dtype> {
public:
    static constexpr const char* class_name() noexcept {
        return "COPY";
    }

    bool is_gpu_entry() const noexcept override {
        return true;
    }

    copy_entry() = delete;
    copy_entry(ccl_sched* sched,
               ccl_buffer in_buf,
               ccl_buffer out_buf,
               size_t count,
               const ccl_datatype& dtype,
               copy_attr attr = copy_attr())
            : sched_entry(sched),
              sched(sched),
              in_buf(in_buf),
              out_buf(out_buf),
              count(count),
              dtype(dtype),
              attr(attr),
              buf_size_bytes(dtype.size() * count),
              worker_idx(0),
              is_initialized(false),
              ctype(copy_type::regular) {}

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
    ~copy_entry() {
        if (ctype == copy_type::ze) {
            finalize();
        }
    }

    void init(sycl::queue*& q) {
        if (is_initialized) {
            return;
        }

        LOG_DEBUG("initialization");

        auto sycl_device = q->get_device();
        device = sycl_device.template get_native<cl::sycl::backend::level_zero>();

        auto sycl_context = q->get_context();
        context = sycl_context.template get_native<cl::sycl::backend::level_zero>();

        uint32_t num_queue_groups;
        get_num_queue_groups(device, &num_queue_groups);

        ze_queue_properties_t queue_props;
        get_queues_properties(device, num_queue_groups, &queue_props);

        uint32_t comp_ordinal, comp_queue_index;
        get_comp_queue_ordinal(device, queue_props, &comp_ordinal);
        get_queue_index(
            queue_props, comp_ordinal, sched->coll_param.comm->rank(), &comp_queue_index);

        comp_queue_desc = default_comp_queue_desc;
        comp_queue_desc.index = comp_queue_index;
        comp_queue_desc.ordinal = comp_ordinal;

        ccl::global_data::get().ze_cache->get(
            worker_idx, context, device, &comp_queue_desc, &comp_queue);
        LOG_DEBUG("get compute queue: { ordinal: ",
                  comp_queue_desc.ordinal,
                  ", index: ",
                  comp_queue_desc.index,
                  " }");

        comp_list_desc = default_cmd_list_desc;
        comp_list_desc.commandQueueGroupOrdinal = comp_ordinal;
        ccl::global_data::get().ze_cache->get(
            worker_idx, context, device, &comp_list_desc, &comp_list);
        LOG_DEBUG("get compute list: { ordinal: ", comp_list_desc.commandQueueGroupOrdinal, " }");

        ZE_CALL(zeCommandListCreate, (context, device, &comp_list_desc, &comp_list));

        if (attr.peer_rank > copy_helper::invalid_rank) {
            if (!out_buf) {
                sched->get_memory().handle_manager.get(attr.peer_rank, 0, out_buf);
            }

            if (!in_buf) {
                sched->get_memory().handle_manager.get(attr.peer_rank, 0, in_buf);
            }
        }

        ZE_CALL(
            zeCommandListAppendMemoryCopy,
            (comp_list, out_buf.get_ptr(), in_buf.get_ptr(), buf_size_bytes, nullptr, 0, nullptr));
        ZE_CALL(zeCommandListClose, (comp_list));

        fence_desc = default_fence_desc;
        ccl::global_data::get().ze_cache->get(worker_idx, comp_queue, &fence_desc, &fence);

        is_initialized = true;

        LOG_DEBUG("initialization complete");
    }

    void finalize() {
        if (!is_initialized) {
            return;
        }

        LOG_DEBUG("finalization");

        ccl::global_data::get().ze_cache->push(worker_idx, comp_queue, &fence_desc, &fence);
        ccl::global_data::get().ze_cache->push(
            worker_idx, context, device, &comp_list_desc, &comp_list);
        ccl::global_data::get().ze_cache->push(
            worker_idx, context, device, &comp_queue_desc, &comp_queue);

        is_initialized = false;

        LOG_DEBUG("finalization complete");
    }
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT

    void start() override {
        //update_fields();

        CCL_THROW_IF_NOT(sched != nullptr, "no sched");
        LOG_DEBUG(class_name(), ": in_buf ", in_buf, ", out_buf ", out_buf, ", count ", count);

#ifdef CCL_ENABLE_SYCL
        worker_idx = sched->queue->get_idx();
        sycl::queue* q = nullptr;
        sycl::usm::alloc in_ptr_type = sycl::usm::alloc::unknown;
        sycl::usm::alloc out_ptr_type = sycl::usm::alloc::unknown;
        if (sched->coll_param.stream) {
            q = sched->coll_param.stream->get_native_stream(worker_idx);
            CCL_THROW_IF_NOT(q, "null sycl queue");
            in_ptr_type = sycl::get_pointer_type(in_buf.get_ptr(), q->get_context());
            out_ptr_type = sycl::get_pointer_type(out_buf.get_ptr(), q->get_context());
        }
#endif // CCL_ENABLE_SYCL

        if (!sched->coll_param.stream || (attr.direction == copy_direction::h2h)) {
#ifdef CCL_ENABLE_SYCL
            CCL_THROW_IF_NOT(in_ptr_type != sycl::usm::alloc::device,
                             "unexpected device usm type for input buffer");
            CCL_THROW_IF_NOT(out_ptr_type != sycl::usm::alloc::device,
                             "unexpected device usm type for output buffer");
#endif // CCL_ENABLE_SYCL
            do_regular_copy();
            return;
        }

#ifdef CCL_ENABLE_SYCL
        is_sycl_buf = sched->coll_attr.is_sycl_buf;
        if (q->get_backend() != cl::sycl::backend::level_zero || is_sycl_buf) {
            ctype = copy_type::sycl;

            copy_direction direction;

            if (is_sycl_buf) {
                direction = attr.direction;
            }
            else {
                LOG_DEBUG("in_ptr_type: ",
                          ccl_usm_type_to_str(in_ptr_type),
                          ", out_ptr_type: ",
                          ccl_usm_type_to_str(out_ptr_type),
                          ", native_stream: ",
                          sched->coll_param.stream->to_string(),
                          ", count: ",
                          count);

                if ((in_ptr_type != sycl::usm::alloc::device) &&
                    (out_ptr_type != sycl::usm::alloc::device)) {
                    do_regular_copy();
                    return;
                }

                if ((in_ptr_type == sycl::usm::alloc::device) &&
                    (out_ptr_type == sycl::usm::alloc::device)) {
                    direction = copy_direction::d2d;
                }

                if ((in_ptr_type == sycl::usm::alloc::host) &&
                    (out_ptr_type == sycl::usm::alloc::device)) {
                    direction = copy_direction::h2d;
                }

                if ((in_ptr_type == sycl::usm::alloc::device) &&
                    (out_ptr_type == sycl::usm::alloc::host)) {
                    direction = copy_direction::d2h;
                }
            }

            copier =
                sycl_copier(direction, in_buf, out_buf, count, dtype, is_sycl_buf, attr.offset);
            copier.set_queue(q);
            ccl_tuple_for_each_indexed<ccl_sycl_buffer_one_dim_types>(copier);
            status = ccl_sched_entry_status_started;
        }
#ifdef MULTI_GPU_SUPPORT
        else {
            ctype = copy_type::ze;

            init(q);

            if (is_initialized && status == ccl_sched_entry_status_not_started) {
                ZE_CALL(zeFenceReset, (fence));
            }

            LOG_DEBUG("execute command list");
            ZE_CALL(zeCommandQueueExecuteCommandLists, (comp_queue, 1, &comp_list, fence));

            if (((global_data::env().ze_serialize_mode & ze_serialize_block)) != 0) {
                LOG_DEBUG("wait until command lists are executed");
                ZE_CALL(zeHostSynchronize, (comp_queue));
            }
            status = ccl_sched_entry_status_started;
        }
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL
    }

    void update() override {
#ifdef CCL_ENABLE_SYCL
        if (ctype == copy_type::sycl) {
            if (copier.is_completed()) {
                status = ccl_sched_entry_status_complete;
            }
        }
#ifdef MULTI_GPU_SUPPORT
        else {
            ze_result_t fence_status;
            if (global_data::env().kernel_debug == 0) {
                fence_status = zeFenceQueryStatus(fence);
            }
            else {
                fence_status =
                    zeCommandQueueSynchronize(comp_queue, std::numeric_limits<uint64_t>::max());
            }

            if (fence_status == ZE_RESULT_SUCCESS) {
                LOG_DEBUG("command list complete");
                if (!sched->coll_attr.to_cache) {
                    finalize();
                }
                status = ccl_sched_entry_status_complete;
            }
            else if (fence_status == ZE_RESULT_NOT_READY) {
                // just return in case if the kernel is not ready yet, will check again on the next iteration
                return;
            }
            else {
                CCL_THROW("error at zeFenceQueryStatus");
            }
        }
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL
    }

    void do_regular_copy() {
        auto comp_status = ccl_comp_copy(
            in_buf.get_ptr(buf_size_bytes), out_buf.get_ptr(buf_size_bytes), count, dtype);
        CCL_ASSERT(comp_status == ccl::status::success, "bad status ", comp_status);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_in_buf> id) {
        return in_buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id) {
        return count;
    }

    ccl_datatype& get_field_ref(field_id_t<ccl_sched_entry_field_dtype> id) {
        return dtype;
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", count ",
                           count,
                           ", in_buf ",
                           in_buf,
                           ", out_buf ",
                           out_buf,
                           ", offset ",
                           attr.offset,
                           "\n");
    }

private:
    ccl_sched* const sched;
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t count;
    ccl_datatype dtype;
    copy_attr attr;
    bool is_sycl_buf;
    const size_t buf_size_bytes;
    size_t worker_idx;
    bool is_initialized;
    copy_type ctype;
#ifdef CCL_ENABLE_SYCL
    sycl_copier copier;
#ifdef MULTI_GPU_SUPPORT
    ze_context_handle_t context;
    ze_device_handle_t device;
    ze_command_queue_handle_t comp_queue;
    ze_command_queue_desc_t comp_queue_desc;
    ze_command_list_desc_t comp_list_desc;
    ze_command_list_handle_t comp_list;
    ze_fence_handle_t fence;
    ze_fence_desc_t fence_desc;
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL
};
