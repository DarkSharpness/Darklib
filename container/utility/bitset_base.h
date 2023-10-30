#pragma once

#include "common.h"

#include <bit>
#include <cstddef>
#include <climits>
#include <cstring>


namespace dark {

/* Localization of the functions. */
#define __BITSET_LOCAL__ inline __attribute__((always_inline)) static constexpr

/**
 * @brief Memcpy or memmove according to the memory overlapping.
 * Not compile time friendly!
 */
template <bool _May_Overlap>
__BITSET_LOCAL__
void memcpy_or_move(void *__ptr,const void * __src, size_t __n)
noexcept {
    if constexpr (_May_Overlap) std::memmove(__ptr, __src, __n);
    else                        std::memcpy (__ptr, __src, __n);
}


/**
 * @brief Compile time friendly integer operations
 * base on builtin functions.
 */
struct integer_base {
    /**
     * @return Number of bits of the integral type.
     */
    template <typename _Tp>
    requires std::integral <_Tp>
    static consteval size_t bit_width() noexcept { return CHAR_BIT * sizeof(_Tp); }

    template <typename _Tp>
    requires std::same_as <_Tp, std::byte>
    static constexpr size_t bit_width() noexcept { return CHAR_BIT; }

    /**
     * @brief Compile time friendly memset.
     * Set the data in range [__ptr, __ptr + __n) to __val (by byte).
     */
    template <typename _Tp>
    requires std::integral <_Tp> && std::same_as <_Tp, std::decay_t <_Tp>>
    static constexpr void memset(_Tp *__ptr, int __val, size_t __n) noexcept {
        if consteval {
            _Tp __dat {};
            for (size_t i = 0 ; i != sizeof(_Tp) ; ++i)
                __dat = (__dat << CHAR_BIT) | __val;
            while(__n--) *(__ptr++) = __dat;
        } else {
            std::memset(__ptr, __val, __n * sizeof(_Tp));
        }
    }

    /**
     * @brief Compile time friendly memcpy.
     * Copy the data in range [__src, __src + __n) to [__ptr, __ptr + __n).
     * @note No memory overlapping is allowed!
     */
    template <typename _Tp>
    requires std::integral <_Tp> && std::same_as <_Tp, std::decay_t <_Tp>>
    static constexpr void memcpy(_Tp *__ptr,const _Tp * __src, size_t __n) noexcept {
        if consteval {
            const _Tp *__end = __src + __n;
            while(__src != __end) *(__ptr++) = *(__src++);
        } else {
            std::memcpy(__ptr, __src, __n * sizeof(_Tp));
        }
    }

    /**
     * @brief Compile time friendly memmove.
     * Copy the data in range [__src, __src + __n) to [__ptr, __ptr + __n).
     * @note Memory overlapping is allowed!
     */
    template <typename _Tp>
    requires std::integral <_Tp> && std::same_as <_Tp, std::decay_t <_Tp>>
    static constexpr void memmove(_Tp *__ptr,const _Tp * __src, size_t __n) noexcept {
        if consteval {
            const _Tp *__end = __src + __n;
            if (__ptr < __src || __ptr >= __end)
                while(__src != __end) *(__ptr++) = *(__src++);
            else if (__ptr > __src) {
                __ptr += __n;
                while(__end != __src) *(--__ptr) = *(--__end);
            }
        } else {
            std::memmove(__ptr, __src, __n * sizeof(_Tp));
        }
    }

