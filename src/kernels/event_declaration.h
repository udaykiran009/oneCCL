#ifdef HOST_CTX
#define __global

#include <memory>
using namespace ccl;

template <class native_type>
struct shared_event_traits {};

#else
typedef ushort bfloat16;
#endif

typedef struct __attribute__((packed)) shared_event_float {
    __global int* produced_bytes;
    __global float* mem_chunk;
} shared_event_float;

#ifdef HOST_CTX

template <>
struct shared_event_traits<float> {
    using impl_t = shared_event_float;
};

#endif
