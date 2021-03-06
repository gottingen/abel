// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_CHRONO_CYCLECLOCK_H_
#define ABEL_CHRONO_CYCLECLOCK_H_

#include <cstdint>

#include "abel/base/profile.h"

namespace abel {
namespace chrono_internal {

class cycle_clock {
public:
    // cycle_clock::now()
    //
    // Returns the value of a cycle counter that counts at a rate that is
    // approximately constant.
    static int64_t now();

    // cycle_clock::frequency()
    //
    // Returns the amount by which `cycle_clock::now()` increases per second. Note
    // that this value may not necessarily match the core CPU clock frequency.
    static double frequency();

private:
    cycle_clock() = delete;  // no instances
    cycle_clock(const cycle_clock &) = delete;

    cycle_clock &operator=(const cycle_clock &) = delete;
};

using cycle_clock_source_func = int64_t (*)();

class cycle_clock_source {
private:
    static void clock_register(cycle_clock_source_func source);
};

}  // namespace chrono_internal
}  // namespace abel

#endif  // ABEL_CHRONO_CYCLECLOCK_H_
