#pragma once

#include "common/utils/buffer.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

class ze_a2a_gatherv_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_A2A_GATHERV";
    }

    const char* name() const override {
        return class_name();
    }

    virtual std::string name_ext() const override;

    explicit ze_a2a_gatherv_entry(ccl_sched* sched,
                                  ccl_buffer send_buf,
                                  size_t send_count,
                                  ccl_buffer recv_buf,
                                  const size_t* recv_counts,
                                  const ccl_datatype& dtype,
                                  int root,
                                  ccl_comm* comm,
                                  size_t peer_buf_idx = 0);

    void init_ze_hook() override;

protected:
    void dump_detail(std::stringstream& str) const override;

private:
    const ccl_buffer send_buf;
    const size_t send_bytes;
    const ccl_buffer recv_buf;
    const std::vector<size_t> recv_counts;
    const ccl_datatype dtype;
    const int root;
    const size_t peer_buf_idx;
};
