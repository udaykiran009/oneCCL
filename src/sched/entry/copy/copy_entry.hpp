#pragma once

#include "coll/coll_check.hpp"
#include "sched/entry/copy/copy_helper.hpp"
#include "sched/entry/entry.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#include <CL/sycl/backend/level_zero.hpp>

#ifdef MULTI_GPU_SUPPORT
#include "sched/entry/gpu/ze_base_entry.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL

using namespace ccl;

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
using namespace ccl::ze;
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT

enum class copy_type : int { regular = 0, sycl = 1, ze = 2 };

struct copy_attr {
    int peer_rank;
    copy_direction direction;
    size_t in_buf_offset;

    copy_attr(int pr = copy_helper::invalid_rank,
              copy_direction cd = copy_direction::h2h,
              size_t o = 0)
            : peer_rank(pr),
              direction(cd),
              in_buf_offset(o) {}
};

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
class copy_entry : public ze_base_entry,
#else
class copy_entry : public sched_entry,
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
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
            :
#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
              ze_base_entry(sched),
#else
              sched_entry(sched),
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
              sched(sched),
              in_buf(in_buf),
              out_buf(out_buf),
              count(count),
              dtype(dtype),
              attr(attr),
              buf_size_bytes(dtype.size() * count),
              worker_idx(0),
              is_initialized(false),
              ctype(copy_type::regular) {
    }

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
    ~copy_entry() {
        if (ctype == copy_type::ze) {
            finalize();
        }
    }

    void init() override {
        if (is_initialized) {
            return;
        }

        LOG_DEBUG("initialization");

        ze_base_entry::init();

        if (attr.peer_rank > copy_helper::invalid_rank) {
            if (!out_buf) {
                sched->get_memory().handle_manager.get(attr.peer_rank, 0, out_buf);
            }

            if (!in_buf) {
                sched->get_memory().handle_manager.get(attr.peer_rank, 0, in_buf);
            }
        }

        ZE_CALL(zeCommandListAppendMemoryCopy,
                (comp_list,
                 out_buf.get_ptr(),
                 ((char*)in_buf.get_ptr()) + attr.in_buf_offset * dtype.size(),
                 buf_size_bytes,
                 entry_event,
                 0,
                 nullptr));
        ZE_CALL(zeCommandListClose, (comp_list));

        is_initialized = true;

        LOG_DEBUG("initialization complete");
    }

    void finalize() override {
        if (!is_initialized) {
            return;
        }

        LOG_DEBUG("finalization");

        ze_base_entry::finalize();

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
        LOG_DEBUG("in_ptr_type: ",
                  ccl_usm_type_to_str(in_ptr_type),
                  ", out_ptr_type: ",
                  ccl_usm_type_to_str(out_ptr_type));
#endif // CCL_ENABLE_SYCL

        LOG_DEBUG("count: ", count, ", direction: ", to_string(attr.direction));

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
            if (!is_sycl_buf) {
                if ((in_ptr_type != sycl::usm::alloc::device) &&
                    (out_ptr_type != sycl::usm::alloc::device)) {
                    do_regular_copy();
                    return;
                }
            }

            copier = sycl_copier(
                attr.direction, in_buf, out_buf, count, dtype, is_sycl_buf, attr.in_buf_offset);
            copier.set_queue(q);
            ccl_tuple_for_each_indexed<ccl_sycl_buffer_one_dim_types>(copier);
            status = ccl_sched_entry_status_started;
        }
#ifdef MULTI_GPU_SUPPORT
        else {
            ctype = copy_type::ze;

            init();

            ze_base_entry::start();
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
            ze_base_entry::update();
            if (status == ccl_sched_entry_status_complete && !sched->coll_attr.to_cache) {
                finalize();
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

    const char* name() const noexcept override {
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
                           ", in_buf_offset ",
                           attr.in_buf_offset,
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
#endif // CCL_ENABLE_SYCL
};
