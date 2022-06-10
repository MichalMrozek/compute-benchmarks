/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/stream_after_transfer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<StreamAfterTransfer> registerTestCase{};

class StreamAfterTransferTest : public ::testing::TestWithParam<std::tuple<StreamMemoryType, size_t, bool>> {
};

TEST_P(StreamAfterTransferTest, Test) {
    StreamAfterTransferArguments args;
    args.api = Api::OpenCL;
    args.type = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.useEvents = std::get<2>(GetParam());

    StreamAfterTransfer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    StreamAfterTransferTest,
    StreamAfterTransferTest,
    ::testing::Combine(
        ::testing::ValuesIn(StreamMemoryTypeArgument::enumValues),
        ::testing::Values(1 * megaByte, 8 * megaByte, 16 * megaByte, 32 * megaByte, 64 * megaByte, 128 * megaByte, 256 * megaByte, 512 * megaByte, 1 * gigaByte),
        ::testing::Values(false, true)));
