#pragma once

#define NATIVE_TYPE(NAME, T) \
    namespace ccl { \
    template <> \
    struct native_type_info<a2a_gpu_comm_data_##NAME> { \
        static constexpr bool is_supported = true; \
        static constexpr bool is_class = true; \
    }; \
    }

NATIVE_TYPE(int8, int8_t)
NATIVE_TYPE(uint8, uint8_t)
NATIVE_TYPE(int16, int16_t)
NATIVE_TYPE(uint16, uint16_t)
NATIVE_TYPE(int32, int32_t)
NATIVE_TYPE(uint32, uint32_t)
NATIVE_TYPE(int64, int64_t)
NATIVE_TYPE(uint64, uint64_t)
NATIVE_TYPE(float32, float)
NATIVE_TYPE(float64, double)

#undef NATIVE_TYPE
