

#pragma once

#include <abel/log/common.h>
#include <abel/log/details/console_globals.h>
#include <abel/log/details/null_mutex.h>
#include <abel/log/sinks/sink.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <wincon.h>

namespace abel {
    namespace log {
        namespace sinks {
/*
 * Windows color console sink. Uses WriteConsoleA to write to the console with
 * colors
 */
            template<typename OutHandle, typename ConsoleMutex>
            class wincolor_sink : public sink {
            public:
                const WORD BOLD = FOREGROUND_INTENSITY;
                const WORD RED = FOREGROUND_RED;
                const WORD GREEN = FOREGROUND_GREEN;
                const WORD CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE;
                const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                const WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;

                wincolor_sink()
                        : out_handle_(OutHandle::handle()), mutex_(ConsoleMutex::mutex()) {
                    colors_[trace] = WHITE;
                    colors_[debug] = CYAN;
                    colors_[info] = GREEN;
                    colors_[warn] = YELLOW | BOLD;
                    colors_[err] = RED | BOLD;                         // red bold
                    colors_[critical] = BACKGROUND_RED | WHITE | BOLD; // white bold on red background
                    colors_[off] = 0;
                }

                ~wincolor_sink() override {
                    this->flush();
                }

                wincolor_sink(const wincolor_sink &other) = delete;

                wincolor_sink &operator=(const wincolor_sink &other) = delete;

                // change the color for the given level
                void set_color(level_enum level, WORD color) {
                    std::lock_guard<mutex_t> lock(mutex_);
                    colors_[level] = color;
                }

                void log(const details::log_msg &msg) ABEL_INHERITANCE_FINAL override {
                    std::lock_guard<mutex_t> lock(mutex_);
                    fmt::memory_buffer formatted;
                    formatter_->format(msg, formatted);
                    if (msg.color_range_end > msg.color_range_start) {
                        // before color range
                        print_range_(formatted, 0, msg.color_range_start);

                        // in color range
                        auto orig_attribs = set_console_attribs(colors_[msg.level]);
                        print_range_(formatted, msg.color_range_start, msg.color_range_end);
                        ::SetConsoleTextAttribute(out_handle_,
                                                  orig_attribs); // reset to orig colors
                        // after color range
                        print_range_(formatted, msg.color_range_end, formatted.size());
                    } else // print without colors if color range is invalid
                    {
                        print_range_(formatted, 0, formatted.size());
                    }
                }

                void flush() ABEL_INHERITANCE_FINAL override {
                    // windows console always flushed?
                }

                void set_pattern(const std::string &pattern) override ABEL_INHERITANCE_FINAL {
                    std::lock_guard<mutex_t> lock(mutex_);
                    formatter_ = std::unique_ptr<abel::formatter>(new pattern_formatter(pattern));
                }

                void set_formatter(std::unique_ptr<abel::formatter> sink_formatter) override ABEL_INHERITANCE_FINAL {
                    std::lock_guard<mutex_t> lock(mutex_);
                    formatter_ = std::move(sink_formatter);
                }

            private:
                using mutex_t = typename ConsoleMutex::mutex_t;

                // set color and return the orig console attributes (for resetting later)
                WORD set_console_attribs(WORD attribs) {
                    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
                    ::GetConsoleScreenBufferInfo(out_handle_, &orig_buffer_info);
                    WORD back_color = orig_buffer_info.wAttributes;
                    // retrieve the current background color
                    back_color &= static_cast<WORD>(~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                                                      FOREGROUND_INTENSITY));
                    // keep the background color unchanged
                    ::SetConsoleTextAttribute(out_handle_, attribs | back_color);
                    return orig_buffer_info.wAttributes; // return orig attribs
                }

                // print a range of formatted message to console
                void print_range_(const fmt::memory_buffer &formatted, size_t start, size_t end) {
                    auto size = static_cast<DWORD>(end - start);
                    ::WriteConsoleA(out_handle_, formatted.data() + start, size, nullptr, nullptr);
                }

                HANDLE out_handle_;
                mutex_t &mutex_;
                std::unordered_map<level_enum, WORD, level_hasher> colors_;
            };

            using wincolor_stdout_sink_mt = wincolor_sink<details::console_stdout, details::console_mutex>;
            using wincolor_stdout_sink_st = wincolor_sink<details::console_stdout, details::console_nullmutex>;

            using wincolor_stderr_sink_mt = wincolor_sink<details::console_stderr, details::console_mutex>;
            using wincolor_stderr_sink_st = wincolor_sink<details::console_stderr, details::console_nullmutex>;

        } // namespace sinks
    } //namespace log
} // namespace abel
