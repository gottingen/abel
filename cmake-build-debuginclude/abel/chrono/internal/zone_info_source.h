// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   https://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifndef ABEL_CHRONO_INTERNAL_ZONE_INFO_SOURCE_H_
#define ABEL_CHRONO_INTERNAL_ZONE_INFO_SOURCE_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <abel/base/profile.h>

namespace abel {

namespace chrono_internal {

// A stdio-like interface for providing zoneinfo data for a particular zone.
class zone_info_source {
public:
    virtual ~zone_info_source ();

    virtual std::size_t read (void *ptr, std::size_t size) = 0;  // like fread()
    virtual int skip (std::size_t offset) = 0;                   // like fseek()

    // Until the zoneinfo data supports versioning information, we provide
    // a way for a ZoneInfoSource to indicate it out-of-band.  The default
    // implementation returns an empty std::string.
    virtual std::string version () const;
};

}  // namespace chrono_internal

}  // namespace abel

namespace abel {

namespace chrono_internal {

// A function-pointer type for a factory that returns a ZoneInfoSource
// given the name of a time zone and a fallback factory.  Returns null
// when the data for the named zone cannot be found.
using ZoneInfoSourceFactory =
std::unique_ptr<abel::chrono_internal::zone_info_source> (*) (
    const std::string &,
    const std::function<std::unique_ptr<
        abel::chrono_internal::zone_info_source> (const std::string &)> &);

// The user can control the mapping of zone names to zoneinfo data by
// providing a definition for zone_info_source_factory.
// For example, given functions my_factory() and my_other_factory() that
// can return a ZoneInfoSource for a named zone, we could inject them into
// abel::chrono_internal::load_time_zone() with:
//
//   namespace {
//   std::unique_ptr<abel::chrono_internal::ZoneInfoSource> CustomFactory(
//       const std::string& name,
//       const std::function<std::unique_ptr<abel::chrono_internal::ZoneInfoSource>(
//           const std::string& name)>& fallback_factory) {
//     if (auto zip = my_factory(name)) return zip;
//     if (auto zip = fallback_factory(name)) return zip;
//     if (auto zip = my_other_factory(name)) return zip;
//     return nullptr;
//   }
//   }  // namespace
//   ZoneInfoSourceFactory zone_info_source_factory = CustomFactory;
//
// This might be used, say, to use zoneinfo data embedded in the program,
// or read from a (possibly compressed) file archive, or both.
//
// zone_info_source_factory() will be called:
//   (1) from the same thread as the abel::chrono_internal::load_time_zone() call,
//   (2) only once for any zone name, and
//   (3) serially (i.e., no concurrent execution).
//
// The fallback factory obtains zoneinfo data by reading files in ${TZDIR},
// and it is used automatically when no zone_info_source_factory definition
// is linked into the program.
extern ZoneInfoSourceFactory zone_info_source_factory;

}  // namespace chrono_internal

}  // namespace abel

#endif  // ABEL_CHRONO_INTERNAL_ZONE_INFO_SOURCE_H_
