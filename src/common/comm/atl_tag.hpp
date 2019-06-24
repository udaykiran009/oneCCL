#pragma once

#include "common/comm/comm_id.hpp"

using mlsl_op_id_t = uint8_t;
using mlsl_sched_id_t = uint16_t;
using mlsl_comm_id_t = uint16_t;

class mlsl_atl_tag
{
public:
    mlsl_atl_tag(size_t tag_bits, size_t max_tag) :
        tag_bits(tag_bits),
        max_tag(max_tag)
    {}

    mlsl_atl_tag(const mlsl_atl_tag& other) = delete;
    mlsl_atl_tag(mlsl_atl_tag&& other) = delete;

    mlsl_atl_tag& operator=(const mlsl_atl_tag& other) = delete;
    mlsl_atl_tag& operator=(mlsl_atl_tag&& other) = delete;

    ~mlsl_atl_tag() = default;

    /**
     * Generates the tag to be used by ATL communication operations
     * @param comm_id identifier of the communicator
     * @param sched_id identifier if the schedule
     * @param op_id local operation ID. Used to generate unique ATL tag when the rest of input parameters do not change
     * @return ATL communication tag
     */
    uint64_t create(mlsl_comm_id_t comm_id, size_t rank, mlsl_sched_id_t sched_id, mlsl_op_id_t op_id);

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

    const int op_id_shift = 0;
    const int sched_id_shift = 8;
    const int rank_shift = 24;
    const int comm_id_shift = 48;

    const uint64_t op_id_mask    = 0x00000000000000FF;
    const uint64_t sched_id_mask = 0x0000000000FFFF00;
    const uint64_t rank_mask     = 0x0000FFFFFF000000;
    const uint64_t comm_id_mask  = 0xFFFF000000000000;
    
};
