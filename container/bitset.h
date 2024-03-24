#pragma once
#include <bit>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <stdexcept>
#include "allocator.h"

namespace dark {


struct dynamic_bitset;


namespace __detail::__bitset {

/* Word used in bitset. */
using _Word_t = std::size_t;

/* Word bits. */
inline constexpr std::size_t __WBits = sizeof(_Word_t) * CHAR_BIT;

/* Set __n-th bit to 1.  */
inline constexpr _Word_t
mask_pos(std::size_t __n) { return _Word_t{1} << __n; }

/* Set first n low bits to 1, others to 0. */
inline constexpr _Word_t
mask_low(std::size_t __n) { return (_Word_t{1} << __n) - 1; }

/* Set first n low bits to 0, others to 1. */
inline constexpr _Word_t
mask_top(std::size_t __n) { return (~_Word_t{0}) << __n; }

/* Allocate a sequence of zero memory. */
inline constexpr _Word_t *
alloc_zero(std::size_t __n) { return allocator<_Word_t>::calloc(__n); }

/* Allocate a sequence of raw memory. */
inline constexpr _Word_t *
alloc_none(std::size_t __n) { return allocator<_Word_t>::allocate(__n); }

/* Deallocate memory. */
inline constexpr void deallocate(_Word_t *__ptr, std::size_t __n)
{ allocator<_Word_t>::deallocate(__ptr, __n); }

/* Copy __n words from __src to __dst (memcpy/memmove). */
template <bool _Move = false>
inline constexpr void
word_copy(_Word_t *__dst, const _Word_t *__src, std::size_t __n) {
    if (std::is_constant_evaluated()) {
        if (__src == __dst) return; // No need to copy.
        if (__dst < __src + __n && __src < __dst) {
            // Overlapping, copy from the end.
            __dst += __n; __src += __n;
            for (std::size_t i = 0 ; i != __n ; ++i)
                *--__dst = *--__src;
        } else {
            for (std::size_t i = 0 ; i != __n ; ++i)
                __dst[i] = __src[i];
        }
    } else { // Non-constant evaluated.
        const std::size_t __size = __n * sizeof(_Word_t);
        if constexpr (_Move) {
            std::memmove(__dst, __src, __size);
        } else {
            std::memcpy(__dst, __src, __size);
        }
    }
}

/* Move __n words from __src to __dst using memmove. */
inline constexpr void
word_move(_Word_t *__dst, const _Word_t *__src, std::size_t __n) {
    if (std::is_constant_evaluated()) {
        if (__dst <= __src && __dst + __n > __src)
            for (std::size_t i = __n - 1 ; i >= 0 ; --i)
                __dst[i] = __src[i];
    } else {
        std::memmove(__dst, __src, __n * sizeof(_Word_t));
    }
}

/* Reset __n words to given 0 or 1. */
inline constexpr void
word_reset(_Word_t *__dst, bool __val, std::size_t __n) {
    if (std::is_constant_evaluated()) {
        for (std::size_t i = 0 ; i != __n ; ++i)
            __dst[i] = -_Word_t(__val);
    } else {
        std::memset(__dst, -__val, __n * sizeof(_Word_t));
    }
}


/* Return the quotient and remainder of __n , 64 */
inline constexpr auto div_mod(_Word_t __n) {
    struct {
        _Word_t div;
        _Word_t mod;
    } __ret = {__n / __WBits, __n % __WBits};
    return __ret;
}

/* Return ceiling of __n / 64 */
inline constexpr _Word_t div_ceil(_Word_t __n) {
    return (__n + __WBits - 1) / __WBits;
}

/* Return ceiling of __n / 64 - 1, last available word. */
inline constexpr _Word_t div_down(_Word_t __n) {
    return (__n - 1) / __WBits;
}

/* Return 64 - __n, where __n is less than 64 */
inline constexpr _Word_t rev_bits(_Word_t __n) {
    return ((__WBits - 1) ^ __n) + 1;
}

/* Make the last word valid. */
inline constexpr void validate(_Word_t *__dst, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    if (__mod != 0) __dst[__div] &= mask_low(__mod);
}

/* Custom bit manipulator. */
struct reference {
  private:
    _Word_t *   ptr;        // Pointer to the word
    std::size_t msk;        // Mask word of the bit

