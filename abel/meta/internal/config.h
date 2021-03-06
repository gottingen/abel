// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_META_INTERNAL_CONFIG_H_
#define ABEL_META_INTERNAL_CONFIG_H_

#include <type_traits>
#include "abel/base/profile.h"

#ifndef TSL_VARIABLE_TEMPLATES_ENABLED
    #if defined(ABEL_COMPILER_NO_VARIABLE_TEMPLATES)
        #define TSL_VARIABLE_TEMPLATES_ENABLED 0
    #else
        #define TSL_VARIABLE_TEMPLATES_ENABLED 1
    #endif
#endif

#ifndef TSL_TEMPLATE_ALIASES_ENABLED

    #ifdef ABEL_COMPILER_NO_TEMPLATE_ALIASES
        #define TSL_TEMPLATE_ALIASES_ENABLED 0
    #else
        #define TSL_TEMPLATE_ALIASES_ENABLED 1
    #endif

#endif  // TSL_TEMPLATE_ALIASES_ENABLED


// In MSVC we can't probe std::hash or stdext::hash because it triggers a
// static_assert instead of failing substitution. Libc++ prior to 4.0
// also used a static_assert.
//
#if defined(_MSC_VER) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 4000 && _LIBCPP_STD_VER > 11)
    #define TSL_STD_HASH_SFINAE_FRIENDLY 0
#else
    #define TSL_STD_HASH_SFINAE_FRIENDLY 1
#endif


#endif  // ABEL_META_INTERNAL_CONFIG_H_
