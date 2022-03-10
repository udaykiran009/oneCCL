#include "common/global/global.hpp"
#include "sched/entry/copy/copy_helper.hpp"

copy_attr::copy_attr()
        : peer_rank(ccl_comm::invalid_rank),
          peer_buf_idx(0),
          direction(copy_direction::undefined),
          map_comm(nullptr),
          in_buf_offset(0),
          out_buf_offset(0),
          use_nontemporal(false)
#ifdef CCL_ENABLE_ZE
          ,
          hint_queue_index(0)
#endif // CCL_ENABLE_ZE
{
}

copy_attr::copy_attr(int peer_rank,
                     size_t peer_buf_idx,
                     copy_direction direction,
                     ccl_comm* map_comm,
                     size_t in_buf_offset,
                     size_t out_buf_offset)
        : peer_rank(peer_rank),
          peer_buf_idx(peer_buf_idx),
          direction(direction),
          map_comm(map_comm),
          in_buf_offset(in_buf_offset),
          out_buf_offset(out_buf_offset) {}

copy_attr::copy_attr(copy_direction direction, size_t in_buf_offset, size_t out_buf_offset)
        : direction(direction),
          in_buf_offset(in_buf_offset),
          out_buf_offset(out_buf_offset) {}

using copy_direction_str_enum =
    ccl::utils::enum_to_str<ccl::utils::enum_to_underlying(copy_direction::c2c) + 1>;
std::string to_string(copy_direction val) {
    return copy_direction_str_enum({ "UNDEFINED", "H2H", "D2H", "H2D", "D2D", "T2T", "C2C" })
        .choose(val, "UNKNOWN");
}

#ifdef CCL_ENABLE_SYCL

sycl_copier::sycl_copier(copy_direction direction,
                         ccl_buffer in_buf,
                         ccl_buffer out_buf,
                         size_t count,
                         const ccl_datatype& dtype,
                         bool is_sycl_buf,
                         size_t in_buf_offset,
                         size_t out_buf_offset)
        : direction(direction),
          in_buf(in_buf),
          out_buf(out_buf),
          count(count),
          dtype(dtype),
          is_sycl_buf(is_sycl_buf),
          in_buf_offset(in_buf_offset),
          out_buf_offset(out_buf_offset) {}

bool sycl_copier::is_completed() const {
    return e.get_info<sycl::info::event::command_execution_status>() ==
           sycl::info::event_command_status::complete;
}

void sycl_copier::set_queue(const sycl::queue* external_q) {
    q = const_cast<sycl::queue*>(external_q);
    CCL_THROW_IF_NOT(q);
}

std::string sycl_copier::get_dtype_name(const ccl_datatype& dt) const {
    return ccl::global_data::get().dtypes->name(dt);
}

#endif // CCL_ENABLE_SYCL
