// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/random/internal/wide_multiply.h"

#include "gtest/gtest.h"
#include "abel/base/int128.h"

using abel::random_internal::MultiplyU64ToU128;

namespace {

    TEST(WideMultiplyTest, MultiplyU64ToU128Test) {
        constexpr uint64_t k1 = 1;
        constexpr uint64_t kMax = ~static_cast<uint64_t>(0);

        EXPECT_EQ(abel::uint128(0), MultiplyU64ToU128(0, 0));

        // Max uint64
        EXPECT_EQ(MultiplyU64ToU128(kMax, kMax),
                  abel::make_uint128(0xfffffffffffffffe, 0x0000000000000001));
        EXPECT_EQ(abel::make_uint128(0, kMax), MultiplyU64ToU128(kMax, 1));
        EXPECT_EQ(abel::make_uint128(0, kMax), MultiplyU64ToU128(1, kMax));
        for (int i = 0; i < 64; ++i) {
            EXPECT_EQ(abel::make_uint128(0, kMax) << i,
                      MultiplyU64ToU128(kMax, k1 << i));
            EXPECT_EQ(abel::make_uint128(0, kMax) << i,
                      MultiplyU64ToU128(k1 << i, kMax));
        }

        // 1-bit x 1-bit.
        for (int i = 0; i < 64; ++i) {
            for (int j = 0; j < 64; ++j) {
                EXPECT_EQ(abel::make_uint128(0, 1) << (i + j),
                          MultiplyU64ToU128(k1 << i, k1 << j));
                EXPECT_EQ(abel::make_uint128(0, 1) << (i + j),
                          MultiplyU64ToU128(k1 << i, k1 << j));
            }
        }

        // Verified multiplies
        EXPECT_EQ(MultiplyU64ToU128(0xffffeeeeddddcccc, 0xbbbbaaaa99998888),
                  abel::make_uint128(0xbbbb9e2692c5dddc, 0xc28f7531048d2c60));
        EXPECT_EQ(MultiplyU64ToU128(0x0123456789abcdef, 0xfedcba9876543210),
                  abel::make_uint128(0x0121fa00ad77d742, 0x2236d88fe5618cf0));
        EXPECT_EQ(MultiplyU64ToU128(0x0123456789abcdef, 0xfdb97531eca86420),
                  abel::make_uint128(0x0120ae99d26725fc, 0xce197f0ecac319e0));
        EXPECT_EQ(MultiplyU64ToU128(0x97a87f4f261ba3f2, 0xfedcba9876543210),
                  abel::make_uint128(0x96fbf1a8ae78d0ba, 0x5a6dd4b71f278320));
        EXPECT_EQ(MultiplyU64ToU128(0xfedcba9876543210, 0xfdb97531eca86420),
                  abel::make_uint128(0xfc98c6981a413e22, 0x342d0bbf48948200));
    }

}  // namespace