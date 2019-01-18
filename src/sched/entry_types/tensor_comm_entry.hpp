#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"
#include "out_of_order/ooo_match.hpp"

class tensor_comm_entry : public sched_entry
{
public:
    tensor_comm_entry() = delete;
    tensor_comm_entry(out_of_order::ooo_match* ooo_handler,
                      const char* tensor_name)
        : ooo_handler(ooo_handler), tensor_name(tensor_name)
    {}

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            MLSL_LOG(DEBUG, "tensor name %s", tensor_name);
            ooo_handler->create_comm_and_run_sched(tensor_name);
            status = mlsl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return "TENSOR_COMM";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<tensor_comm_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "tensor %s\n", tensor_name);
        return dump_buf + bytes_written;
    }

private:
    out_of_order::ooo_match* ooo_handler;
    const char* tensor_name;
};