    /**
     * @brief Compile time friendly memcmp.
     * Compare the data in range [__src, __src + __n) with [__dst, __dst + __n).
     * @return 0 if equal, positive if __src > __dst, negative if __src < __dst.
     */
    template <typename _Tp>
    requires std::integral <_Tp> && std::same_as <_Tp, std::decay_t <_Tp>>
    static constexpr int memcmp(const _Tp *__src, const _Tp *__dst, size_t __n) noexcept {
        if consteval {
            while(__n--) {
                _Tp __lhs = *(__src++);
                _Tp __rhs = *(__dst++);
                if (__lhs == __rhs) continue;

                constexpr size_t __width = bit_width <unsigned char> ();
                constexpr unsigned char __mask = (1 << __width) - 1;

                if (is_little_endian()) {
                    for (size_t i = 0 ; i != sizeof(_Tp) ; ++i) {
                        unsigned char __l = (__lhs >> (i * __width)) & __mask;
                        unsigned char __r = (__rhs >> (i * __width)) & __mask;
                        if (__l != __r) return __l - __r;
                    }
                } else {
                    size_t i = sizeof(_Tp);
                    while(i --> 0) {
                        unsigned char __l = (__lhs >> (i * __width)) & __mask;
                        unsigned char __r = (__rhs >> (i * __width)) & __mask;
                        if (__l != __r) return __l - __r;
                    }
                }
            }
            return 0;
        } else {
            return std::memcmp(__src, __dst, __n * sizeof(_Tp));
        }
    }
};


/**
 * @brief This is the base class of bitset.
 * 
 * @note Only works on little Endian machine.
 */
template <typename _Tp>
requires std::unsigned_integral <_Tp> && (is_little_endian())
struct bitset_base {
    template <typename _Up>
    requires requires { integer_base::bit_width <_Up> (); }
    __BITSET_LOCAL__ size_t bit_width() noexcept { return integer_base::bit_width <_Up> (); }

    /* The bit width of _Tp. */
    inline static constexpr size_t default_width = integer_base::bit_width <_Tp> ();

    /* Return the array length for given bits. */
    __BITSET_LOCAL__ size_t array_length(size_t __n) noexcept {
        return (__n + (default_width - 1)) / default_width;
    }

    /* Return the waste length for given bits. */
    __BITSET_LOCAL__ size_t waste_length(size_t __n) noexcept {
        return array_length(__n) * default_width - __n;
    }

  protected:

    __BITSET_LOCAL__ void dummy_lshift(_Tp *__ptr,const _Tp *__src,size_t __offset,size_t __len)
    noexcept {
        const size_t __wshift = __offset / default_width;
        const size_t __bshift = __offset % default_width;
        const size_t __rshift = default_width - __bshift;

        for(size_t __n = __len - 1 ; __n > __wshift ; --__n)
            __ptr[__n] = _Tp(__src[__n - __wshift]     << __bshift)
                       | _Tp(__src[__n - __wshift - 1] >> __rshift);
        __ptr[__wshift] = __src[0] << __bshift;

        integer_base::memset(__ptr, 0, __wshift);
    }

    __BITSET_LOCAL__ void dummy_rshift(_Tp *__ptr,const _Tp *__src,size_t __offset,size_t __len)
    noexcept {
        const size_t __wshift = __offset / default_width;
        const size_t __bshift = __offset % default_width;
        const size_t __rshift = default_width - __bshift;
        const size_t __limits = __len - __wshift - 1;

        for(size_t __n = 0 ; __n < __limits ; ++__n)
            __ptr[__n] = _Tp(__src[__n + __wshift]     >> __bshift)
                       | _Tp(__src[__n + __wshift + 1] << __rshift);
        __ptr[__limits] = __src[__len - 1] >> __bshift;

        integer_base::memset(__ptr + __limits + 1, 0, __wshift);
    }

    template <bool _May_Overlap>
    __BITSET_LOCAL__
    void const_lshift(_Tp *__ptr,const _Tp *__src,size_t __offset,size_t __len)
    noexcept {
        if (__offset % default_width != 0) { /* Constexpr expression. */
            return dummy_lshift(__ptr , __src , __offset , __len);
        } else {
            const size_t __shift = __offset / default_width;
            const size_t __count = __len - __shift;
            if (_May_Overlap) integer_base::memmove(__ptr + __shift , __src , __count);
            else              integer_base::memcpy (__ptr + __shift , __src , __count);
            return integer_base::memset(__ptr , 0 , __shift);
        }
    }

