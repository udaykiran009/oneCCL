#include "common/comm/atl_tag.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

uint64_t ccl_atl_tag::create(ccl_comm_id_t comm_id, size_t rank, ccl_sched_id_t sched_id, ccl_op_id_t op_id)
{
    uint64_t tag = 0;

    if (tag_bits == 32)
    {
        tag |= ((uint64_t)op_id) << op_id_shift;
        tag |= ((uint64_t)sched_id) << sched_id_shift;
    }
    else if (tag_bits == 64)
    {
        tag |= ((uint64_t)op_id) << op_id_shift;
        tag |= ((uint64_t)sched_id) << sched_id_shift;
        tag |= ((uint64_t)rank) << rank_shift;
        tag |= ((uint64_t)comm_id) << comm_id_shift;
    }
    else
    {
        CCL_ASSERT(0);
    }

    LOG_DEBUG("tag ", tag, "(comm_id: ", comm_id, ", rank ", rank, ", sched_id: ", sched_id, ", op_id: ", (int)op_id, ")");
    CCL_THROW_IF_NOT(tag <= max_tag, "unexpected tag value ", tag, ", max_tag ", max_tag);

    return tag;
}