    friend class ::dark::dynamic_bitset;

    /* ctor */
    constexpr reference(_Word_t *__ptr, std::size_t __pos)
    noexcept : ptr(__ptr), msk(mask_pos(__pos)) {}

  public:
    /* Convert to bool. */
    constexpr operator bool() const { return *ptr & msk; }

    /* Set current bit to 1. */
    constexpr void set()    { *ptr |= msk; }
    /* Set current bit to 0. */
    constexpr void reset()  { *ptr &= ~msk; }
    /* Flip current bit. */
    constexpr void flip()   { *ptr ^= msk; }

    /* Assign value to current bit. */
    constexpr bool
    operator = (bool val) { val ? set() : reset(); return val; }
    /* Assign value to current bit. */
    constexpr bool
    operator = (const reference &rhs) { return *this = bool(rhs); }
};

/* Custom bit vector. */
struct dynamic_storage {
  private:
    _Word_t *   head;   // Pointer to the first word
    std::size_t buffer; // Buffer size
  protected:
    std::size_t length; // Real length of the bitset

    /* Reallocate memory. */
    constexpr void
    realloc(std::size_t __n) { head = alloc_none(buffer = __n); }

    /* Deallocate memory. */
    constexpr void dealloc() { deallocate(head, buffer); }

    /* Deallocate memory. */
    constexpr static void dealloc(_Word_t *__ptr, std::size_t __n) {
        deallocate(__ptr, __n);
    }

    /* Reset the storage. */
    constexpr void reset() { head = nullptr; buffer = 0; length = 0; }

  public:
    /* ctor & operator section. */

    constexpr ~dynamic_storage()  noexcept { this->dealloc(); }
    constexpr dynamic_storage()   noexcept { this->reset();   }

    constexpr dynamic_storage(std::size_t __n) {
        head = alloc_none(buffer = div_ceil(length = __n));
    }

    constexpr dynamic_storage(std::size_t __n, std::nullptr_t) {
        head = alloc_zero(buffer = div_ceil(length = __n));
    }

    constexpr dynamic_storage(const dynamic_storage &rhs)
        : dynamic_storage(rhs.length) {
        word_copy(head, rhs.head, rhs.word_count());
    }

    constexpr dynamic_storage(dynamic_storage &&rhs) noexcept {
        head   = rhs.head;
        buffer = rhs.buffer;
        length = rhs.length;
        rhs.reset();
    }

    constexpr dynamic_storage &operator = (const dynamic_storage &rhs) {
        if (this == &rhs) return *this;
        if (this->capacity() < rhs.word_count()){
            this->dealloc();
            this->realloc(rhs.length);
            length = rhs.length;
        }
        word_copy(head, rhs.head, rhs.word_count());
        return *this;
    }

    constexpr dynamic_storage &operator = (dynamic_storage &&rhs)
    noexcept { return this->swap(rhs); }

  public:
    /* Function section. */

    /* Return the real word in the bitmap */
    constexpr std::size_t word_count() const { return div_ceil(length); }
    /* Return the capacity of the storage. */
    constexpr std::size_t capacity()   const { return buffer; }

    constexpr dynamic_storage &swap(dynamic_storage &rhs) {
        std::swap(head, rhs.head);
        std::swap(buffer, rhs.buffer);
        std::swap(length, rhs.length);
        return *this;
    }

    constexpr _Word_t *data() const { return head; }
    constexpr _Word_t  data(std::size_t __n) const { return head[__n]; }
    constexpr _Word_t &data(std::size_t __n)       { return head[__n]; }

    /* Grow the size by one, and fill with given value in the back. */
    constexpr void grow_full(bool __val) {
        const auto __size = length / __WBits;
        const auto __capa = this->capacity();
        if (__size == __capa) {
            auto *__temp = head;
            this->realloc(__capa << 1 | !__capa);
            word_copy(head, __temp, __capa);
            this->dealloc(__temp, __capa);
        }
        data(__size) = __val;
    }

