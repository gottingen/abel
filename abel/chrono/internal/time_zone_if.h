// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CHRONO_INTERNAL_TIME_ZONE_IF_H_
#define ABEL_CHRONO_INTERNAL_TIME_ZONE_IF_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include "abel/base/profile.h"
#include "abel/chrono/internal/chrono_time_internal.h"
#include "abel/chrono/internal/time_zone.h"

namespace abel {
namespace chrono_internal {


// A simple interface used to hide time-zone complexities from time_zone::Impl.
// Subclasses implement the functions for civil-time conversions in the zone.
class time_zone_if {
public:
    // A factory function for time_zone_if implementations.
    static std::unique_ptr<time_zone_if> load(const std::string &name);

    virtual ~time_zone_if();

    virtual time_zone::absolute_lookup break_time(
            const time_point<seconds> &tp) const = 0;

    virtual time_zone::civil_lookup make_time(const civil_second &cs) const = 0;

    virtual bool next_transition(const time_point<seconds> &tp,
                                 time_zone::civil_transition *trans) const = 0;

    virtual bool prev_transition(const time_point<seconds> &tp,
                                 time_zone::civil_transition *trans) const = 0;

    virtual std::string version() const = 0;

    virtual std::string description() const = 0;

protected:
    time_zone_if() {}
};

// Convert between time_point<seconds> and a count of seconds since the
// Unix epoch.  We assume that the std::chrono::system_clock and the
// Unix clock are second aligned, but not that they share an epoch.
ABEL_FORCE_INLINE std::int_fast64_t to_unix_seconds(const time_point<seconds> &tp) {
    return (tp - std::chrono::time_point_cast<seconds>(
            std::chrono::system_clock::from_time_t(0)))
            .count();
}

ABEL_FORCE_INLINE time_point<seconds> from_unix_seconds(std::int_fast64_t t) {
    return std::chrono::time_point_cast<seconds>(
            std::chrono::system_clock::from_time_t(0)) +
           seconds(t);
}


}  // namespace chrono_internal
}  // namespace abel
#endif  // ABEL_CHRONO_INTERNAL_TIME_ZONE_IF_H_
