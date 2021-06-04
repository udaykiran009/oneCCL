#include <numeric>

#include "coll/algorithms/algorithms_enum.hpp"
#include "coll/coll_param.hpp"
#include "common/global/global.hpp"
#include "sched/sched_base.hpp"

std::string to_string(ccl_sched_add_mode mode) {
    switch (mode) {
        case ccl_sched_add_front: return "FRONT";
        case ccl_sched_add_back: return "BACK";
        default: return "DEFAULT";
    }
    return "DEFAULT";
}

void ccl_sched_base::set_coll_attr(const ccl_coll_attr& attr) {
    coll_attr = attr;
}

void ccl_sched_base::update_coll_param_and_attr(const ccl_coll_param& param,
                                                const ccl_coll_attr& attr) {
#ifdef CCL_ENABLE_SYCL
    coll_param.copy_deps(param.deps);
#endif /* CCL_ENABLE_SYCL */

    bool has_pre_post_copies =
        (!coll_param.device_send_bufs.empty() || !coll_param.device_recv_bufs.empty()) ? true
                                                                                       : false;

    if (has_pre_post_copies) {
        CCL_THROW_IF_NOT(coll_param.device_send_bufs.size() == param.send_bufs.size(),
                         "send_bufs sizes mismatch");
        CCL_THROW_IF_NOT(coll_param.device_recv_bufs.size() == param.recv_bufs.size(),
                         "recv_bufs sizes mismatch");
        coll_param.device_send_bufs = param.send_bufs;
        coll_param.device_recv_bufs = param.recv_bufs;
    }
    else {
        CCL_THROW_IF_NOT(coll_param.send_bufs.size() == param.send_bufs.size(),
                         "send_bufs sizes mismatch");
        CCL_THROW_IF_NOT(coll_param.recv_bufs.size() == param.recv_bufs.size(),
                         "recv_bufs sizes mismatch");
        coll_param.send_bufs = param.send_bufs;
        coll_param.recv_bufs = param.recv_bufs;
    }

    int comm_size = coll_param.comm->size();

    if (coll_param.ctype == ccl_coll_allgatherv) {
        if (coll_attr.vector_buf)
            CCL_THROW_IF_NOT(static_cast<int>(coll_param.recv_bufs.size()) == comm_size);
        CCL_THROW_IF_NOT(static_cast<int>(coll_param.recv_counts.size()) == comm_size);
    }

    if (coll_param.ctype == ccl_coll_alltoallv) {
        if (coll_attr.vector_buf)
            CCL_THROW_IF_NOT(static_cast<int>(coll_param.send_bufs.size()) == comm_size);
        CCL_THROW_IF_NOT(static_cast<int>(coll_param.send_counts.size()) == comm_size);

        if (coll_attr.vector_buf)
            CCL_THROW_IF_NOT(static_cast<int>(coll_param.recv_bufs.size()) == comm_size);
        CCL_THROW_IF_NOT(static_cast<int>(coll_param.recv_counts.size()) == comm_size);
    }

    if (coll_param.ctype == ccl_coll_sparse_allreduce) {
        coll_param.sparse_param.send_ind_buf = param.sparse_param.send_ind_buf;
        coll_param.sparse_param.send_val_buf = param.sparse_param.send_val_buf;
        coll_param.sparse_param.recv_ind_buf = param.sparse_param.recv_ind_buf;
        coll_param.sparse_param.recv_val_buf = param.sparse_param.recv_val_buf;
    }

    if (ccl::global_data::env().priority_mode == ccl_priority_direct) {
        coll_attr.priority = attr.priority;
    }
}

size_t ccl_sched_base::get_priority() const {
    size_t priority = 0;

    switch (ccl::global_data::env().priority_mode) {
        case ccl_priority_none: priority = 0; break;
        case ccl_priority_direct:
        case ccl_priority_lifo: priority = coll_attr.priority; break;
        default:
            CCL_FATAL("unexpected priority_mode ", ccl::global_data::env().priority_mode);
            break;
    }

    LOG_DEBUG("sched, ", this, ", priority ", priority);

    return priority;
}

