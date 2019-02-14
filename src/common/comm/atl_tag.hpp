#pragma once

#include "common/comm/comm_id.hpp"

using mlsl_atl_comm_tag_t = uint64_t;
using mlsl_sched_id_t = uint16_t;

/********************************************************************************
 *  atl tag                                                                     *
 * ******************************************************************************
 * 01234567 01234567 | 01234567 01234567  | 01234567 01234567 01234567 01234567 |
 *                                        |                                     |
 *      comm_id      | sched_id(per comm) |            global rank              |
 ********************************************************************************/
//number of bits used to shift global rank
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_SHIFT = 0;
//number of bits used to shift schedule id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_SHIFT = 32;
//number of bits used to shift comm id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_SHIFT = 48;
//mask to filter global rank after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_MASK     = 0x00000000FFFFFFFF;
//mask to filter sched id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_MASK = 0x0000FFFF00000000;
//mask to filter comm id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_MASK  = 0xFFFF000000000000;


inline mlsl_atl_comm_tag_t mlsl_create_atl_tag(mlsl_comm_id_t comm_id, mlsl_sched_id_t sched_id, size_t rank)
{
    mlsl_atl_comm_tag_t tag = 0;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(comm_id)  << MLSL_ATL_TAG_COMM_ID_SHIFT)  & MLSL_ATL_TAG_COMM_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(sched_id) << MLSL_ATL_TAG_SCHED_ID_SHIFT) & MLSL_ATL_TAG_SCHED_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(rank)     << MLSL_ATL_TAG_RANK_SHIFT)     & MLSL_ATL_TAG_RANK_MASK;

    MLSL_LOG(DEBUG, "comm %u, sched_id %u, rank %zu, tag %lu", comm_id, sched_id, rank, tag);
    return tag;
}

