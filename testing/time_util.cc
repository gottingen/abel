// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "testing/time_util.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include "abel/log/logging.h"
#include "abel/chrono/internal/zone_info_source.h"

namespace abel {

namespace chrono_internal {

abel::time_zone load_time_zone(const std::string &name) {
    abel::time_zone tz;
    DCHECK(load_time_zone(name, &tz), name.c_str());
    return tz;
}

}  // namespace chrono_internal

}  // namespace abel

namespace abel {

namespace chrono_internal {
namespace {

// Embed the zoneinfo data for time zones used during tests and benchmarks.
// The data was generated using "xxd -i zoneinfo-file".  There is no need
// to update the data as long as the tests do not depend on recent changes
// (and the past rules remain the same).

#include "abel/chrono/internal/zoneinfo.inc"

const struct ZoneInfo {
    const char *name;
    const char *data;
    std::size_t length;
} kZoneInfo[] = {
        // The three real time zones used by :time_test and :time_benchmark.
        {"America/Los_Angeles",  //
         reinterpret_cast<char *>(America_Los_Angeles), America_Los_Angeles_len},
        {"America/New_York",  //
         reinterpret_cast<char *>(America_New_York), America_New_York_len},
        {"Australia/Sydney",  //
         reinterpret_cast<char *>(Australia_Sydney), Australia_Sydney_len},

        // Other zones named in tests but which should fail to load.
        {"Invalid/time_zone", nullptr, 0},
        {"", nullptr, 0},

        // Also allow for loading the local time zone under TZ=US/Pacific.
        {"US/Pacific",  //
         reinterpret_cast<char *>(America_Los_Angeles), America_Los_Angeles_len},

        // Allows use of the local time zone from a system-specific location.
#ifdef _MSC_VER
        {"localtime",  //
         reinterpret_cast<char*>(America_Los_Angeles), America_Los_Angeles_len},
#else
        {"/etc/localtime",  //
         reinterpret_cast<char *>(America_Los_Angeles), America_Los_Angeles_len},
#endif
};

class TestZoneInfoSource : public abel::chrono_internal::zone_info_source {
  public:
    TestZoneInfoSource(const char *data, std::size_t size)
            : data_(data), end_(data + size) {}

    std::size_t read(void *ptr, std::size_t size) override {
        const std::size_t len = std::min<std::size_t>(size, end_ - data_);
        memcpy(ptr, data_, len);
        data_ += len;
        return len;
    }

    int skip(std::size_t offset) override {
        data_ += std::min<std::size_t>(offset, end_ - data_);
        return 0;
    }

  private:
    const char *data_;
    const char *const end_;
};

std::unique_ptr<abel::chrono_internal::zone_info_source> TestFactory(
        const std::string &name,
        const std::function<std::unique_ptr<abel::chrono_internal::zone_info_source>(
                const std::string &name)> & /*fallback_factory*/) {
    for (const ZoneInfo &zoneinfo : kZoneInfo) {
        if (name == zoneinfo.name) {
            if (zoneinfo.data == nullptr)
                return nullptr;
            return std::unique_ptr<abel::chrono_internal::zone_info_source>(
                    new TestZoneInfoSource(zoneinfo.data, zoneinfo.length));
        }
    }
    DLOG_CRITICAL("Unexpected time zone \"{}\" in test", name.c_str());
    return nullptr;
}

}  // namespace

#if !defined(__MINGW32__)
// MinGW does not support the weak symbol extension mechanism.
ZoneInfoSourceFactory zone_info_source_factory = TestFactory;
#endif

}  // namespace chrono_internal

}  // namespace abel


void increment(char *s) {
    for (int i = static_cast<int>(std::strlen(s)) - 1; i >= 0; --i) {
        if (s[i] != '9') {
            ++s[i];
            break;
        }
        s[i] = '0';
    }
}

std::string get_system_error(int error_code) {
#if defined(__MINGW32__) || !defined(_WIN32)
    return strerror(error_code);
#else
    enum { BUFFER_SIZE = 200 };
  char buffer[BUFFER_SIZE];
  if (strerror_s(buffer, BUFFER_SIZE, error_code))
    throw std::exception("strerror_s failed");
  return buffer;
#endif
}

const char *const FILE_CONTENT = "Don't panic!";

abel::buffered_file open_buffered_file(FILE **fp) {
    abel::file read_end, write_end;
    abel::file::pipe(read_end, write_end);
    write_end.write(FILE_CONTENT, std::strlen(FILE_CONTENT));
    write_end.close();
    abel::buffered_file f = read_end.fdopen("r");
    if (fp)
        *fp = f.get();
    return f;
}

