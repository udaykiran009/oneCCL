#ifndef LP_H
#define LP_H

#define CCL_GPU_BF16_TRUNCATE

#ifdef CCL_GPU_BF16_TRUNCATE

float __bf16_to_fp32(ushort V) {
    uint temp = convert_uint(V) << 16;
    return as_float(temp);
}

ushort __fp32_to_bf16(float V) {
    // Truncate for now TODO: Proper rounding
    ushort2 temp = as_ushort2(V);
    return temp.s1;
}

#else
#include "rne.h"
#endif

#endif /* LP_H */
