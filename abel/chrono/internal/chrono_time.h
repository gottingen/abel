// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_
#define ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_

namespace abel {

namespace chrono_internal {

static inline int64_t get_current_time_nanos_from_system() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now() -
            std::chrono::system_clock::from_time_t(0)).count();
}

}  // namespace chrono_internal
}  // namespace abel

#endif  // ABEL_CHRONO_INTERNAL_CHRONO_TIME_H_
