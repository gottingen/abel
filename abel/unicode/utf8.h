// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_UNICODE_UTF8_H_
#define ABEL_UNICODE_UTF8_H_


#include <cstdint>
#include <stdexcept>

namespace abel {

// Supported combinations:
//   0xxx_xxxx
//   110x_xxxx 10xx_xxxx
//   1110_xxxx 10xx_xxxx 10xx_xxxx
//   1111_0xxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
//   1111_10xx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
//   1111_110x 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
struct utf8 final {

    static size_t const max_unicode_symbol_size = 4;
    static size_t const max_supported_symbol_size = 6;

    static uint32_t const max_supported_code_point = 0x7FFFFFFF;

    using char_type = uint8_t;


    enum utf8_states_t {
        S_STRT = 0, S_RJCT = 8
    };

    static bool in_range(uint32_t c, uint32_t lo, uint32_t hi) {
        return (static_cast<uint32_t>(c - lo) < (hi - lo + 1));
    }

    static bool is_surrogate(uint32_t c) {
        return in_range(c, 0xd800, 0xdfff);
    }

    static bool is_high_surrogate(uint32_t c) {
        return (c & 0xfffffc00) == 0xd800;
    }

    static bool is_low_surrogate(uint32_t c) {
        return (c & 0xfffffc00) == 0xdc00;
    }

    template<typename PeekFn>
    static size_t char_size(PeekFn &&peek_fn) {
        char_type const ch0 = std::forward<PeekFn>(peek_fn)();
        if (ch0 < 0x80) // 0xxx_xxxx
            return 1;
        if (ch0 < 0xC0)
            throw std::runtime_error("The utf8 first char in sequence is incorrect");
        if (ch0 < 0xE0) // 110x_xxxx 10xx_xxxx
            return 2;
        if (ch0 < 0xF0) // 1110_xxxx 10xx_xxxx 10xx_xxxx
            return 3;
        if (ch0 < 0xF8) // 1111_0xxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            return 4;
        if (ch0 < 0xFC) // 1111_10xx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            return 5;
        if (ch0 < 0xFE) // 1111_110x 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            return 6;
        throw std::runtime_error("The utf8 first char in sequence is incorrect");
    }

    template<typename ReadFn>
    static uint32_t read(ReadFn &&read_fn) {
        char_type const ch0 = read_fn();
        if (ch0 < 0x80) // 0xxx_xxxx
            return ch0;
        if (ch0 < 0xC0)
            throw std::runtime_error("The utf8 first char in sequence is incorrect");
        if (ch0 < 0xE0) {
            // 110x_xxxx 10xx_xxxx
            char_type const ch1 = read_fn();
            if (ch1 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            return (ch0 << 6) + ch1 - 0x3080;
        }
        if (ch0 < 0xF0) // 1110_xxxx 10xx_xxxx 10xx_xxxx
        {
            char_type const ch1 = read_fn();
            if (ch1 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch2 = read_fn();
            if (ch2 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            return (ch0 << 12) + (ch1 << 6) + ch2 - 0xE2080;
        }
        if (ch0 < 0xF8) {
            // 1111_0xxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            char_type const ch1 = read_fn();
            if (ch1 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch2 = read_fn();
            if (ch2 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch3 = read_fn();
            if (ch3 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            return (ch0 << 18) + (ch1 << 12) + (ch2 << 6) + ch3 - 0x3C82080;
        }
        if (ch0 < 0xFC) {
            // 1111_10xx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            char_type const ch1 = read_fn();
            if (ch1 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch2 = read_fn();
            if (ch2 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch3 = read_fn();
            if (ch3 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch4 = read_fn();
            if (ch4 >> 6 != 2) throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            return (ch0 << 24) + (ch1 << 18) + (ch2 << 12) + (ch3 << 6) + ch4 - 0xFA082080;
        }
        // 1111_110x 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
        if (ch0 < 0xFE) {
            char_type const ch1 = read_fn();
            if (ch1 >> 6 != 2)
                throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch2 = read_fn();
            if (ch2 >> 6 != 2)
                throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch3 = read_fn();
            if (ch3 >> 6 != 2)
                throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch4 = read_fn();
            if (ch4 >> 6 != 2)
                throw std::runtime_error("The utf8 slave char in sequence is incorrect");
            char_type const ch5 = read_fn();
            if (ch5 >> 6 != 2)
                throw std::runtime_error("The utf8 slave char in sequence is incorrect");

            return (ch0 << 30) + (ch1 << 24) + (ch2 << 18) + (ch3 << 12) + (ch4 << 6) + ch5 - 0x82082080;
        }
        throw std::runtime_error("The utf8 first char in sequence is incorrect");

    }

    template<typename WriteFn>
    static void write(uint32_t const cp, WriteFn &&write_fn) {
        if (cp < 0x80)          // 0xxx_xxxx
            write_fn(static_cast<char_type>(cp));
        else if (cp < 0x800) {
            // 110x_xxxx 10xx_xxxx
            write_fn(static_cast<char_type>(0xC0 | cp >> 6));
            write_fn(static_cast<char_type>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            // 1110_xxxx 10xx_xxxx 10xx_xxxx
            write_fn(static_cast<char_type>(0xE0 | cp >> 12));
            write_fn(static_cast<char_type>(0x80 | (cp >> 6 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x200000) {
            // 1111_0xxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            write_fn(static_cast<char_type>(0xF0 | cp >> 18));
            write_fn(static_cast<char_type>(0x80 | (cp >> 12 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 6 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x4000000) {
            // 1111_10xx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            write_fn(static_cast<char_type>(0xF8 | cp >> 24));
            write_fn(static_cast<char_type>(0x80 | (cp >> 18 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 12 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 6 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x80000000) {
            // 1111_110x 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx 10xx_xxxx
            write_fn(static_cast<char_type>(0xFC | cp >> 30));
            write_fn(static_cast<char_type>(0x80 | (cp >> 24 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 18 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 12 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp >> 6 & 0x3F)));
            write_fn(static_cast<char_type>(0x80 | (cp & 0x3F)));
        } else
            throw std::runtime_error("Tool large UTF8 code point");
        return;
    }
};

}  // namespace abel

#endif  // ABEL_UNICODE_UTF8_H_