    /* Pop one element. */
    constexpr void pop_back() noexcept {
        __detail::__bitset::validate(this->data(), --length);
    }

    /* Clear to empty. */
    constexpr void clear() noexcept { length = 0; }
};

inline constexpr void
do_and(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    for (std::size_t i = 0; i != __div; ++i)
        __dst[i] &= __rhs[i];
    if (__mod != 0)
        __dst[__div] &= __rhs[__div] | mask_top(__mod);
}

inline constexpr void
do_or_(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    for (std::size_t i = 0; i != __div; ++i)
        __dst[i] |= __rhs[i];
    if (__mod != 0)
        __dst[__div] |= __rhs[__div] & mask_low(__mod);
}

inline constexpr void
do_xor(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    for (std::size_t i = 0; i != __div; ++i)
        __dst[i] ^= __rhs[i];
    if (__mod != 0)
        __dst[__div] ^= __rhs[__div] & mask_low(__mod);
}

static_assert(std::endian::native == std::endian::little,
    "Our implement only supports little endian now.");

/* 2-pointer small struct. */
struct vec2 { _Word_t *dst; const _Word_t *src; };

inline static constexpr vec2
operator << (vec2 __vec, std::size_t __n) { return {__vec.dst, __vec.src - __n}; }
inline static constexpr vec2
operator >> (vec2 __vec, std::size_t __n) { return {__vec.dst, __vec.src + __n}; }
inline static constexpr vec2
operator + (vec2 __vec, std::size_t __n) { return {__vec.dst + __n, __vec.src + __n}; }

/* Lshift word by word (with memmove) */
inline constexpr void
word_lshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift == 0) return;
    const auto [__dst, __src] = __vec;
    const auto __offset = __shift / __WBits;
    const auto __count  = div_ceil(__n) - __offset;
    word_copy<true>(__dst + __offset, __src, __count);
    return word_reset(__dst, 0, __offset);
}

/* Rshift word by word. */
inline constexpr void
word_rshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift == 0) return;
    const auto [__dst, __src] = __vec;
    const auto __offset = __shift / __WBits;
    const auto __count  = div_ceil(__n) - __offset;
    return word_copy<true>(__dst, __src + __offset, __count);
}

/* Lshift bits by bits. */
inline constexpr void
bits_lshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    const auto [__div, __mod] = div_mod(__shift);

    const auto __len = div_down(__n);
    const auto __rev = rev_bits(__mod); // 64 - __mod
    const auto __end = __vec.src;

    auto [__dst, __src] = (__vec + __len) << __div;

    /* The last word may be unsafe. */
    auto __pre = div_down(__n - __mod) == __len ? *__src : 0;

    while (__src != __end) {
        auto __cur = *--__src;
        *__dst-- = __pre << __mod | __cur >> __rev;
        __pre = __cur; // Update the previous word.
    }

    /* The first word must be safe. */
    *__dst = __pre << __mod;

    /* Validate those words in the front. */
    return word_reset(__dst - __div, 0, __div);
}

/* Lshift bits by bits. */
inline constexpr void
bits_rshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    const auto [__div, __mod] = div_mod(__shift);

    const auto __len = div_down(__n);
    const auto __rev = rev_bits(__mod); // 64 - __mod
    const auto __end = __vec.dst + __len;

    auto [__dst, __src] = __vec >> __div;
    /* The first word must be safe. */
    auto __pre = *__src;

    while (__dst != __end) {
        auto __cur = *++__src;
        *__dst++ = __pre >> __mod | __cur << __rev;
        __pre = __cur; // Update the previous word.
    }

    /* The last word may be unsafe. */
    auto __cur = div_down(__n + __mod) == __len ? 0 : *++__src;

    /* Rely on the fact that the unused bit of src is filled with 0. */
    *__dst = __pre >> __mod | __cur << __rev;
}

