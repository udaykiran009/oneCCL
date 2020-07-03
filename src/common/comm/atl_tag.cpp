#include "common/comm/atl_tag.hpp"
#include "exec/exec.hpp"

void ccl_atl_tag::print() {
    LOG_INFO("\n",
             "\ntag_bits:      ",
             tag_bits,
             "\nmax_tag:       ",
             max_tag,
             "\npof2(max_tag): ",
             ccl_pof2(max_tag),
             "\nmax_tag_mask:  ",
             max_tag_mask,
             "\n");
}

uint64_t ccl_atl_tag::create(ccl_comm_id_t comm_id,
                             size_t rank,
                             ccl_sched_id_t sched_id,
                             ccl_op_id_t op_id) {
    uint64_t tag = 0;

    if (tag_bits == 32) {
        tag |= (((uint64_t)op_id) << op_id_shift) & op_id_mask;
        tag |= (((uint64_t)sched_id) << sched_id_shift) & sched_id_mask;
    }
    else if (tag_bits == 64) {
        tag |= (((uint64_t)op_id) << op_id_shift) & op_id_mask;
        tag |= (((uint64_t)sched_id) << sched_id_shift) & sched_id_mask;
        tag |= (((uint64_t)rank) << rank_shift) & rank_mask;
        tag |= (((uint64_t)comm_id) << comm_id_shift) & comm_id_mask;
    }
    else {
        CCL_ASSERT(0);
    }

    if (tag > max_tag)
        tag &= max_tag_mask;

    LOG_DEBUG("tag ",
              tag,
              " (comm_id: ",
              comm_id,
              ", rank ",
              rank,
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
                     " (comm_id: ",
                     comm_id,
                     ", rank ",
                     rank,
                     ", sched_id: ",
                     sched_id,
                     ", op_id: ",
                     (int)op_id,
                     ")");

    return tag;
}
