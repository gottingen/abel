//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/file_helper.h>
#include <abel/log/details/null_mutex.h>
#include <abel/asl/format/format.h>
#include <abel/log/sinks/base_sink.h>
#include <abel/log/log.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace abel {
    namespace log {
        namespace sinks {

/*
 * Generator of daily log file names in format basename.YYYY-MM-DD.ext
 */
            struct daily_filename_calculator {
                // Create filename for the form basename.YYYY-MM-DD
                static filename_t calc_filename(const filename_t &filename, const tm &now_tm) {
                    filename_t basename, ext;
                    std::tie(basename, ext) = details::file_helper::split_by_extenstion(filename);
                    std::conditional<std::is_same<filename_t::value_type, char>::value,
                            fmt::memory_buffer,
                            fmt::wmemory_buffer>::type w;
                    fmt::format_to(
                            w,
                            ABEL_LOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}{}"),
                            basename,
                            now_tm.tm_year + 1900,
                            now_tm.tm_mon + 1,
                            now_tm.tm_mday,
                            ext);
                    return fmt::to_string(w);
                }
            };

/*
 * Rotating file sink based on date. rotates at midnight
 */
            template<typename Mutex, typename FileNameCalc = daily_filename_calculator>
            class daily_file_sink ABEL_INHERITANCE_FINAL : public base_sink<Mutex> {
            public:
                // create daily file sink which rotates on given time
                daily_file_sink(filename_t base_filename, int rotation_hour, int rotation_minute, bool truncate = false)
                        : base_filename_(std::move(base_filename)), rotation_h_(rotation_hour),
                          rotation_m_(rotation_minute),
                          truncate_(truncate) {
                    if (rotation_hour < 0 || rotation_hour > 23 || rotation_minute < 0 || rotation_minute > 59) {
                        throw log_ex("daily_file_sink: Invalid rotation time in ctor");
                    }
                    auto now = abel::now();
                    file_helper_.open(FileNameCalc::calc_filename(base_filename_, abel::local_tm(now)), truncate_);
                    rotation_tp_ = next_rotation_tp_();
                }

            protected:
                void sink_it_(const details::log_msg &msg) override {

                    if (msg.time >= rotation_tp_) {
                        file_helper_.open(FileNameCalc::calc_filename(base_filename_, abel::local_tm(msg.time)),
                                          truncate_);
                        rotation_tp_ = next_rotation_tp_();
                    }
                    fmt::memory_buffer formatted;
                    sink::formatter_->format(msg, formatted);
                    file_helper_.write(formatted);
                }

                void flush_() override {
                    file_helper_.flush();
                }

            private:

                abel::abel_time next_rotation_tp_() {
                    auto now = abel::now();
                    tm date = abel::local_tm(now);
                    date.tm_hour = rotation_h_;
                    date.tm_min = rotation_m_;
                    date.tm_sec = 0;
                    auto rotation_time = abel::from_tm(date, abel::local_time_zone());
                    if (rotation_time > now) {
                        return rotation_time;
                    }
                    return {rotation_time + abel::hours(24)};
                }

                filename_t base_filename_;
                int rotation_h_;
                int rotation_m_;
                abel::abel_time rotation_tp_;
                details::file_helper file_helper_;
                bool truncate_;
            };

            using daily_file_sink_mt = daily_file_sink<std::mutex>;
            using daily_file_sink_st = daily_file_sink<details::null_mutex>;

        } // namespace sinks

//
// factory functions
//
        template<typename Factory = default_factory>
        inline std::shared_ptr<logger> daily_logger_mt(
                const std::string &logger_name, const filename_t &filename, int hour = 0, int minute = 0,
                bool truncate = false) {
            return Factory::template create<sinks::daily_file_sink_mt>(logger_name, filename, hour, minute, truncate);
        }

        template<typename Factory = default_factory>
        inline std::shared_ptr<logger> daily_logger_st(
                const std::string &logger_name, const filename_t &filename, int hour = 0, int minute = 0,
                bool truncate = false) {
            return Factory::template create<sinks::daily_file_sink_st>(logger_name, filename, hour, minute, truncate);
        }
    } //namespace log
} // namespace abel
