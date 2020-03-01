#pragma once

#include "common/comm/comm_id_storage.hpp"
#include "common/utils/utils.hpp"

using ccl_op_id_t = uint8_t;
using ccl_sched_id_t = uint16_t;
using ccl_comm_id_t = uint16_t;

class ccl_atl_tag
{
public:
    ccl_atl_tag(size_t tag_bits, size_t max_tag) :
        tag_bits(tag_bits),
        max_tag(max_tag)
    {
        if (max_tag == ccl_pof2(max_tag) * 2 - 1)
            max_tag_mask = max_tag;
        else
            max_tag_mask = ccl_pof2(max_tag) - 1;

        LOG_INFO("tag_bits ", tag_bits,
                 ", max_tag ", max_tag,
                 ", pof2(max_tag) ", ccl_pof2(max_tag),
                 ", max_tag_mask ", max_tag_mask);
    }

    ccl_atl_tag(const ccl_atl_tag& other) = delete;
    ccl_atl_tag(ccl_atl_tag&& other) = delete;

    ccl_atl_tag& operator=(const ccl_atl_tag& other) = delete;
    ccl_atl_tag& operator=(ccl_atl_tag&& other) = delete;

    ~ccl_atl_tag() = default;

    /**
     * Generates the tag to be used by ATL communication operations
     * @param comm_id identifier of the communicator
     * @param sched_id identifier if the schedule
     * @param op_id local operation ID. Used to generate unique ATL tag when the rest of input parameters do not change
     * @return ATL communication tag
     */
    uint64_t create(ccl_comm_id_t comm_id, size_t rank, ccl_sched_id_t sched_id, ccl_op_id_t op_id);

private:

    /**********************************************************************************
     *  atl tag layout                                                                *
     * ********************************************************************************
     * 01234567 01234567 | 01234567 01234567 01234567 | 01234567 01234567  | 01234567 |
     *                   |                            |                    |          |
     *      comm_id      |            rank            | sched_id(per comm) |   op_id  |
     *********************************************************************************/

    size_t tag_bits;
    size_t max_tag;
    size_t max_tag_mask;

    const int op_id_shift = 0;
    const int sched_id_shift = 8;
    const int rank_shift = 24;
    const int comm_id_shift = 48;

    const uint64_t op_id_mask    = 0x00000000000000FF;
    const uint64_t sched_id_mask = 0x0000000000FFFF00;
    const uint64_t rank_mask     = 0x0000FFFFFF000000;
    const uint64_t comm_id_mask  = 0xFFFF000000000000;
};
