// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_SYSTEM_ERROR_CODE_H_
#define ABEL_SYSTEM_ERROR_CODE_H_

namespace abel {

extern int describe_customized_errno(int, const char *, const char *);

const char *abel_error(int error_code);

const char *abel_error();

}  // namespace abel

template<int error_code>
class abel_errno_helper {
};

#define ABEL_REGISTER_ERRNO(error_code, description)                   \
    const int ABEL_ALLOW_UNUSED ABEL_CONCAT(abel_errno_dummy_, __LINE__) =              \
        ::abel::describe_customized_errno((error_code), #error_code, (description)); \
    template <> class abel_errno_helper<(int)(error_code)> {};

#endif  // ABEL_SYSTEM_ERROR_CODE_H_
