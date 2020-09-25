#pragma once

#define NATIVE_TYPE(T) \
    namespace ccl { \
    template <> \
    struct native_type_info<a2a_gpu_comm_data_##T> { \
        static constexpr bool is_supported = true; \
        static constexpr bool is_class = true; \
    }; \
    }

NATIVE_TYPE(char)
NATIVE_TYPE(uchar)

NATIVE_TYPE(short)
NATIVE_TYPE(ushort)

NATIVE_TYPE(int)
NATIVE_TYPE(uint)

NATIVE_TYPE(long)
NATIVE_TYPE(ulong)

NATIVE_TYPE(float)
NATIVE_TYPE(double)

#undef NATIVE_TYPE
