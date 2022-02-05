#include "comp/comp.hpp"
#include "sched/entry/entry.hpp"
#include "sched/entry/recv_copy_entry.hpp"
#include "sched/queue/queue.hpp"

void recv_copy_entry::start() {
    atl_tag = comm->get_atl_comm()->tag->create(
        src, comm->get_comm_id(), sched->sched_id, sched->get_op_id());
    LOG_DEBUG("starting RECV in RECV_COPY entry, src ",
              src,
              ", tag ",
              atl_tag,
              ", req ",
              req,
              ", bytes ",
              bytes);

    atl_status_t atl_status = comm->get_atl_comm()->recv(
        sched->bin->get_atl_ep(), recv_buf.get_ptr(bytes), bytes, src, atl_tag, req);

    update_status(atl_status);
}

void recv_copy_entry::update() {
    atl_status_t atl_status = comm->get_atl_comm()->check(sched->bin->get_atl_ep(), req);

    if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
        CCL_THROW("RECV_COPY entry failed. atl_status: ", atl_status_to_str(atl_status));
    }

    if (!req.is_completed) {
        return;
    }

    LOG_DEBUG("completed RECV in RECV_COPY entry, req=", req, ", starting COPY");

    auto comp_status = ccl_comp_copy(
        recv_buf.get_ptr(bytes), copy_buf.get_ptr(bytes), bytes, attr.use_nontemporal);
    CCL_ASSERT(comp_status == ccl::status::success, "bad status ", comp_status);

    status = ccl_sched_entry_status_complete;
    LOG_DEBUG("completed COPY in RECV_COPY entry");
}
