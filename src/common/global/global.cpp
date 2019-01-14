#include "common/global/global.hpp"

#include <new>

mlsl_global_data global_data{};

void* operator new(std::size_t size)
{
    return MLSL_MALLOC(size, "operator_new");
}

void* operator new[](std::size_t size)
{
    return MLSL_CALLOC(size, "operator_new[]");
}

void* operator new(std::size_t size, const std::nothrow_t& tag)
{
    return MLSL_MEMALIGN_IMPL(size, CACHELINE_SIZE);
}

void* operator new[](std::size_t size, const std::nothrow_t& tag)
{
    return MLSL_CALLOC_IMPL(size, CACHELINE_SIZE);
}

void operator delete(void* ptr)
{
    MLSL_FREE(ptr);
}

void operator delete[](void* ptr)
{
    MLSL_FREE(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& tag)
{
    MLSL_FREE(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& tag)
{
    MLSL_FREE(ptr);
}
