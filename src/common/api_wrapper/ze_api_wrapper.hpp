#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)

#include <ze_api.h>
#include <zes_api.h>

namespace ccl {

typedef struct ze_lib_ops {
    decltype(zeInit) *zeInit;
    decltype(zeDriverGet) *zeDriverGet;
    decltype(zeDriverGetApiVersion) *zeDriverGetApiVersion;
    decltype(zeMemGetAllocProperties) *zeMemGetAllocProperties;
    decltype(zeMemGetAddressRange) *zeMemGetAddressRange;
    decltype(zeMemAllocHost) *zeMemAllocHost;
    decltype(zeMemAllocDevice) *zeMemAllocDevice;
    decltype(zeMemAllocShared) *zeMemAllocShared;
    decltype(zeMemFree) *zeMemFree;
    decltype(zeMemOpenIpcHandle) *zeMemOpenIpcHandle;
    decltype(zeMemCloseIpcHandle) *zeMemCloseIpcHandle;
    decltype(zeMemGetIpcHandle) *zeMemGetIpcHandle;
    decltype(zeDeviceGet) *zeDeviceGet;
    decltype(zeDeviceGetProperties) *zeDeviceGetProperties;
    decltype(zeDeviceCanAccessPeer) *zeDeviceCanAccessPeer;
    decltype(zeDeviceGetCommandQueueGroupProperties) *zeDeviceGetCommandQueueGroupProperties;
    decltype(zeDeviceGetP2PProperties) *zeDeviceGetP2PProperties;
    decltype(zeDeviceGetGlobalTimestamps) *zeDeviceGetGlobalTimestamps;
    decltype(zeDriverGetProperties) *zeDriverGetProperties;
    decltype(zeDriverGetIpcProperties) *zeDriverGetIpcProperties;
    decltype(zeCommandQueueCreate) *zeCommandQueueCreate;
    decltype(zeCommandQueueExecuteCommandLists) *zeCommandQueueExecuteCommandLists;
    decltype(zeCommandQueueSynchronize) *zeCommandQueueSynchronize;
    decltype(zeCommandQueueDestroy) *zeCommandQueueDestroy;
    decltype(zeCommandListCreate) *zeCommandListCreate;
    decltype(zeCommandListCreateImmediate) *zeCommandListCreateImmediate;
    decltype(zeCommandListAppendMemoryCopy) *zeCommandListAppendMemoryCopy;
    decltype(zeCommandListAppendLaunchKernel) *zeCommandListAppendLaunchKernel;
    decltype(zeCommandListAppendWaitOnEvents) *zeCommandListAppendWaitOnEvents;
    decltype(zeCommandListAppendBarrier) *zeCommandListAppendBarrier;
    decltype(zeCommandListClose) *zeCommandListClose;
    decltype(zeCommandListReset) *zeCommandListReset;
    decltype(zeCommandListDestroy) *zeCommandListDestroy;
    decltype(zeContextCreate) *zeContextCreate;
    decltype(zeContextDestroy) *zeContextDestroy;
    decltype(zeEventPoolCreate) *zeEventPoolCreate;
    decltype(zeEventCreate) *zeEventCreate;
    decltype(zeEventQueryStatus) *zeEventQueryStatus;
    decltype(zeEventHostSynchronize) *zeEventHostSynchronize;
    decltype(zeEventHostReset) *zeEventHostReset;
    decltype(zeEventHostSignal) *zeEventHostSignal;
    decltype(zeEventDestroy) *zeEventDestroy;
    decltype(zeEventPoolOpenIpcHandle) *zeEventPoolOpenIpcHandle;
    decltype(zeEventPoolCloseIpcHandle) *zeEventPoolCloseIpcHandle;
    decltype(zeEventPoolGetIpcHandle) *zeEventPoolGetIpcHandle;
    decltype(zeEventQueryKernelTimestamp) *zeEventQueryKernelTimestamp;
    decltype(zeEventPoolDestroy) *zeEventPoolDestroy;
    decltype(zeFenceHostSynchronize) *zeFenceHostSynchronize;
    decltype(zeFenceCreate) *zeFenceCreate;
    decltype(zeKernelCreate) *zeKernelCreate;
    decltype(zeKernelSetArgumentValue) *zeKernelSetArgumentValue;
    decltype(zeKernelSuggestGroupSize) *zeKernelSuggestGroupSize;
    decltype(zeKernelSetGroupSize) *zeKernelSetGroupSize;
    decltype(zeKernelDestroy) *zeKernelDestroy;
    decltype(zeModuleCreate) *zeModuleCreate;
    decltype(zeModuleDestroy) *zeModuleDestroy;
    decltype(zeModuleBuildLogGetString) *zeModuleBuildLogGetString;
    decltype(zeModuleBuildLogDestroy) *zeModuleBuildLogDestroy;
    decltype(zeDeviceGetComputeProperties) *zeDeviceGetComputeProperties;
    decltype(zeDeviceGetMemoryAccessProperties) *zeDeviceGetMemoryAccessProperties;
    decltype(zeDeviceGetMemoryProperties) *zeDeviceGetMemoryProperties;
    decltype(zeDeviceGetSubDevices) *zeDeviceGetSubDevices;
    decltype(zesDevicePciGetProperties) *zesDevicePciGetProperties;
    decltype(zesDeviceEnumFabricPorts) *zesDeviceEnumFabricPorts;
    decltype(zesFabricPortGetConfig) *zesFabricPortGetConfig;
    decltype(zesFabricPortGetProperties) *zesFabricPortGetProperties;
    decltype(zesFabricPortGetState) *zesFabricPortGetState;
} ze_lib_ops_t;

static std::vector<std::string> ze_fn_names = {
    "zeInit",
    "zeDriverGet",
    "zeDriverGetApiVersion",
    "zeMemGetAllocProperties",
    "zeMemGetAddressRange",
    "zeMemAllocHost",
    "zeMemAllocDevice",
    "zeMemAllocShared",
    "zeMemFree",
    "zeMemOpenIpcHandle",
    "zeMemCloseIpcHandle",
    "zeMemGetIpcHandle",
    "zeDeviceGet",
    "zeDeviceGetProperties",
    "zeDeviceCanAccessPeer",
    "zeDeviceGetCommandQueueGroupProperties",
    "zeDeviceGetP2PProperties",
    "zeDeviceGetGlobalTimestamps",
    "zeDriverGetProperties",
    "zeDriverGetIpcProperties",
    "zeCommandQueueCreate",
    "zeCommandQueueExecuteCommandLists",
    "zeCommandQueueSynchronize",
    "zeCommandQueueDestroy",
    "zeCommandListCreate",
    "zeCommandListCreateImmediate",
    "zeCommandListAppendMemoryCopy",
    "zeCommandListAppendLaunchKernel",
    "zeCommandListAppendWaitOnEvents",
    "zeCommandListAppendBarrier",
    "zeCommandListClose",
    "zeCommandListReset",
    "zeCommandListDestroy",
    "zeContextCreate",
    "zeContextDestroy",
    "zeEventPoolCreate",
    "zeEventCreate",
    "zeEventQueryStatus",
    "zeEventHostSynchronize",
    "zeEventHostReset",
    "zeEventHostSignal",
    "zeEventDestroy",
    "zeEventPoolOpenIpcHandle",
    "zeEventPoolCloseIpcHandle",
    "zeEventPoolGetIpcHandle",
    "zeEventQueryKernelTimestamp",
    "zeEventPoolDestroy",
    "zeFenceHostSynchronize",
    "zeFenceCreate",
    "zeKernelCreate",
    "zeKernelSetArgumentValue",
    "zeKernelSuggestGroupSize",
    "zeKernelSetGroupSize",
    "zeKernelDestroy",
    "zeModuleCreate",
    "zeModuleDestroy",
    "zeModuleBuildLogGetString",
    "zeModuleBuildLogDestroy",
    "zeDeviceGetComputeProperties",
    "zeDeviceGetMemoryAccessProperties",
    "zeDeviceGetMemoryProperties",
    "zeDeviceGetSubDevices",
    "zesDevicePciGetProperties",
    "zesDeviceEnumFabricPorts",
    "zesFabricPortGetConfig",
    "zesFabricPortGetProperties",
    "zesFabricPortGetState",
};

extern ccl::ze_lib_ops_t ze_lib_ops;

#define zeInit                  ccl::ze_lib_ops.zeInit
#define zeDriverGet             ccl::ze_lib_ops.zeDriverGet
#define zeDriverGetApiVersion   ccl::ze_lib_ops.zeDriverGetApiVersion
#define zeMemGetAllocProperties ccl::ze_lib_ops.zeMemGetAllocProperties
#define zeMemGetAddressRange    ccl::ze_lib_ops.zeMemGetAddressRange
#define zeMemAllocHost          ccl::ze_lib_ops.zeMemAllocHost
#define zeMemAllocDevice        ccl::ze_lib_ops.zeMemAllocDevice
#define zeMemAllocShared        ccl::ze_lib_ops.zeMemAllocShared
#define zeMemFree               ccl::ze_lib_ops.zeMemFree
#define zeMemOpenIpcHandle      ccl::ze_lib_ops.zeMemOpenIpcHandle
#define zeMemCloseIpcHandle     ccl::ze_lib_ops.zeMemCloseIpcHandle
#define zeMemGetIpcHandle       ccl::ze_lib_ops.zeMemGetIpcHandle
#define zeDeviceGet             ccl::ze_lib_ops.zeDeviceGet
#define zeDeviceGetProperties   ccl::ze_lib_ops.zeDeviceGetProperties
#define zeDeviceCanAccessPeer   ccl::ze_lib_ops.zeDeviceCanAccessPeer
#define zeDeviceGetCommandQueueGroupProperties \
    ccl::ze_lib_ops.zeDeviceGetCommandQueueGroupProperties
#define zeDeviceGetP2PProperties          ccl::ze_lib_ops.zeDeviceGetP2PProperties
#define zeDeviceGetGlobalTimestamps       ccl::ze_lib_ops.zeDeviceGetGlobalTimestamps
#define zeDriverGetProperties             ccl::ze_lib_ops.zeDriverGetProperties
#define zeDriverGetIpcProperties          ccl::ze_lib_ops.zeDriverGetIpcProperties
#define zeCommandQueueCreate              ccl::ze_lib_ops.zeCommandQueueCreate
#define zeCommandQueueExecuteCommandLists ccl::ze_lib_ops.zeCommandQueueExecuteCommandLists
#define zeCommandQueueSynchronize         ccl::ze_lib_ops.zeCommandQueueSynchronize
#define zeCommandQueueDestroy             ccl::ze_lib_ops.zeCommandQueueDestroy
#define zeCommandListCreate               ccl::ze_lib_ops.zeCommandListCreate
#define zeCommandListCreateImmediate      ccl::ze_lib_ops.zeCommandListCreateImmediate
#define zeCommandListAppendMemoryCopy     ccl::ze_lib_ops.zeCommandListAppendMemoryCopy
#define zeCommandListAppendLaunchKernel   ccl::ze_lib_ops.zeCommandListAppendLaunchKernel
#define zeCommandListAppendWaitOnEvents   ccl::ze_lib_ops.zeCommandListAppendWaitOnEvents
#define zeCommandListAppendBarrier        ccl::ze_lib_ops.zeCommandListAppendBarrier
#define zeCommandListClose                ccl::ze_lib_ops.zeCommandListClose
#define zeCommandListReset                ccl::ze_lib_ops.zeCommandListReset
#define zeCommandListDestroy              ccl::ze_lib_ops.zeCommandListDestroy
#define zeContextCreate                   ccl::ze_lib_ops.zeContextCreate
#define zeContextDestroy                  ccl::ze_lib_ops.zeContextDestroy
#define zeEventPoolCreate                 ccl::ze_lib_ops.zeEventPoolCreate
#define zeEventCreate                     ccl::ze_lib_ops.zeEventCreate
#define zeEventQueryStatus                ccl::ze_lib_ops.zeEventQueryStatus
#define zeEventHostSynchronize            ccl::ze_lib_ops.zeEventHostSynchronize
#define zeEventHostReset                  ccl::ze_lib_ops.zeEventHostReset
#define zeEventHostSignal                 ccl::ze_lib_ops.zeEventHostSignal
#define zeEventDestroy                    ccl::ze_lib_ops.zeEventDestroy
#define zeEventPoolOpenIpcHandle          ccl::ze_lib_ops.zeEventPoolOpenIpcHandle
#define zeEventPoolCloseIpcHandle         ccl::ze_lib_ops.zeEventPoolCloseIpcHandle
#define zeEventPoolGetIpcHandle           ccl::ze_lib_ops.zeEventPoolGetIpcHandle
#define zeEventQueryKernelTimestamp       ccl::ze_lib_ops.zeEventQueryKernelTimestamp
#define zeEventPoolDestroy                ccl::ze_lib_ops.zeEventPoolDestroy
#define zeFenceHostSynchronize            ccl::ze_lib_ops.zeFenceHostSynchronize
#define zeFenceCreate                     ccl::ze_lib_ops.zeFenceCreate
#define zeKernelCreate                    ccl::ze_lib_ops.zeKernelCreate
#define zeKernelSetArgumentValue          ccl::ze_lib_ops.zeKernelSetArgumentValue
#define zeKernelSuggestGroupSize          ccl::ze_lib_ops.zeKernelSuggestGroupSize
#define zeKernelSetGroupSize              ccl::ze_lib_ops.zeKernelSetGroupSize
#define zeKernelDestroy                   ccl::ze_lib_ops.zeKernelDestroy
#define zeModuleCreate                    ccl::ze_lib_ops.zeModuleCreate
#define zeModuleDestroy                   ccl::ze_lib_ops.zeModuleDestroy
#define zeModuleBuildLogGetString         ccl::ze_lib_ops.zeModuleBuildLogGetString
#define zeModuleBuildLogDestroy           ccl::ze_lib_ops.zeModuleBuildLogDestroy
#define zeDeviceGetComputeProperties      ccl::ze_lib_ops.zeDeviceGetComputeProperties
#define zeDeviceGetMemoryAccessProperties ccl::ze_lib_ops.zeDeviceGetMemoryAccessProperties
#define zeDeviceGetMemoryProperties       ccl::ze_lib_ops.zeDeviceGetMemoryProperties
#define zeDeviceGetSubDevices             ccl::ze_lib_ops.zeDeviceGetSubDevices
#define zesDevicePciGetProperties         ccl::ze_lib_ops.zesDevicePciGetProperties
#define zesDeviceEnumFabricPorts          ccl::ze_lib_ops.zesDeviceEnumFabricPorts
#define zesFabricPortGetConfig            ccl::ze_lib_ops.zesFabricPortGetConfig
#define zesFabricPortGetProperties        ccl::ze_lib_ops.zesFabricPortGetProperties
#define zesFabricPortGetState             ccl::ze_lib_ops.zesFabricPortGetState

bool ze_api_init();
void ze_api_fini();

} //namespace ccl

#endif //CCL_ENABLE_SYCL && CCL_ENABLE_ZE