ccl_buffer ccl_sched_base::alloc_buffer(size_t bytes) {
    LOG_DEBUG("try to allocate buffer size: ", bytes);
    CCL_THROW_IF_NOT(bytes > 0, "incorrect buffer size: ", bytes);

    ccl_buffer buffer =
        ccl_buffer(CCL_MALLOC(bytes, "sched_buffer"), bytes, 0, ccl_buffer_type::DIRECT);
    memory.buf_list.emplace_back(buffer, bytes);
    CCL_THROW_IF_NOT(buffer.get_ptr(), "null ptr");

    LOG_DEBUG("allocated buffer ptr: ", buffer.get_ptr(), ", size: ", buffer.get_size());
    return buffer;
}

#ifdef CCL_ENABLE_SYCL
ccl_buffer ccl_sched_base::alloc_staging_buffer(size_t bytes) {
    LOG_DEBUG("try to allocate usm host buffer size: ", bytes);
    CCL_THROW_IF_NOT(bytes > 0, "incorrect buffer size: ", bytes);

    ccl_buffer buffer;
    if (ccl::global_data::env().staging_buffer == ccl_staging_usm) {
        CCL_ASSERT(coll_param.stream);
        sycl::context ctx = coll_param.stream->get_native_stream().get_context();
        buffer = ccl_buffer(aligned_alloc_host(64, bytes, ctx), bytes, 0, ccl_buffer_type::DIRECT);
        memory.sycl_buf_list.emplace_back(buffer, bytes, ctx);
        LOG_DEBUG(
            "allocated host usm buffer ptr: ", buffer.get_ptr(), ", size: ", buffer.get_size());
    }
    else {
        buffer = alloc_buffer(bytes);
    }

    CCL_THROW_IF_NOT(buffer.get_ptr(), "null ptr");

    return buffer;
}
#endif /* CCL_ENABLE_SYCL */

void ccl_sched_base::free_buffers() {
    std::list<ccl_sched_buffer_handler>::iterator it;
    for (it = memory.buf_list.begin(); it != memory.buf_list.end(); it++) {
        LOG_DEBUG("free ", it->buffer.get_ptr());
        CCL_FREE(it->buffer.get_ptr());
    }
    memory.buf_list.clear();

#ifdef CCL_ENABLE_SYCL
    std::list<ccl_sched_sycl_buffer_handler>::iterator sycl_it;
    for (sycl_it = memory.sycl_buf_list.begin(); sycl_it != memory.sycl_buf_list.end(); sycl_it++) {
        LOG_DEBUG("free host usm ", sycl_it->buffer.get_ptr());
        free(sycl_it->buffer.get_ptr(), sycl_it->ctx);
    }
    memory.sycl_buf_list.clear();
#endif /* CCL_ENABLE_SYCL */
}

ccl_buffer ccl_sched_base::update_buffer(ccl_buffer buffer, size_t new_size) {
    LOG_DEBUG("update pointer data size: ",
              buffer.get_ptr(),
              ", from: ",
              buffer.get_size(),
              ", to: ",
              new_size);
    CCL_THROW_IF_NOT(new_size > 0, "incorrect buffer size: ", new_size);

    /* in case old_ptr will be freed */
    void* aux_ptr = buffer.get_ptr();

    ccl_buffer new_buf = ccl_buffer(
        CCL_REALLOC(
            buffer.get_ptr(), (size_t)buffer.get_size(), new_size, CACHELINE_SIZE, "sched_buffer"),
        new_size,
        0,
        ccl_buffer_type::DIRECT);
    bool updated = false;
    for (auto& it : memory.buf_list) {
        if (it.buffer.get_ptr() == aux_ptr) {
            /* assign ptr unconditionally, because realloc can return the same pointer */
            it.buffer = new_buf;
            it.size = new_size;
            updated = true;
            break;
        }
    }

    CCL_THROW_IF_NOT(updated, "Cannot update memory in buf_list for addres: ", new_buf.get_ptr());
    return new_buf;
}

