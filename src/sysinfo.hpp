#ifndef MLSL_SYSINFO_HPP
#define MLSL_SYSINFO_HPP

#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

namespace MLSL
{
    enum CpuType
    {
        CPU_NONE = 0,
        XEON     = 1,
        XEON_PHI = 2
    };

    enum NetType
    {
        NET_NONE = 0,
        ETH      = 1,
        MLX      = 2,
        HFI      = 3
    };

    enum AutoConfigType
    {
        NO_AUTO_CONFIG          = 0,
        NET_AUTO_CONFIG         = 1,
        CPU_AUTO_CONFIG         = 2,
        NET_AND_CPU_AUTO_CONFIG = 3
    };

    struct CpuInfo
    {
        enum CpuType cpuType;
        size_t cpuCoresCount;
        size_t cpuThreadsCount;
    };

    struct NetInfo
    {
        enum NetType netType;
    };

    string ConvertToCpuName(CpuType cpuType);
    string ConvertToNetName(NetType netType);

    class SysInfo
    {
    private:
        vector<NetInfo> netInfo;
        CpuInfo cpuInfo;
        void SetCpuInfo();
        void SetNetInfo();
        NetType ConvertToNetType(char* netName);
        CpuType ConvertToCpuType(char* cpuName);
        struct Property { string name; };
        void GetCpuPropetyValue(Property property, char* propertyValue, FILE* file);

    public:
        SysInfo();
        ~SysInfo();
        void Initialize();
        NetInfo GetNetInfo();
        CpuInfo GetCpuInfo() { return cpuInfo;};
    };
}

#endif /*MLSL_SYSINFO_HPP*/
