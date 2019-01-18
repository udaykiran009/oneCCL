#include "common/global/global.hpp"

#include <new>

mlsl_global_data global_data{};

#ifdef ENABLE_DEBUG
std::atomic_size_t allocations_count;
std::atomic_size_t deallocations_count;

#define REGISTER_NEW() ++allocations_count
#define REGISTER_DELETE() ++deallocations_count

#else

#define REGISTER_NEW()
#define REGISTER_DELETE()

#endif

//todo: globally overloaded operators new and delete are exposed outside of .so, so applications will it
// need to find another way of allocation of aligned memory

void* operator new(std::size_t size)
{
    REGISTER_NEW();
    return MLSL_MALLOC(size, "operator_new");
}

void* operator new[](std::size_t size)
{
    REGISTER_NEW();
    return MLSL_CALLOC(size, "operator_new[]");
}

void* operator new(std::size_t size, const std::nothrow_t& tag)
{
    REGISTER_NEW();
    return MLSL_MEMALIGN_IMPL(size, CACHELINE_SIZE);
}

void* operator new[](std::size_t size, const std::nothrow_t& tag)
{
    REGISTER_NEW();
    return MLSL_CALLOC_IMPL(size, CACHELINE_SIZE);
}

void operator delete(void* ptr)
{
    REGISTER_DELETE();
    MLSL_FREE(ptr);
}

void operator delete[](void* ptr)
{
    REGISTER_DELETE();
    MLSL_FREE(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& tag)
{
    REGISTER_DELETE();
    MLSL_FREE(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& tag)
{
    REGISTER_DELETE();
    MLSL_FREE(ptr);
}
