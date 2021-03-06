// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_MATH_IS_EVEN_H_
#define ABEL_BASE_MATH_IS_EVEN_H_

#include "abel/base/math/is_odd.h"

namespace abel {


ABEL_CONSTEXPR bool is_even(const long long x) ABEL_NOEXCEPT {
    return !is_odd(x);
}

}  // namespace abel

#endif  // ABEL_BASE_MATH_IS_EVEN_H_
