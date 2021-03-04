#pragma once

#include "multi_process_fixture.hpp"

#ifdef STANDALONE_UT
namespace ccl {
template <class type>
struct native_type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};
} // namespace ccl
#else
#include "oneapi/ccl/type_traits.hpp"
#endif

#include "lp.hpp"

using bfloat16 = ccl::bfloat16;
using float16 = ccl::float16;

#define DECLARE_KERNEL_TYPE(COLL) \
    template <class T> \
    struct COLL##_param_traits;

#define DECLARE_OP_KERNEL_TYPE(COLL) \
    template <class T, class Op> \
    struct COLL##_param_traits;

DECLARE_KERNEL_TYPE(allgatherv)
DECLARE_OP_KERNEL_TYPE(allreduce)
DECLARE_KERNEL_TYPE(alltoallv)
DECLARE_KERNEL_TYPE(bcast)
DECLARE_OP_KERNEL_TYPE(reduce)
DECLARE_OP_KERNEL_TYPE(reduce_scatter)

template <class T>
struct my_sum {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs + rhs;
    }

    static constexpr T init() {
        return 0;
    }
};

template <class T>
struct my_prod {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs * rhs;
    }

    static constexpr T init() {
        return 1;
    }
};

template <class T>
struct my_min {
    T operator()(const T& lhs, const T& rhs) const {
        return std::min(lhs, rhs);
    }

    static constexpr T init() {
        return std::numeric_limits<T>::max();
    }
};

template <class T>
struct my_max {
    T operator()(const T& lhs, const T& rhs) const {
        return std::max(lhs, rhs);
    }

    static constexpr T init() {
        return std::numeric_limits<T>::min();
    }
};

template <>
struct my_sum<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(bf16_to_fp32(lhs) + bf16_to_fp32(rhs));
    }

    static constexpr bfloat16 init() {
        return bfloat16(0);
    }
};

template <>
struct my_prod<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(bf16_to_fp32(lhs) * bf16_to_fp32(rhs));
    }

    static constexpr bfloat16 init() {
        return bfloat16(1);
    }
};

template <>
struct my_min<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(std::min(bf16_to_fp32(lhs), bf16_to_fp32(rhs)));
    }

    static constexpr bfloat16 init() {
        return bfloat16(0x7f7f);
    }
};

template <>
struct my_max<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(std::max(bf16_to_fp32(lhs), bf16_to_fp32(rhs)));
    }

    static constexpr bfloat16 init() {
        return bfloat16(0xff7f);
    }
};

template <>
struct my_sum<float16> {
    float16 operator()(const float16& lhs, const float16& rhs) const {
        return fp32_to_fp16(fp16_to_fp32(lhs) + fp16_to_fp32(rhs));
    }

    static constexpr float16 init() {
        return float16(0);
    }
};

template <>
struct my_prod<float16> {
    float16 operator()(const float16& lhs, const float16& rhs) const {
        return fp32_to_fp16(fp16_to_fp32(lhs) * fp16_to_fp32(rhs));
    }

    static constexpr float16 init() {
        return float16(1);
    }
};

template <>
struct my_min<float16> {
    float16 operator()(const float16& lhs, const float16& rhs) const {
        return fp32_to_fp16(std::min(fp16_to_fp32(lhs), fp16_to_fp32(rhs)));
    }

    static constexpr float16 init() {
        return float16(0x7bff);
    }
};

template <>
struct my_max<float16> {
    float16 operator()(const float16& lhs, const float16& rhs) const {
        return fp32_to_fp16(std::max(fp16_to_fp32(lhs), fp16_to_fp32(rhs)));
    }

    static constexpr float16 init() {
        return float16(0xfbff);
    }
};

#define DEFINE_KERNEL_TYPE(NAME, T, COLL) \
    template <> \
    struct COLL##_param_traits<T> { \
        static constexpr const char* kernel_name = #COLL "_execution_" #NAME; \
        static constexpr const char* ipc_kernel_name = #COLL "_execution_" #NAME; \
    };

