#include "atl/atl_def.h"

std::map<atl_mnic_t, std::string> mnic_type_names = { std::make_pair(ATL_MNIC_NONE, "none"),
                                                      std::make_pair(ATL_MNIC_LOCAL, "local"),
                                                      std::make_pair(ATL_MNIC_GLOBAL, "global") };

std::map<atl_mnic_offset_t, std::string> mnic_offset_names = {
    std::make_pair(ATL_MNIC_OFFSET_NONE, "none"),
    std::make_pair(ATL_MNIC_OFFSET_LOCAL_PROC_IDX, "local_proc_idx")
};

std::string to_string(atl_mnic_t type) {
    auto it = mnic_type_names.find(type);
    if (it != mnic_type_names.end()) {
        return it->second;
    }
    else {
        return "unknown";
    }
}

std::string to_string(atl_mnic_offset_t offset) {
    auto it = mnic_offset_names.find(offset);
    if (it != mnic_offset_names.end()) {
        return it->second;
    }
    else {
        return "unknown";
    }
}
