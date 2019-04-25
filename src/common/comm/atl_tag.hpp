#pragma once

#include "common/comm/comm_id.hpp"

using mlsl_atl_comm_tag_t = uint64_t;
using mlsl_sched_id_t = uint16_t;
using mlsl_op_id_t = uint8_t;

/**********************************************************************************
 *  atl tag                                                                       *
 * ********************************************************************************
 * 01234567 01234567 | 01234567 01234567  | 01234567 | 01234567 01234567 01234567 |
 *                   |                    |          |                            |
 *      comm_id      | sched_id(per comm) |   op_id  |         global rank        |
 *********************************************************************************/
//number of bits used to shift global rank
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_SHIFT = 0;
//number of bits used to shift op_id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_OP_ID_SHIFT = 24;
//number of bits used to shift schedule id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_SHIFT = 32;
//number of bits used to shift comm id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_SHIFT = 48;
//mask to filter global rank after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_MASK      = 0x0000000000FFFFFF;
//mask to filter global rank after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_OP_ID_MASK     = 0x00000000FF000000;
//mask to filter sched id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_MASK  = 0x0000FFFF00000000;
//mask to filter comm id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_MASK   = 0xFFFF000000000000;

/**
 * Generates the tag to be used by ATL communication operations
 * @param comm_id identifier of the communicator
 * @param sched_id identifier if the schedule
 * @param op_id local operation ID. Used to generate unique ATL tag when the rest of input parameters do not change
 * @param rank source or destionation rank ID, depends on ATL communication type
 * @return ATL communication tag
 */
inline mlsl_atl_comm_tag_t mlsl_create_atl_tag(mlsl_comm_id_t comm_id, mlsl_sched_id_t sched_id, mlsl_op_id_t op_id, size_t rank)
{
    mlsl_atl_comm_tag_t tag = 0;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(comm_id)  << MLSL_ATL_TAG_COMM_ID_SHIFT)  & MLSL_ATL_TAG_COMM_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(sched_id) << MLSL_ATL_TAG_SCHED_ID_SHIFT) & MLSL_ATL_TAG_SCHED_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(op_id)    << MLSL_ATL_TAG_OP_ID_SHIFT)    & MLSL_ATL_TAG_OP_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(rank)     << MLSL_ATL_TAG_RANK_SHIFT)     & MLSL_ATL_TAG_RANK_MASK;

    LOG_DEBUG("comm ", std::setbase(16), comm_id,
              ", sched_id ", std::setbase(16), sched_id,
              ", rank ", std::setbase(16), rank,
              ", op_id ", std::setbase(16), op_id,
              ", tag ", std::setbase(16), tag);
    return tag;
}
