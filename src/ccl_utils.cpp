#include <sstream>
#include <stdexcept>

#include "ccl.hpp"
#include "ccl_type_traits.hpp"

std::ostream& operator<<(std::ostream& out, const ccl::device_index_type& index) {
    out << ccl::to_string(index);
    return out;
}

namespace ccl {

CCL_API
std::string to_string(const device_index_type& device_id) {
    std::stringstream ss;
    ss << "[" << std::get<ccl::device_index_enum::driver_index_id>(device_id) << ":"
       << std::get<ccl::device_index_enum::device_index_id>(device_id) << ":";

    auto subdevice_id = std::get<ccl::device_index_enum::subdevice_index_id>(device_id);
    if (subdevice_id == ccl::unused_index_value) {
        ss << "*";
    }
    else {
        ss << std::get<ccl::device_index_enum::subdevice_index_id>(device_id);
    }
    ss << "]";

    return ss.str();
}

CCL_API
device_index_type from_string(const std::string& device_id_str) {
    std::string::size_type from_pos = device_id_str.find('[');
    if (from_pos == std::string::npos) {
        throw std::invalid_argument(std::string("Cannot get ccl::device_index_type from input: ") +
                                    device_id_str);
    }

    if (device_id_str.size() == 1) {
        throw std::invalid_argument(
            std::string("Cannot get ccl::device_index_type from input, too less: ") +
            device_id_str);
    }
    from_pos++;

    device_index_type path(unused_index_value, unused_index_value, unused_index_value);

    size_t cur_index = 0;
    do {
        std::string::size_type to_pos = device_id_str.find(':', from_pos);
        std::string::size_type count =
            (to_pos != std::string::npos ? to_pos - from_pos : std::string::npos);
        std::string index_string(device_id_str, from_pos, count);
        switch (cur_index) {
            case device_index_enum::driver_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "driver index invalid: ") +
                        device_id_str);
                }
                std::get<device_index_enum::driver_index_id>(path) = index;
                break;
            }
            case device_index_enum::device_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "device index invalid: ") +
                        device_id_str);
                }
                std::get<device_index_enum::device_index_id>(path) = index;
                break;
            }
            case device_index_enum::subdevice_index_id: {
                auto index = std::atoll(index_string.c_str());
                std::get<device_index_enum::subdevice_index_id>(path) =
                    index < 0 ? unused_index_value : index;
                break;
            }
            default:
                throw std::invalid_argument(
                    std::string("Cannot get ccl::device_index_type from input, "
                                "unsupported format: ") +
                    device_id_str);
        }

        cur_index++;
        if (device_id_str.size() > to_pos) {
            to_pos++;
        }
        from_pos = to_pos;
    } while (from_pos < device_id_str.size());

    return path;
}
} // namespace ccl
