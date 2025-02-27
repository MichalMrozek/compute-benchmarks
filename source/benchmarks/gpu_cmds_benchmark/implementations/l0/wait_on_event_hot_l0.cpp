/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/wait_on_event_hot.h"

#include <gtest/gtest.h>

static TestResult run(const WaitOnEventHotArguments &arguments, Statistics &statistics) {
    LevelZero levelzero;
    const uint64_t timerResolution = levelzero.getTimerResoultion(levelzero.device);

    // Create buffer
    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    const auto bufferSize = sizeof(uint64_t) * 2;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize))
    uint64_t *beginTimestamp = static_cast<uint64_t *>(buffer);
    uint64_t *endTimestamp = beginTimestamp + 1;

    // Create event and signal it
    const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 1};
    const ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(event));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    for (auto commandIndex = 0u; commandIndex < arguments.measuredCommands; commandIndex++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(cmdList, 1, &event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        auto commandTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        commandTime *= timerResolution;
        commandTime /= arguments.measuredCommands;
        statistics.pushValue(commandTime, MeasurementUnit::Microseconds, MeasurementType::Gpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<WaitOnEventHot> registerTestCase(run, Api::L0);
