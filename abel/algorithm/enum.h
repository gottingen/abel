// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_ALGORITHM_ENUM_H_
#define ABEL_ALGORITHM_ENUM_H_

#include <type_traits>
#include <functional>
#include <cstddef>

namespace abel {

template<typename T>
class enum_hash {
    static_assert(std::is_enum<T>::value, "must be an enum");
  public:
    std::size_t operator()(const T &e) const {
        using utype = typename std::underlying_type<T>::type;
        return std::hash<utype>()(static_cast<utype>(e));
    }
};
}  // namespace abel

#endif  // ABEL_ALGORITHM_ENUM_H_
