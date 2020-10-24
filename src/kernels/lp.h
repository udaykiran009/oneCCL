#ifndef LP_H
#define LP_H

#ifdef LIFT_EMBARGO
#include "rne.h"
#else
float __bf16_to_fp32(ushort V) {
    uint temp = convert_uint(V) << 16;
    return as_float(temp);
}

ushort __fp32_to_bf16(float V) {
    // Truncate for now TODO: Proper rounding
    ushort2 temp = as_ushort2(V);
    return temp.s1;
}

#endif
#endif /* LP_H */
