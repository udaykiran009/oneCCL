#ifndef ENV_HPP
#define ENV_HPP

#include "common.hpp"

namespace MLSL
{
    typedef struct EnvData
    {
        int logLevel;
        int dupGroup;
        int enableStats;
        int autoConfigType;
    } EnvData __attribute__ ((aligned (CACHELINE_SIZE)));
    extern EnvData envData;

    void ParseEnvVars();
    void PrintEnvVars();
}

#endif /* ENV_HPP */
