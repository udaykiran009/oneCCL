#pragma once

#include <string>

namespace ccl {

struct float16 {
    constexpr float16() : data(0) {}
    constexpr float16(uint16_t v) : data(v) {}
    uint16_t data;

    friend std::ostream& operator<<(std::ostream& out, const float16& v) {
        out << v.data;
        return out;
    }

    friend bool operator==(const float16& v1, const float16& v2) {
        return (v1.data == v2.data) ? true : false;
    }

    friend bool operator!=(const float16& v1, const float16& v2) {
        return !(v1 == v2);
    }

} __attribute__((packed));

struct bfloat16 {
    constexpr bfloat16() : data(0) {}
    constexpr bfloat16(uint16_t v) : data(v) {}
    uint16_t data;

    friend std::ostream& operator<<(std::ostream& out, const bfloat16& v) {
        out << v.data;
        return out;
    }

    friend bool operator==(const bfloat16& v1, const bfloat16& v2) {
        return (v1.data == v2.data) ? true : false;
    }

    friend bool operator!=(const bfloat16& v1, const bfloat16& v2) {
        return !(v1 == v2);
    }

} __attribute__((packed));

std::string to_string(const float16& v);
std::string to_string(const bfloat16& v);

} // namespace ccl
