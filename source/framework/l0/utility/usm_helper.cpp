/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/utility/usm_helper.h"

namespace L0::UsmHelper {

ze_result_t allocate(UsmMemoryPlacement placement, LevelZero &levelZero, size_t size, void **buffer) {
    const ze_host_mem_alloc_desc_t hostAllocDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return zeMemAllocDevice(levelZero.context, &deviceAllocDesc, size, 0, levelZero.device, buffer);
    case UsmMemoryPlacement::Host:
        return zeMemAllocHost(levelZero.context, &hostAllocDesc, size, 0, buffer);
    case UsmMemoryPlacement::Shared:
        return zeMemAllocShared(levelZero.context, &deviceAllocDesc, &hostAllocDesc, size, 0, levelZero.device, buffer);
    case UsmMemoryPlacement::NonUsmImported: {
        *buffer = malloc(size);
        return levelZero.importHostPointer.importExternalPointer(levelZero.driver, *buffer, size);
    }
    case UsmMemoryPlacement::NonUsm: {
        *buffer = malloc(size);
        return ZE_RESULT_SUCCESS;
    }
    default:
        FATAL_ERROR("Unknown placement");
    }
}

ze_result_t allocate(UsmRuntimeMemoryPlacement runtimePlacement, LevelZero &levelZero, size_t size, void **buffer) {
    UsmMemoryPlacement placement{};
    switch (runtimePlacement) {
    case UsmRuntimeMemoryPlacement::Device:
        placement = UsmMemoryPlacement::Device;
        break;
    case UsmRuntimeMemoryPlacement::Host:
        placement = UsmMemoryPlacement::Host;
        break;
    case UsmRuntimeMemoryPlacement::Shared:
        placement = UsmMemoryPlacement::Shared;
        break;
    default:
        FATAL_ERROR("Unknown placement");
    }
    return allocate(placement, levelZero, size, buffer);
}

ze_result_t deallocate(UsmMemoryPlacement placement, LevelZero &levelZero, void *buffer) {
    if (placement == UsmMemoryPlacement::NonUsm) {
        free(buffer);
        return ZE_RESULT_SUCCESS;
    } else if (placement == UsmMemoryPlacement::NonUsmImported) {
        auto ret = levelZero.importHostPointer.releaseExternalPointer(levelZero.driver, buffer);
        free(buffer);
        return ret;
    } else {
        return zeMemFree(levelZero.context, buffer);
    }
}

ze_result_t allocate(DeviceSelection placement, LevelZero &levelzero, size_t size, void **outBuffer) {
    const DeviceSelection gpuDevice = DeviceSelectionHelper::withoutHost(placement);
    const auto gpuDevicesCount = DeviceSelectionHelper::getDevicesCount(gpuDevice);
    FATAL_ERROR_IF(gpuDevicesCount > 1, "USM allocations can have 0 or 1 gpu device");

    const bool hasDevice = gpuDevicesCount == 1;
    const bool hasHost = DeviceSelectionHelper::hasDevice(placement, DeviceSelection::Host);

    const ze_host_mem_alloc_desc_t hostAllocDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};

    if (hasDevice && hasHost) {
        return zeMemAllocShared(levelzero.context, &deviceAllocDesc, &hostAllocDesc, size, 0, levelzero.getDevice(gpuDevice), outBuffer);
    }

    if (hasDevice) {
        return zeMemAllocDevice(levelzero.context, &deviceAllocDesc, size, 0, levelzero.getDevice(gpuDevice), outBuffer);
    }

    if (hasHost) {
        return zeMemAllocHost(levelzero.context, &hostAllocDesc, size, 0, outBuffer);
    }

    FATAL_ERROR("USM allocations need at least one storage location");
}

} // namespace L0::UsmHelper