ccl_buffer ccl_sched_base::find_and_realloc_buffer(void* in_ptr,
                                                   size_t new_size,
                                                   size_t expected_size) {
    LOG_DEBUG("sched: ", this, ", contains buffer objects: ", memory.buf_list.size());
    for (auto& it : memory.buf_list) {
        if (it.buffer.get_ptr() == in_ptr) {
#ifdef ENABLE_DEBUG_SPARSE
            if (expected_size != 0 && (it.buffer.get_size() < expected_size)) {
                std::stringstream ss;
                ss << "Unexpected realloc buffer by pointer: " << in_ptr
                   << ", cur size: " << it.buffer.get_size() << ", to: " << new_size
                   << ", expected: " << expected_size;
                ss << "\nbuffers:\n";
                for (const auto& it : memory.buf_list) {
                    ss << it.buffer << ", ";
                }
                LOG_ERROR(ss.str());
                CCL_ASSERT(false, ss.str());
                CCL_THROW_IF_NOT(
                    false, "Cannot fin buffer by ptr: ", in_ptr, ", available buffers: ", ss.str());
            }
#endif //ENABLE_DEBUG_SPARSE
            if ((it.buffer.get_size() < 0) ||
                (static_cast<size_t>(it.buffer.get_size()) < new_size)) {
                LOG_DEBUG("try to realloc buffer by pointer: ",
                          in_ptr,
                          ", from: ",
                          it.buffer.get_size(),
                          ", to: ",
                          new_size,
                          ", expected: ",
                          expected_size);

                it.buffer = ccl_buffer(CCL_REALLOC(in_ptr,
                                                   (size_t)it.buffer.get_size(),
                                                   new_size,
                                                   CACHELINE_SIZE,
                                                   "sched_buffer"),
                                       new_size,
                                       0,
                                       ccl_buffer_type::DIRECT);
                it.size = new_size;
            }
            return it.buffer;
        }
    }

    /* throw expection */
    std::stringstream ss;
    for (const auto& it : memory.buf_list) {
        ss << it.buffer << ", ";
    }
    CCL_THROW_IF_NOT(
        false, "cannot find buffer by ptr: ", in_ptr, ", available buffers: ", ss.str());
    return ccl_buffer();
}

void ccl_sched_base::add_memory_region(atl_mr_t* mr) {
    CCL_THROW_IF_NOT(mr);
    memory.mr_list.emplace_back(mr);
}

void ccl_sched_base::get_pre_post_copy_counts(std::vector<size_t>& d2h_counts,
                                              std::vector<size_t>& h2d_counts,
                                              bool& reuse_buffers) {
    ccl_coll_param& param = coll_param;

    d2h_counts.clear();
    h2d_counts.clear();
    reuse_buffers = false;

    switch (param.ctype) {
        case ccl_coll_allgatherv:
            d2h_counts.push_back(param.get_send_count());
            if (param.recv_bufs.size() > 1) {
                h2d_counts.insert(
                    h2d_counts.end(), param.recv_counts.begin(), param.recv_counts.end());
            }
            else {
                h2d_counts.push_back(
                    std::accumulate(param.recv_counts.begin(), param.recv_counts.end(), 0));
            }
            break;
        case ccl_coll_allreduce:
            d2h_counts.push_back(param.get_send_count());
            h2d_counts.push_back(param.get_recv_count());
            /* use in-place to avoid allocation of extra staging buffer*/
            reuse_buffers = true;
            break;
        case ccl_coll_alltoall:
            d2h_counts.push_back(param.get_send_count() * param.comm->size());
            h2d_counts.push_back(param.get_recv_count() * param.comm->size());
            break;
        case ccl_coll_alltoallv:
            if (param.recv_bufs.size() > 1) {
                /* expect that vector_buf is enabled for send/recv both */
                d2h_counts.insert(
                    d2h_counts.end(), param.send_counts.begin(), param.send_counts.end());
                h2d_counts.insert(
                    h2d_counts.end(), param.recv_counts.begin(), param.recv_counts.end());
            }
            else {
                d2h_counts.push_back(
                    std::accumulate(param.send_counts.begin(), param.send_counts.end(), 0));
                h2d_counts.push_back(
                    std::accumulate(param.recv_counts.begin(), param.recv_counts.end(), 0));
            }
            break;
        case ccl_coll_bcast:
            if (param.comm->rank() == param.root)
                d2h_counts.push_back(param.get_send_count());
            h2d_counts.push_back(param.get_recv_count());
            reuse_buffers = true;
            break;
        case ccl_coll_reduce:
            d2h_counts.push_back(param.get_send_count());
            if (param.comm->rank() == param.root)
                h2d_counts.push_back(param.get_recv_count());
            break;
        case ccl_coll_reduce_scatter:
            d2h_counts.push_back(param.get_send_count());
            h2d_counts.push_back(param.get_recv_count());
            break;
        case ccl_coll_sparse_allreduce:
            CCL_FATAL("SYCL stream is not supported for sparse_allreduce yet");
            CCL_ASSERT(0);
            break;
        default: break;
    }
}