    template <bool _May_Overlap>
    __BITSET_LOCAL__
    void const_rshift(_Tp *__ptr,const _Tp *__src,size_t __offset,size_t __len)
    noexcept {
        if (__offset % default_width != 0) { /* Constexpr expression. */
            return dummy_rshift(__ptr , __src , __offset , __len);
        } else {
            const size_t __shift = __offset / default_width;
            const size_t __count = __len - __shift;
            if (_May_Overlap) integer_base::memmove(__ptr , __src + __shift , __count);
            else              integer_base::memcpy (__ptr , __src + __shift , __count);
            return integer_base::memset(__ptr + __count , 0 , __shift);
        }
    }

  public:

    /**
     * @brief Compile time friendly left shift.
     * @param __ptr Pointer to the first element of the array.
     * @param __offset Offset to shift the array (by bit).
     * @param __len Length of the bitset array.
     * @note Ensure that the offset will not exceed the length of the bitset.
     */
    template <bool _May_Overlap>
    __BITSET_LOCAL__
    void do_lshift(_Tp *__ptr,const _Tp *__rhs,size_t __offset,size_t __len)
    noexcept {
        if consteval { /* Constexpr expression. */
            return const_lshift <_May_Overlap> (__ptr , __rhs , __offset , __len);
        }

        if (__offset % bit_width <std::byte> () != 0) {
            return dummy_lshift(__ptr , __rhs , __offset , __len);
        } else { /* Reduce to byte-wise first. */
            const size_t __shift = __offset / bit_width <std::byte> ();
            const size_t __count = __len * sizeof(_Tp) - __shift;

            std::byte const *__src = (std::byte const *)(__rhs);
            std::byte       *__dst = (std::byte *)(__ptr) + __shift;
            std::byte       *__pos = (std::byte *)(__ptr);

            memcpy_or_move <_May_Overlap> (__dst, __src , __count);
            std::memset (__pos , 0 , __shift);
        }
    }

    /**
     * @brief Compile-time friendly right shift.
     * @param __ptr Pointer to the first element of the array.
     * @param __offset Offset to shift the array (by bit).
     * @param __len Length of the bitset array.
     * @note Ensure that the offset will not exceed the length of the bitset.
     */
    template <bool _May_Overlap>
    __BITSET_LOCAL__
    void do_rshift(_Tp *__ptr,const _Tp *__rhs,size_t __offset,size_t __len)
    noexcept {
        if consteval { /* Constexpr expression. */
            return const_rshift <_May_Overlap> (__ptr, __rhs, __offset, __len);
        }

        if (__offset % bit_width <std::byte> () != 0) {
            return dummy_rshift(__ptr, __rhs, __offset, __len);
        } else { /* Reduce to byte-wise first. */
            const size_t __shift = __offset / bit_width <std::byte> ();
            const size_t __count = __len * sizeof(_Tp) - __shift;

            std::byte const *__src = (std::byte const*)(__rhs) + __shift;
            std::byte       *__dst = (std::byte *)(__ptr);
            std::byte       *__pos = (std::byte *)(__ptr) + __count;

            memcpy_or_move <_May_Overlap> (__dst, __src , __count);
            std::memset (__pos , 0 , __shift);
        }
    }

    __BITSET_LOCAL__ void do_not(_Tp *__ptr, const _Tp *__src, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) __ptr[__n] = ~__src[__n];
    }

