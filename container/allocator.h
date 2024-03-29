/* A simple allocator. */
#pragma once
#include "basic.h"
#include <cstdlib>
#include <bits/allocator.h>
#include <type_traits>

namespace dark {

template <class _Tp>
struct allocator {
    inline static constexpr size_t __N = sizeof(_Tp);

    template <class U>
    struct rebind { using other = allocator<U>; };

    using size_type         = size_t;
    using difference_type   = ptrdiff_t;
    using value_type        = _Tp;
    using pointer           = _Tp *;
    using reference         = _Tp &;
    using const_pointer     = const _Tp *;
    using const_reference   = const _Tp &;

    [[nodiscard,__gnu__::__always_inline__]]
    constexpr static _Tp *allocate(size_t __n) {
        if (std::is_constant_evaluated()) {
            return std::allocator <_Tp> {}.allocate(__n);
        } else {
            return static_cast <_Tp *> (::std::malloc(__n * __N));
        }
    }

    [[nodiscard,__gnu__::__always_inline__]]
    constexpr static _Tp *zeallocate(size_t __n) {
        static_assert(std::is_integral_v <_Tp>,
            "Only integral types are allowed in calloc now.");
        if (std::is_constant_evaluated()) {
            auto *__raw = std::allocator <_Tp> {}.allocate(__n);
            for (size_t i = 0; i < __n; ++i) __raw[i] = 0;
            return __raw;
        } else {
            return static_cast <_Tp *> (::std::calloc(__n, __N));
        }
    }

    [[nodiscard,__gnu__::__always_inline__]]
    constexpr static _Tp *reallocate(_Tp *__ptr, size_t __n) {
        static_assert(std::is_trivial_v <_Tp>,
            "Only trivial types are allowed in realloc now.");
        if (std::is_constant_evaluated()) {
            return std::allocator <_Tp> {}.reallocate(__ptr,__n);
        } else {
            return static_cast <_Tp *> (::std::realloc(__ptr,__n * __N));
        }
    }

    [[__gnu__::__always_inline__]]
    constexpr static void deallocate(_Tp *__ptr,[[maybe_unused]] size_t __n)
    noexcept {
        if (std::is_constant_evaluated()) {
            if (__ptr != nullptr)
                return std::allocator <_Tp> {}.deallocate(__ptr,__n);
        } else {
            return ::std::free(__ptr);
        }
    }
};

} // namespace dark
