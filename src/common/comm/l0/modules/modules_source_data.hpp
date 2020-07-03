#pragma once
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "coll/algorithms/algorithms_enum.hpp"
#include "common/comm/l0/device_group_routing_schema.hpp"

namespace native {
using source_data_t = std::vector<uint8_t>;
using source_data_storage_t = std::map<ccl::device_topology_type, source_data_t>;

template <ccl_coll_type module_type>
struct typed_source_data_storage_t : source_data_storage_t {
    static constexpr ccl_coll_type get_type() {
        return module_type;
    }
};

source_data_t load_binary_file(const std::string& source_path);

template <ccl_coll_type... modules_types>
class modules_src_container {
public:
    using modules_src_tuple = std::tuple<typed_source_data_storage_t<modules_types>...>;

    static modules_src_container& instance() {
        static modules_src_container inst;
        return inst;
    }

    template <ccl_coll_type type>
    void load_kernel_source(const std::string& source_path, ccl::device_topology_type method) {
        static_assert(type < std::tuple_size<modules_src_tuple>::value,
                      " Module of 'type' is not supported");
        auto binary_file = load_binary_file(source_path);
        typed_source_data_storage_t<type>& typed_storage = std::get<type>(storage);

        auto ret = typed_storage.insert({ method, std::move(binary_file) });
        if (!ret.second) {
            throw std::runtime_error(std::string("Kernel type \"") + ccl_coll_type_to_str(type) +
                                     "\", with \"" + ::to_string(method) +
                                     "\" algo exist already. File: " + source_path +
                                     " is not loaded!");
        }
    };

    const modules_src_tuple& get_modules() const {
        return storage;
    }

    template <ccl_coll_type type>
    const source_data_storage_t& get_modules_collection() const noexcept {
        static_assert(type < std::tuple_size<modules_src_tuple>::value,
                      " Module of 'type' is not supported");
        return std::get<type>(storage);
    }

private:
    modules_src_container() = default;
    modules_src_tuple storage;
};
} // namespace native