/* Perform left shift operation without any validation. */
inline constexpr void
do_lshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift % __WBits != 0)
        return bits_lshift(__vec, __n, __shift);
    else // __shift % CHAR_BIT == 0. Align to byte.
        return word_lshift(__vec, __n, __shift);
}

/* Perform right shift operation with natural validation. */
inline constexpr void
do_rshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift % __WBits != 0)
        return bits_rshift(__vec, __n, __shift);
    else // __shift % CHAR_BIT == 0. Align to byte.
        return word_rshift(__vec, __n, __shift);
}


} // namespace __detail::__bitset


struct dynamic_bitset : private __detail::__bitset::dynamic_storage {
  public:
    using _Bitset   = dynamic_bitset;
    using reference = __detail::__bitset::reference;

    inline static constexpr std::size_t npos = -1;

  private:
    using _Base_t = __detail::__bitset::dynamic_storage;
    using _Word_t = __detail::__bitset::_Word_t;

    constexpr static _Word_t min(_Word_t __x, _Word_t __y) { return __x < __y ? __x : __y; }
  public:
    /* ctor and operator section. */

    constexpr dynamic_bitset() = default;
    constexpr ~dynamic_bitset() = default;

    constexpr dynamic_bitset(const dynamic_bitset &) = default;
    constexpr dynamic_bitset(dynamic_bitset &&) noexcept = default;

    constexpr dynamic_bitset &operator = (const dynamic_bitset &) = default;
    constexpr dynamic_bitset &operator = (dynamic_bitset &&) noexcept = default;

    constexpr dynamic_bitset(std::size_t __n) : _Base_t(__n, nullptr) {}

    constexpr dynamic_bitset(std::size_t __n, bool __x) : _Base_t(__n) {
        __detail::__bitset::word_reset(this->data(), __x, this->word_count());
        if (__x) __detail::__bitset::validate(this->data(), length);
    }

    constexpr dynamic_bitset(std::string_view __str) : dynamic_bitset(__str.size()) {
        length = __str.size();
        for (std::size_t i = 0 ; i != length ; ++i)
            if (__str[i] == '1') this->set(i);
    }

