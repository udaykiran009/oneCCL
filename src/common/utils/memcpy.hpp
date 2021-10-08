#pragma once

#include <cstddef>

namespace ccl {

void memcpy(void* dst, const void* src, size_t size);
void memcpy_nontemporal(void* dst, const void* src, size_t size)
#if CCL_AVX_TARGET_ATTRIBUTES
    __attribute__((target("avx2")))
#endif // CCL_AVX_TARGET_ATTRIBUTES
    ;

} // namespace ccl