void ccl_sched_base::alloc_buffers_for_pre_post_copy() {
#ifdef CCL_ENABLE_SYCL

    ccl_coll_param& param = coll_param;

    param.device_send_bufs.clear();
    param.device_recv_bufs.clear();

    if (!param.stream || (!param.stream->is_sycl_device_stream()))
        return;

    bool should_alloc_buffers = true;

    if (!coll_attr.is_sycl_buffer) {
        auto bufs = param.get_all_non_zero_bufs();
        if (!bufs.empty()) {
            auto usm_type =
                sycl::get_pointer_type(bufs[0], param.stream->get_native_stream().get_context());
            if ((usm_type == sycl::usm::alloc::host) || (usm_type == sycl::usm::alloc::shared) ||
                ((usm_type == sycl::usm::alloc::device) &&
                 atl_wrapper::attr.out.enable_device_buf)) {
                should_alloc_buffers = false;
            }
        }
    }

    LOG_DEBUG("coll_type ", param.ctype, ", should_alloc_buffers ", should_alloc_buffers);

    if (!should_alloc_buffers) {
        return;
    }

    /*
        move user-supplied pointers into device_* fields
        they will be used further for pre-post copies
    */
    param.device_send_bufs = param.send_bufs;
    param.device_recv_bufs = param.recv_bufs;

    std::vector<size_t> d2h_counts;
    std::vector<size_t> h2d_counts;
    bool reuse_buffers;
    get_pre_post_copy_counts(d2h_counts, h2d_counts, reuse_buffers);

    LOG_DEBUG("alloc tmp buffers for D2H and H2D copies, coll_type ",
              ccl_coll_type_to_str(param.ctype),
              ", dtype_size ",
              param.dtype.size(),
              ", comm_size ",
              param.comm->size(),
              ", d2h_counts_size ",
              d2h_counts.size(),
              ", h2d_counts_size ",
              h2d_counts.size(),
              ", reuse_buffers ",
              reuse_buffers);

    if (reuse_buffers) {
        /* keep only single vector with counts */
        if (d2h_counts.size() < h2d_counts.size())
            d2h_counts = h2d_counts;
        h2d_counts.clear();
    }

    for (size_t idx = 0; idx < d2h_counts.size(); idx++) {
        if (d2h_counts[idx])
            param.send_bufs[idx] =
                alloc_staging_buffer(d2h_counts[idx] * param.dtype.size()).get_ptr();
        else
            param.send_bufs[idx] = nullptr;
    }

    for (size_t idx = 0; idx < h2d_counts.size(); idx++) {
        if (h2d_counts[idx])
            param.recv_bufs[idx] =
                alloc_staging_buffer(h2d_counts[idx] * param.dtype.size()).get_ptr();
        else
            param.recv_bufs[idx] = nullptr;
    }

    if (reuse_buffers) {
        param.recv_bufs = param.send_bufs;
    }

    CCL_THROW_IF_NOT(param.send_bufs.size() == param.device_send_bufs.size(),
                     "send_bufs.size() mismatch: ",
                     param.send_bufs.size(),
                     " vs ",
                     param.device_send_bufs.size());

    CCL_THROW_IF_NOT(param.recv_bufs.size() == param.device_recv_bufs.size(),
                     "recv_bufs.size() mismatch: ",
                     param.recv_bufs.size(),
                     " vs ",
                     param.device_recv_bufs.size());

#endif /* CCL_ENABLE_SYCL */
}

void ccl_sched_base::update_id() {
    sched_id = coll_param.comm->get_sched_id(internal_type != ccl_sched_internal_none);
}

void ccl_sched_base::dump(std::ostream& out, const char* name) const {
    ccl_logger::format(out, "\n-----------------", name, "---------------\n");
    ccl_logger::format(out,
                       "sched: ",
                       this,
                       ", coll ",
                       ccl_coll_type_to_str(coll_param.ctype),
                       ", comm_id ",
                       std::dec,
                       coll_param.comm->id(),
                       ", sched_id ",
                       sched_id);
}
