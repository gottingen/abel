// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef TEST_TESTING_ATOMIC_HOOK_TEST_HELPER_H_
#define TEST_TESTING_ATOMIC_HOOK_TEST_HELPER_H_

#include "abel/atomic/atomic_hook.h"

namespace abel {

namespace atomic_hook_internal {

using VoidF = void (*)();
extern abel::atomic_hook<VoidF> func;
extern int default_func_calls;

void DefaultFunc();

void RegisterFunc(VoidF func);

}  // namespace atomic_hook_internal

}  // namespace abel

#endif  // TEST_TESTING_ATOMIC_HOOK_TEST_HELPER_H_
