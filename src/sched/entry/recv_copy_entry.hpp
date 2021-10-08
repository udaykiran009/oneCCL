#pragma once

#include "sched/entry/copy/copy_helper.hpp"
#include "sched/entry/entry.hpp"

class recv_copy_entry final : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "RECV_COPY";
    }

    recv_copy_entry() = delete;
    recv_copy_entry(ccl_sched* sched,
                    ccl_buffer recv_buf,
                    ccl_buffer copy_buf,
                    size_t bytes,
                    int src,
                    ccl_comm* comm,
                    copy_attr attr)
            : sched_entry(sched),
              recv_buf(recv_buf),
              copy_buf(copy_buf),
              bytes(bytes),
              src(src),
              comm(comm),
              attr(attr) {}

    void start() override;
    void update() override;

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           ", recv_buf ",
                           recv_buf,
                           ", copy_buf ",
                           copy_buf,
                           ", bytes ",
                           bytes,
                           ", src ",
                           src,
                           ", atl_tag ",
                           atl_tag,
                           ", comm_id ",
                           sched->get_comm_id(),
                           ", req ",
                           &req,
                           "\n");
    }

private:
    ccl_buffer recv_buf;
    ccl_buffer copy_buf;
    size_t bytes;
    int src;
    ccl_comm* comm;
    copy_attr attr;

    uint64_t atl_tag = 0;
    atl_req_t req{};
};
