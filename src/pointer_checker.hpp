#ifndef POINTER_CHECKER_HPP
#define POINTER_CHECKER_HPP

#include <list>

using namespace std;

namespace MLSL
{
    enum PointerCheckerResultType
    {
        PCRT_NONE         = 0,
        PCRT_UNKNOWN_PTR  = 1,
        PCRT_OUT_OF_RANGE = 2,
    };

    typedef struct BufInfo
    {
        char*  start;
        char*  stop;
        size_t size;
    } BufInfo;

    class PointerChecker
    {
    private:
        list<BufInfo> bufList;

    public:
        PointerChecker();
        ~PointerChecker();
        void Add(void* ptr, size_t size);
        void Remove(void* ptr);
        PointerCheckerResultType CheckInternal(void* ptr, size_t size);
        void Check(void* ptr, size_t size);
        void Print(PointerCheckerResultType result);
    };
}

#endif /* POINTER_CHECKER_HPP */
