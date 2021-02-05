#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

#include "export_configuration.hpp"

struct tracer {
    tracer() : output(global_output) {
        is_error = false;
    }

    ~tracer() {
        if (is_error) {
            dump();
        }
    }

    void set_error(const std::string& message) const {
        error_str = std::string("\n") + message + "\n";
        is_error = true;
    }

    void dump() const {
        std::stringstream ss;
        ss << output.rdbuf();
        std::cerr << "\n/**********************************************"
                     "Test Error***************************************\n"
                     "Test Case Last Error:\t"
                  << error_str << "Full Log:\n"
                  << ss.str() << std::endl;
    }

    std::ostream& output;
    mutable std::string error_str;
    mutable bool is_error;
};

namespace utils {

std::string to_string(const ccl::device_index_type& device_id) {
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

ccl::device_index_type from_string(const std::string& device_id_str) {
    std::string::size_type from_pos = device_id_str.find('[');
    if (from_pos == std::string::npos) {
        throw std::invalid_argument(std::string("Cannot get ccl::device_index_type from input: ") +
                                    device_id_str + ". Index should be decorated by \"[...]\"");
    }

    if (device_id_str.size() == 1) {
        throw std::invalid_argument(
            std::string("Cannot get ccl::device_index_type from input, too less string: ") +
            device_id_str);
    }
    from_pos++;

    ccl::device_index_type path(
        ccl::unused_index_value, ccl::unused_index_value, ccl::unused_index_value);

    size_t cur_index = 0;
    do {
        std::string::size_type to_pos = device_id_str.find(':', from_pos);
        std::string::size_type count =
            (to_pos != std::string::npos ? to_pos - from_pos : std::string::npos);
        std::string index_string(device_id_str, from_pos, count);
        switch (cur_index) {
            case ccl::device_index_enum::driver_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "driver index invalid: ") +
                        device_id_str);
                }
                std::get<ccl::device_index_enum::driver_index_id>(path) = index;
                break;
            }
            case ccl::device_index_enum::device_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "device index invalid: ") +
                        device_id_str);
                }
                std::get<ccl::device_index_enum::device_index_id>(path) = index;
                break;
            }
            case ccl::device_index_enum::subdevice_index_id: {
                auto index = std::atoll(index_string.c_str());
                std::get<ccl::device_index_enum::subdevice_index_id>(path) =
                    index < 0 ? ccl::unused_index_value : index;
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

template <typename T>
void str_to_array(const std::string& input, std::vector<T>& output, char delimiter) {
    if (input.empty()) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.push_back(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}
template <>
void str_to_array(const std::string& input, std::vector<std::string>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.push_back(processes_input);
    }
}

template <typename T>
void str_to_mset(const std::string& input, std::multiset<T>& output, char delimiter) {
    if (input.empty()) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.insert(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}

template <>
void str_to_mset(const std::string& input,
                 std::multiset<ccl::device_index_type>& output,
                 char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.insert(utils::from_string(processes_input));
    }
}

inline std::vector<uint8_t> load_binary_file(const std::string& source_path) {
    std::ifstream stream(source_path, std::ios::in | std::ios::binary);

    std::vector<uint8_t> binary_file;
    if (!stream.good()) {
        std::string error("Failed to load binary file: ");
        error += source_path;
        throw std::runtime_error(error);
    }

    size_t length = 0;
    stream.seekg(0, stream.end);
    length = static_cast<size_t>(stream.tellg());
    stream.seekg(0, stream.beg);

    binary_file.resize(length);
    stream.read(reinterpret_cast<char*>(binary_file.data()), length);
    return binary_file;
}

inline int readFromSocket(int socket, unsigned char* buffer, size_t size) {
    size_t bytesRead = 0;
    int result;
    while (bytesRead < size) {
        result = static_cast<int>(read(socket, buffer + bytesRead, size - bytesRead));
        if (result < 0) {
            return -1;
        }

        bytesRead += static_cast<int>(result);
    }
    return 0;
}

inline int writeToSocket(int socket, unsigned char* buffer, size_t size) {
    size_t bytesWritten = 0;
    int result;
    while (bytesWritten < size) {
        result = static_cast<int>(write(socket, buffer + bytesWritten, size - bytesWritten));
        if (result < 0) {
            return -1;
        }

        bytesWritten += static_cast<int>(result);
    }
    return 0;
}
} // namespace utils
