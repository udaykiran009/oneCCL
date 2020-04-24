#include <stdexcept>
#include <fstream>
#include "common/comm/l0/modules/modules_source_data.hpp"

namespace native
{
source_data_t load_binary_file(const std::string& source_path)
{
    std::ifstream stream(source_path, std::ios::in | std::ios::binary);

    source_data_t binary_file;
    if (!stream.good())
    {
        std::string error("Failed to load binary file: ");
        error += source_path;

        throw std::runtime_error(error);
    }

    size_t length = 0;
    stream.seekg(0, stream.end);
    length = static_cast<size_t>(stream.tellg());
    stream.seekg(0, stream.beg);

    binary_file.resize(length);
    stream.read(reinterpret_cast<char *>(binary_file.data()), length);
    return binary_file;
}
}
