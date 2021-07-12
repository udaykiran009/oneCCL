#include "common/global/global.hpp"
#include "sched/extra_sched.hpp"

void ccl_extra_sched::dump(std::ostream& out) const {
    if (!ccl::global_data::env().sched_dump) {
        return;
    }

    ccl_sched_base::dump(out, class_name());
    ccl_logger::format(out,
                       ", start_idx: ",
                       start_idx,
                       ", req: ",
                       static_cast<const ccl_request*>(this),
                       ", num_entries: ",
                       entries.size(),
                       "\n");

    std::stringstream msg;
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i]->dump(msg, i);
    }
    out << msg.str();

    ccl_logger::format(out, "--------------------------------\n");
}
