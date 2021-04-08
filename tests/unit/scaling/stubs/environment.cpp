#include "oneapi/ccl.hpp"
namespace ccl {
namespace v1 {
size_t get_datatype_size(ccl::datatype dtype) {
    size_t size = 0;
    switch (dtype) {
        case ccl::datatype::int8: size = sizeof(int8_t); break;
        case ccl::datatype::uint8: size = sizeof(uint8_t); break;
        case ccl::datatype::int16: size = sizeof(int16_t); break;
        case ccl::datatype::uint16: size = sizeof(uint16_t); break;
        case ccl::datatype::int32: size = sizeof(int32_t); break;
        case ccl::datatype::uint32: size = sizeof(uint32_t); break;
        case ccl::datatype::int64: size = sizeof(int64_t); break;
        case ccl::datatype::uint64: size = sizeof(uint64_t); break;
        case ccl::datatype::float16: size = sizeof(uint16_t); break;
        case ccl::datatype::float32: size = sizeof(float); break;
        case ccl::datatype::float64: size = sizeof(double); break;
        case ccl::datatype::bfloat16: size = sizeof(uint16_t); break;
        default: size = 0;
    }
    return size;
}
} // namespace v1
} // namespace ccl