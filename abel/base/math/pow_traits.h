// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_MATH_POW_TRAITS_H_
#define ABEL_BASE_MATH_POW_TRAITS_H_

#include "abel/base/profile.h"


namespace abel {

template<typename Integral>
ABEL_CONSTEXPR bool is_power_of_two_template(Integral i) {
    /*
    if (i < 0)
        return false;
    return (i == 0 || !(i & (i - 1)));
    */
    return (i < 0 ? false : i == 0 ?
                            true : !(i & (i - 1)) ?
                                   true : false);
}

/// is_power_of_two()
/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(int i) {
    return is_power_of_two_template(i);
}

/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(unsigned int i) {
    return is_power_of_two_template(i);
}

/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(long i) {
    return is_power_of_two_template(i);
}

/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(unsigned long i) {
    return is_power_of_two_template(i);
}

/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(long long i) {
    return is_power_of_two_template(i);
}

/// does what it says: true if i is a power of two
ABEL_CONSTEXPR bool is_power_of_two(unsigned long long i) {
    return is_power_of_two_template(i);
}
}
#endif  // ABEL_BASE_MATH_POW_TRAITS_H_
