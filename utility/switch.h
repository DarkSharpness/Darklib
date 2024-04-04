#pragma once
#include "basic.h"
#include <ranges>
#include <string_view>

namespace dark {

namespace __detail::__hash {

/* A simple unique_ptr style guard. */
template <typename _Tp>
struct memory_manger {
    _Tp *data;
    constexpr memory_manger(_Tp *__d) : data(__d) {}
    constexpr ~memory_manger() { delete[] data; }
};

/* Hash function for string_view */
inline constexpr size_t
string_hash(std::string_view __str, size_t __h) {
    size_t __hash = 0;
    for (auto __c : __str) __hash = __hash * __h + __c;
    return __hash;
}

/* Check if there is a conflict in the hash */
inline consteval bool
with_conflict(size_t *__vec, size_t __size) {
    for (size_t i = 0 ; i < __size; i++)
        for (size_t j = i + 1; j < __size; j++)
            if (__vec[i] == __vec[j]) return true;
    return false;
}

/* Find a suitable hash */
inline consteval size_t
pick_hash(const std::string_view *__vec, size_t __size) {
    constexpr size_t __table[] = {
        31, 131, 137, 257, 313, 1109,
        11113, 116731, 13109983, 17961079,  
    };
    size_t *__hash = new size_t[__size];
    memory_manger __guard(__hash); // Guard the memory.
    for (auto __h : __table) {
        for (size_t i = 0; i < __size; ++i)
            __hash[i] = string_hash(__vec[i], __h);
        if (!with_conflict(__hash, __size))
            return __h; // A non-conflict hash.
    }

    panic("No suitable hash found. What did you input?");
}


} // namespace __detail::__hash

/**
 * @brief An constexpr string switch helper.
 * Using its operator () to get the hash code of a string.
 */
struct switch_string {
  protected:
    const size_t code; /* Real hash code. */
  public:
    explicit consteval switch_string(size_t __c) : code(__c) {}
    constexpr size_t operator()(std::string_view __str) const {
        return __detail::__hash::string_hash(__str, code);
    }
};

/**
 * @brief Make a switch hash helper for string.
 * @return An constexpr hash_wrapper object. 
 */
template <typename ..._Str>
requires (std::convertible_to<_Str, std::string_view> && ...)
consteval auto make_switch(const _Str& ...__str) {
    const std::string_view __strs[] = {__str...};
    return switch_string{__detail::__hash::pick_hash(__strs, sizeof...(_Str))};
}

/**
 * @brief Make a switch hash helper for string.
 * @return An constexpr hash_wrapper object. 
 */
template <std::ranges::range _Range>
consteval auto make_switch(const _Range &__r) {
    const auto __size = std::ranges::distance(__r);
    const auto __strs = new std::string_view[__size];

    __detail::__hash::memory_manger __guard(__strs); // Guard the memory.

    // Copy the string to the pointer.
    auto __beg = std::ranges::begin(__r);
    auto __end = std::ranges::end(__r);
    auto __ptr = __strs;
    while (__beg != __end) *(__ptr++) = *(__beg++);

    return switch_string{__detail::__hash::pick_hash(__strs, __size)};
}


} // namespace dark
