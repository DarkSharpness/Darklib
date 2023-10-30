#pragma once

#include "common.h"
#include "concept.h"

#include <compare>
#include <climits>
#include <cstring>

namespace dark {


/**
 * @brief A constexpr version of std::memcpy/std::memmove.
 * @tparam _IsMove Whether to use std::memmove.
 * @param __dst Destination pointer.
 * @param __src Source pointer.
 * @param __n Count of objects to copy. (Not bytes)
 * @return Pointer to the destination.
 */
template <bool _IsMove = true, class _Tp>
[[__gnu__::__always_inline__, maybe_unused]]
constexpr _Tp *copy_range(_Tp *__dst,const _Tp *__src, size_t __n) noexcept {
    if consteval {
        if (__dst == __src) return __dst;
        if (__dst < __src || __dst >= __src + __n) {
            _Tp *__bak = __dst;
            while(__n--) *__dst++ = *__src++;
            __dst = __bak;
        } else { /* dst in [src, src + n), so copy backward. */
            __dst = __dst + __n;
            __src = __src + __n;
            while(__n--) *--__dst = *--__src;
        } return __dst; /* End of constexpr case. */
    }
    if constexpr (_IsMove) {
        return (_Tp *)std::memmove(__dst, __src, __n * sizeof(_Tp)); 
    } else {
        return (_Tp *)std::memcpy(__dst, __src, __n * sizeof(_Tp));
    }
}


/**
 * @brief A constexpr version of std::memset.
 * @param __dst Destination pointer. 
 * @param __val Value to set. (Supported for integral types only.)
 * @param __n Count of objects to set. (Not bytes)
 * @return Pointer to the destination.
 */
template <class _Tp>
requires std::integral <_Tp>
[[__gnu__::__always_inline__, maybe_unused]]
constexpr _Tp *fill_data(_Tp *__dst, int __val, size_t __n) noexcept {
    if consteval {
        _Tp __dat {};
        for (size_t i = 0 ; i != sizeof(_Tp) ; ++i)
            __dat = (__dat << CHAR_BIT) | __val;
        __dst = __dst + __n;
        while(__n--) *--__dst = __dat;
        return __dst;
    } else {
        return (_Tp *)std::memset(__dst, __val, __n * sizeof (_Tp));
    }
}


/**
 * @brief A constexpr version of std::memcmp.
 * @param __lhs Left hand side pointer.
 * @param __rhs Right hand side pointer.
 * @param __n Count of objects to compare. (Not bytes)
 * @return Custom compare_three_way_result_t.
 */
template <class _Tp>
requires requires (_Tp __x) { {__x <=> __x} -> std::same_as <std::strong_ordering>; }
[[__gnu__::__always_inline__, maybe_unused]]
constexpr std::strong_ordering
    compare_range(const _Tp *__lhs, const _Tp *__rhs, size_t __n) noexcept {
    if consteval {
        while(__n--) if (auto __cmp = *__lhs++ <=> *__rhs++ ; __cmp != 0) return __cmp;
        return std::strong_ordering::equivalent;
    } else {
        return std::memcmp(__lhs, __rhs, __n * sizeof(_Tp)) <=> 0;
    }
}

/**
 * @brief A constexpr version of std::strlen.
 * @param __dst Destination pointer.
 * @param __src Source pointer.
 * @return Destination pointer.
 */
[[__gnu__::__always_inline__, nodiscard]]
constexpr size_t strlen(const char *__str) noexcept {
    if consteval {
        size_t __len = 0;
        while(*__str++) ++__len;
        return __len;
    } else {
        return std::strlen(__str);
    }
}


/**
 * @brief A constexpr version of std::strcpy.
 * @param __dst Destination pointer.
 * @param __src Source pointer.
 * @return Destination pointer.
 */
[[__gnu__::__always_inline__, nodiscard]]
constexpr char *strcpy(char *__dst,const char *__src) noexcept {
    if consteval {
        char *__bak = __dst;
        while((*__dst++ = *__src++));
        return __bak;
    } else {
        return std::strcpy(__dst, __src);
    }
}

/**
 * @brief A constexpr version of std::strcmp.
 * @param __lhs Left hand side string.
 * @param __rhs Right hand side string.
 * @return std::strong_ordering the result of comparison.
 */
constexpr std::strong_ordering
    strcmp(const char *__lhs, const char *__rhs) {
    if consteval {
        while(*__lhs) if (auto __cmp = *__lhs++ <=> *__rhs++ ; __cmp != 0) return __cmp;
        return 0 <=> *__rhs; // _*__lhs must be 0 now.
    } else {
        return std::strcmp(__lhs, __rhs) <=> 0;
    }
}


/**
 * @brief A constexpr version of std::strncmp.
 * @param __lhs Left hand side string.
 * @param __rhs Right hand side string.
 * @param __n Count of characters to compare.
 * @return std::strong_ordering the result of comparison.
 */
constexpr std::strong_ordering
    strncmp(const char *__lhs, const char *__rhs, size_t __n) {
    if consteval {
        while(__n--) {
            if (*__lhs == 0) return 0 <=> *__rhs; 
            if (auto __cmp = *__lhs++ <=> *__rhs++ ; __cmp != 0) return __cmp;
        } return std::strong_ordering::equivalent;
    } else {
        return std::strncmp(__lhs, __rhs, __n) <=> 0;
    }
}

} // namespace dark
