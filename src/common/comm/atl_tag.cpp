#include "common/comm/atl_tag.hpp"
#include "exec/exec.hpp"

void ccl_atl_tag::print() {
    LOG_INFO("atl-tag:");
    LOG_INFO("  bits: ", tag_bits);
    LOG_INFO("  max: ", max_tag);
    LOG_INFO("  mask: ", max_tag_mask);
    LOG_INFO("  pof2: ", ccl_pof2(max_tag));
}

uint64_t ccl_atl_tag::create(int rank,
                             ccl_comm_id_t comm_id,
                             ccl_sched_id_t sched_id,
                             ccl_op_id_t op_id) {
    uint64_t tag = 0;

    tag |= (((uint64_t)op_id) << op_id_shift) & op_id_mask;
    tag |= (((uint64_t)sched_id) << sched_id_shift) & sched_id_mask;
    tag |= (((uint64_t)comm_id) << comm_id_shift) & comm_id_mask;
    tag |= (((uint64_t)rank) << rank_shift) & rank_mask;

    if (tag > max_tag)
        tag &= max_tag_mask;

    LOG_DEBUG("tag ",
              tag,
              " (rank ",
              rank,
              ", comm_id: ",
              comm_id,
              ", sched_id: ",
              sched_id,
              ", op_id: ",
              (int)op_id,
              ")");

    CCL_THROW_IF_NOT(tag <= max_tag,
                     "unexpected tag value ",
                     tag,
                     ", max_tag ",
                     max_tag,
                     " (rank ",
                     rank,
                     ", comm_id: ",
                     comm_id,
                     ", sched_id: ",
                     sched_id,
                     ", op_id: ",
                     (int)op_id,
                     ")");

    return tag;
}
