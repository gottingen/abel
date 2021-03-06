// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <cassert>
#include <cmath>
#include <cstdint>
#include "abel/base/math/log.h"
#include "gtest/gtest.h"


#if defined(__native_client__) || defined(__EMSCRIPTEN__)
// NACL has a less accurate implementation of std::log2 than most of
// the other platforms. For some values which should have integral results,
// sometimes NACL returns slightly larger values.
//
// The MUSL libc used by emscripten also has a similar bug.
#define ABEL_LOG2
#endif

TEST(math, log2_floor) {
    using abel::log2_floor;
    constexpr uint64_t kZero = 0;
    EXPECT_EQ(0, log2_floor(0));  // boundary. return 0.
    EXPECT_EQ(0, log2_floor(1));
    EXPECT_EQ(1, log2_floor(2));
    EXPECT_EQ(63, log2_floor(~kZero));

    EXPECT_EQ(log2_floor(uint16_t{0}), 0);
    EXPECT_EQ(log2_floor(uint16_t{1}), 0);
    EXPECT_EQ(log2_floor(uint16_t{2}), 1);
    EXPECT_EQ(log2_floor(uint16_t{3}), 1);
    EXPECT_EQ(log2_floor(uint16_t{4}), 2);
    EXPECT_EQ(log2_floor(uint16_t{5}), 2);
    EXPECT_EQ(log2_floor(std::numeric_limits<uint64_t>::max()), 63);

// A boundary case: Converting 0xffffffffffffffff requires > 53
// bits of precision, so the conversion to double rounds up,
// and the result of std::log2(x) > IntLog2Floor(x).
    EXPECT_LT(log2_floor(~kZero), static_cast<int>(std::log2(~kZero)));

    for (int i = 0; i < 64; i++) {
        const uint64_t i_pow_2 = static_cast<uint64_t>(1) << i;
        EXPECT_EQ(i, log2_floor(i_pow_2));
        EXPECT_EQ(i, static_cast<int>(std::log2(i_pow_2)));

        uint64_t y = i_pow_2;
        for (int j = i - 1; j > 0; --j) {
            y = y | (i_pow_2 >> j);
            EXPECT_EQ(i, log2_floor(y));
        }
    }
}

TEST(math, log2_ceil) {
    using abel::log2_ceil;
    constexpr uint64_t kZero = 0;
    EXPECT_EQ(0, log2_ceil(0));  // boundary. return 0.
    EXPECT_EQ(0, log2_ceil(1));
    EXPECT_EQ(1, log2_ceil(2));
    EXPECT_EQ(64, log2_ceil(~kZero));

// A boundary case: Converting 0xffffffffffffffff requires > 53
// bits of precision, so the conversion to double rounds up,
// and the result of std::log2(x) > IntLog2Floor(x).
    EXPECT_LE(log2_ceil(~kZero), static_cast<int>(std::log2(~kZero)));

    for (int i = 0; i < 64; i++) {
        const uint64_t i_pow_2 = static_cast<uint64_t>(1) << i;
        EXPECT_EQ(i, log2_ceil(i_pow_2));
#ifndef ABEL_LOG2
        EXPECT_EQ(i, static_cast<int>(std::ceil(std::log2(i_pow_2))));
#endif

        uint64_t y = i_pow_2;
        for (int j = i - 1; j > 0; --j) {
            y = y | (i_pow_2 >> j);
            EXPECT_EQ(i + 1, log2_ceil(y));
        }
    }
}

TEST(FastMathTest, StirlingLogFactorial) {
    using abel::stirling_log_factorial;

    EXPECT_NEAR(stirling_log_factorial(1.0), 0, 1e-3);
    EXPECT_NEAR(stirling_log_factorial(1.50), 0.284683, 1e-3);
    EXPECT_NEAR(stirling_log_factorial(2.0), 0.69314718056, 1e-4);

    for (int i = 2; i < 50; i++) {
        double d = static_cast<double>(i);
        EXPECT_NEAR(stirling_log_factorial(d), std::lgamma(d + 1), 3e-5);
    }
}
