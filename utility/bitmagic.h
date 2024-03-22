#pragma once
#include <bit>
#include <concepts>

namespace dark {

/* Return the lowest bit of a number. */
template <std::unsigned_integral _Tp>
inline constexpr _Tp low_bit(_Tp x) { return x & -x; }

/* Return the highest bit of a number. */
template <std::unsigned_integral _Tp>
inline constexpr _Tp top_bit(_Tp x) { return std::bit_floor(x); }

/* Return the log2 result of an unsigned integer. */
template <std::unsigned_integral _Tp>
inline constexpr auto log2(_Tp x) { return std::bit_width(x) - 1; }

/* Test whether a number is a pow of 2. */
template <std::unsigned_integral _Tp>
inline constexpr bool is_pow2(_Tp x) { return (x - 1) < ((x - 1) ^ x); }


} // namespace dark