    __BITSET_LOCAL__ void do_and(_Tp *__ptr, const _Tp *__lhs, const _Tp *__rhs, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) __ptr[__n] = __lhs[__n] & __rhs[__n];
    }

    __BITSET_LOCAL__ void do_or (_Tp *__ptr, const _Tp *__lhs, const _Tp *__rhs, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) __ptr[__n] = __lhs[__n] | __rhs[__n];
    }

    __BITSET_LOCAL__ void do_xor(_Tp *__ptr, const _Tp *__lhs, const _Tp *__rhs, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) __ptr[__n] = __lhs[__n] ^ __rhs[__n];
    }

    __BITSET_LOCAL__ void do_set(_Tp *__ptr, size_t __len)
    noexcept {
        return integer_base::memset(__ptr, -1, __len);
    }

    __BITSET_LOCAL__ void do_reset(_Tp *__ptr, size_t __len)
    noexcept {
        return integer_base::memset(__ptr, 0 , __len);
    }

    __BITSET_LOCAL__ bool test_any(const _Tp *__ptr, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) if (__ptr[__n] == 0) return true;
        return false;
    }

    __BITSET_LOCAL__ bool test_all(const _Tp *__ptr, size_t __len)
    noexcept {
        for (size_t __n = 0 ; __n != __len ; ++__n) if (__ptr[__n] != ~_Tp(0)) return false;
        return true;
    }

    __BITSET_LOCAL__ size_t do_count(const _Tp *__ptr, size_t __len)
    noexcept {
        size_t __cnt = 0;
        for (size_t __n = 0 ; __n != __len ; ++__n)
            __cnt += std::popcount(__ptr[__n]);
        return __cnt;
    }

    __BITSET_LOCAL__ bool test_equal(const _Tp *__lhs, const _Tp *__rhs, size_t __len)
    noexcept {
        return integer_base::memcmp(__lhs, __rhs, __len) == 0;
    }

    __BITSET_LOCAL__ char *do_to_string(const _Tp *__ptr,char *__buf,size_t __pos)
    noexcept {
        return nullptr; // Not implemented.
    }

    __BITSET_LOCAL__ size_t do_find_first(const _Tp *__ptr,size_t __tail)
    noexcept {
        const size_t __top = array_length(__tail);
        for(size_t __n = 0 ; __n != __top ; ++__n) {
            _Tp __this = __ptr[__n];
            if (__this) return __n * default_width + std::countr_zero(__this);
        } return __tail;
    }

    __BITSET_LOCAL__ size_t do_find_next(const _Tp *__ptr,size_t __pos,size_t __tail)
    noexcept {
        if (++__pos >= __tail) return __tail;
        size_t __wshift = __pos / default_width;
        size_t __bshift = __pos % default_width;
        /* Masking the low bits first. */
        _Tp __this = __ptr[__wshift] & (_Tp(~_Tp(0)) << __bshift);
        if (__this) return __wshift * default_width + std::countr_zero(__this);

        const size_t __top = array_length(__tail);
        while(++__wshift < __top) {
            __this = __ptr[__wshift];
            if (__this) return __wshift * default_width + std::countr_zero(__this);
        } return __tail;
    }

    struct reference {
        _Tp   *data; /* Real data  */
        size_t bias; /* Bit offset. */

        constexpr reference() noexcept = default;
        constexpr reference(_Tp *__data, size_t __offset) noexcept {
            data = __data + __offset / default_width;
            bias = __offset % default_width;
        }
        constexpr reference(const reference &) noexcept = default;
        constexpr ~reference() noexcept = default;

        constexpr reference &operator = (bool __val) noexcept {
            if (__val) *data |=  (_Tp(1) << bias);
            else       *data &= ~(_Tp(1) << bias); 
            return *this;
        }

        constexpr operator bool() const noexcept {
            return _Tp(*data >> bias) & _Tp(1); 
        }

        constexpr reference &flip() noexcept {
            *data ^= (_Tp(1) << bias);
            return *this;
        }

        constexpr bool operator~() const noexcept {
            return !bool(*this);
        }

        /* b[i] = b[j] case. */
        constexpr reference &operator=(const reference &rhs) noexcept {
            if (rhs) *data |=  (_Tp(1) << bias);
            else     *data &= ~(_Tp(1) << bias);
            return *this;
        }
    };

};

#undef __BITSET_LOCAL__


}