    constexpr _Bitset &operator |= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_or_(this->data(), __rhs.data(), __min);
        return *this;
    }

    constexpr _Bitset &operator &= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_and(this->data(), __rhs.data(), __min);
        return *this;
    }

    constexpr _Bitset &operator ^= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_xor(this->data(), __rhs.data(), __min);
        return *this;
    }

    constexpr _Bitset &operator <<= (std::size_t __n) {
        if (!length) return this->assign(__n, 0), *this;
        length += __n;

        const auto __size = this->word_count();
        const auto __capa = this->capacity();
        const auto __head = this->data();

        /* This is a nice estimation of next potential size.*/
        /* It will be larger, but that too much bigger.     */
        if (__capa < __size) this->realloc(__size + __capa);

        const auto __data = this->data();
        __detail::__bitset::do_lshift({__data, __head}, length, __n);
        __detail::__bitset::validate(__data, length);

        /* If realloc, deallocate the old memory. */
        if (__head != __data) this->dealloc(__head, __capa);
        return *this;
    }

    constexpr _Bitset &operator >>= (std::size_t __n) {
        if (__n < length) {
            length -= __n;
            const auto __data = this->data();
            __detail::__bitset::do_rshift({__data, __data}, length, __n);
        } else this->clear();
        return *this;
    }

    constexpr _Bitset operator ~() const;

  public:
    /* Section of member functions that won't bring size changes. */

    constexpr _Bitset &set() {
        const auto __size = this->word_count();
        __detail::__bitset::word_reset(this->data(), 1, __size);
        __detail::__bitset::validate(this->data(), length);
        return *this;
    }

    constexpr _Bitset &flip() {
        const auto __size = this->word_count();
        for (std::size_t i = 0 ; i != __size ; ++i)
            this->data(i) = ~this->data(i);
        __detail::__bitset::validate(this->data(), length);
        return *this;
    }

    constexpr _Bitset &reset() {
        const auto __size = this->word_count();
        __detail::__bitset::word_reset(this->data(), 0, __size);
        return *this;
    }

    /* Return whether there is any bit set to 1. */
    constexpr bool any() const { return !this->none(); }
    /* Return whether all bits are set to 1. */
    constexpr bool all() const {
        auto [__div, __mod] = __detail::__bitset::div_mod(length);
        for (std::size_t i = 0 ; i != __div ; ++i)
            if (~data(i) != 0) return false;
        return data(__div) == __detail::__bitset::mask_low(__mod);
    }
    /* Return whether all bits are set to 0. */
    constexpr bool none() const {
        auto __top = this->word_count();
        for (std::size_t i = 0 ; i != __top ; ++i)
            if (data(i) != 0) return false;
        return true;
    }

    /* Return the number of bits set to 1. */
    constexpr std::size_t count() const {
        std::size_t __cnt = 0;
        auto __top = this->word_count();
        for (std::size_t i = 0 ; i != __top ; ++i)
            __cnt += std::popcount(data(i));
        return __cnt;
    }

    constexpr void set(std::size_t __n)       { (*this)[__n].set();     }
    constexpr void reset(std::size_t __n)     { (*this)[__n].reset();   }
    constexpr void flip(std::size_t __n)      { (*this)[__n].flip();    }

    constexpr bool test(std::size_t __n) const {
        auto [__div, __mod] = __detail::__bitset::div_mod(__n);
        return (data(__div) >> __mod) & 1;
    }

    constexpr std::size_t size()  const { return length; }

    constexpr reference operator [] (std::size_t __n) {
        auto [__div, __mod] = __detail::__bitset::div_mod(__n);
        return reference(data() + __div, __mod);
    }
    constexpr reference at(std::size_t __n) { this->range_check(__n); return (*this)[__n]; }
    constexpr bool operator [] (std::size_t __n) const { return test(__n); }
    constexpr bool at(std::size_t __n) const { range_check(__n); return test(__n); }

    constexpr reference front() { return (*this)[0]; }
    constexpr reference back()  { return (*this)[length - 1]; }
    constexpr bool front() const { return test(0); }
    constexpr bool back()  const { return test(length - 1); }

    constexpr std::size_t find_first();
    constexpr std::size_t find_next(std::size_t);

  public:
    /* Section of member functions that may bring size changes. */

    constexpr void push_back(bool __x) {
        using namespace __detail::__bitset;
        if (const auto __mod = length++ % __WBits) {
            data(div_down(length)) |= (_Word_t(__x) << __mod);
        } else { // Full word, so grow the storage by 1.
            this->grow_full(__x);
        }
    }

    constexpr void pop_back() noexcept { return _Base_t::pop_back(); }
    constexpr void clear()    noexcept { return _Base_t::clear();    }

    constexpr void assign(std::size_t __n, bool __x) {
        length = __n;

        const auto __size = this->word_count();
        const auto __capa = this->capacity();

        if (__capa < __size) {
            this->dealloc();
            this->realloc(__size + __capa);
        }

        const auto __data = this->data();
        __detail::__bitset::word_reset(__data, __x, __size);

        // Validate the last word for the last bits.
        if (__x) __detail::__bitset::validate(__data, length);
    }

  public:
    void debug() {
        using namespace __detail::__bitset;
        const auto [__div, __mod] = div_mod(length);
        for (std::size_t i = 0 ; i != __div ; ++i) {
            auto __str = std::bitset <__WBits> (data(i)).to_string();
            std::reverse(__str.begin(), __str.end());
            std::cout << __str << '\n';
        }
        if (__mod != 0) {
            auto __str = std::bitset <__WBits> (data(__div)).to_string();
            std::reverse(__str.begin(), __str.end());
            for (std::size_t i = __mod ; i != __WBits ; ++i)
                if (__str[i] != '0') throw std::runtime_error("Invalid bitset.");
                else __str[i] = '-';
            std::cout << __str << '\n';
        }
    }

    constexpr void range_check(std::size_t __n) const {
        if (__n >= length)
            throw std::out_of_range("dynamic_bitset::range_check");
    }
};


} // namespace dark
