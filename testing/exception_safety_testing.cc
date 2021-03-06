// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/exception_safety_testing.h"
#include "gtest/gtest.h"
#include "abel/meta/type_traits.h"

#ifdef ABEL_HAVE_EXCEPTIONS

namespace testing {

exceptions_internal::NoThrowTag nothrow_ctor;

exceptions_internal::StrongGuaranteeTagType strong_guarantee;

exceptions_internal::ExceptionSafetyTestBuilder<> MakeExceptionSafetyTester() {
    return {};
}

namespace exceptions_internal {

int countdown = -1;

ConstructorTracker *ConstructorTracker::current_tracker_instance_ = nullptr;

void MaybeThrow(std::string_view msg, bool throw_bad_alloc) {
    if (countdown-- == 0) {
        if (throw_bad_alloc)
            throw TestBadAllocException(msg);
        throw TestException(msg);
    }
}

testing::AssertionResult FailureMessage(const TestException &e,
                                        int countDown) noexcept {
    return testing::AssertionFailure() << "Exception thrown from " << e.what();
}

std::string GetSpecString(TypeSpec spec) {
    std::string out;
    std::string_view sep;
    const auto append = [&](std::string_view s) {
        abel::string_append(&out, sep, s);
        sep = " | ";
    };
    if (static_cast<bool>(TypeSpec::kNoThrowCopy & spec)) {
        append("kNoThrowCopy");
    }
    if (static_cast<bool>(TypeSpec::kNoThrowMove & spec)) {
        append("kNoThrowMove");
    }
    if (static_cast<bool>(TypeSpec::kNoThrowNew & spec)) {
        append("kNoThrowNew");
    }
    return out;
}

std::string GetSpecString(AllocSpec spec) {
    return static_cast<bool>(AllocSpec::kNoThrowAllocate & spec)
           ? "kNoThrowAllocate"
           : "";
}

}  // namespace exceptions_internal

}  // namespace testing

#endif  // ABEL_HAVE_EXCEPTIONS