#define DEFINE_KERNEL_TYPES(COLL) \
    DEFINE_KERNEL_TYPE(int8, int8_t, COLL) \
    DEFINE_KERNEL_TYPE(uint8, uint8_t, COLL) \
    DEFINE_KERNEL_TYPE(int16, int16_t, COLL) \
    DEFINE_KERNEL_TYPE(uint16, uint16_t, COLL) \
    DEFINE_KERNEL_TYPE(int32, int32_t, COLL) \
    DEFINE_KERNEL_TYPE(uint32, uint32_t, COLL) \
    DEFINE_KERNEL_TYPE(int64, int64_t, COLL) \
    DEFINE_KERNEL_TYPE(uint64, uint64_t, COLL) \
    DEFINE_KERNEL_TYPE(float16, float16, COLL) \
    DEFINE_KERNEL_TYPE(float32, float, COLL) \
    DEFINE_KERNEL_TYPE(float64, double, COLL) \
    DEFINE_KERNEL_TYPE(bfloat16, bfloat16, COLL)

#define DEFINE_KERNEL_TYPE_FOR_OP(NAME, T, COLL, OP) \
    template <> \
    struct COLL##_param_traits<T, my_##OP<T>> { \
        static constexpr const char* kernel_name = #COLL "_execution_" #NAME "_" #OP; \
        static constexpr const char* ipc_kernel_name = #COLL "_execution_" #NAME "_" #OP; \
        using op_type = my_##OP<T>; \
    };

#define DEFINE_KERNEL_TYPES_FOR_OP(COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(int8, int8_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(uint8, uint8_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(int16, int16_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(uint16, uint16_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(int32, int32_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(uint32, uint32_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(int64, int64_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(uint64, uint64_t, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(float16, float16, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(float32, float, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(float64, double, COLL, OP) \
    DEFINE_KERNEL_TYPE_FOR_OP(bfloat16, bfloat16, COLL, OP)

using TestTypes = ::testing::Types<int8_t,
                                   uint8_t,
                                   int16_t,
                                   uint16_t,
                                   int32_t,
                                   uint32_t,
                                   int64_t,
                                   uint64_t,
                                   float16,
                                   float,
                                   double,
                                   bfloat16>;

using TestTypesA2A = ::testing::Types<float>;

#define DEFINE_PAIR(T, Op) std::pair<T, Op>

#define DEFINE_TYPE(T) \
    DEFINE_PAIR(T, my_sum<T>), DEFINE_PAIR(T, my_prod<T>), DEFINE_PAIR(T, my_min<T>), \
        DEFINE_PAIR(T, my_max<T>)

// Note: don't use float with mult op as the rounding error gets
// noticeable quite fast
using TestTypesAndOps = ::testing::Types<DEFINE_PAIR(int8_t, my_sum<int8_t>),
                                         //DEFINE_PAIR(uint8_t, my_prod<uint8_t>),
                                         DEFINE_PAIR(int16_t, my_min<int16_t>),
                                         //DEFINE_PAIR(uint16_t, my_max<uint16_t>),
                                         //DEFINE_PAIR(int32_t, my_sum<int32_t>),
                                         DEFINE_PAIR(uint32_t, my_prod<uint32_t>),
                                         //DEFINE_PAIR(int64_t, my_min<int64_t>),
                                         //DEFINE_PAIR(uint64_t, my_max<uint64_t>),
                                         DEFINE_PAIR(float, my_sum<float>),
                                         DEFINE_PAIR(double, my_min<double>),
                                         DEFINE_PAIR(bfloat16, my_sum<bfloat16>),
                                         DEFINE_PAIR(bfloat16, my_max<bfloat16>),
                                         DEFINE_PAIR(float16, my_sum<float16>),
                                         DEFINE_PAIR(float16, my_max<float16>)>;
