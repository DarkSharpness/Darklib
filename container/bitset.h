#pragma once
#include <bit>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <exception>

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
inline _Word_t *alloc_zero(std::size_t __n) {
    return static_cast <_Word_t *> (std::calloc(__n, sizeof(_Word_t)));
}

/* Allocate a sequence of raw memory. */
inline _Word_t *alloc_none(std::size_t __n) {
    return static_cast <_Word_t *> (std::malloc(__n * sizeof(_Word_t)));
}

/* Copy __n words from __src to __dst. */
inline void word_copy
    (_Word_t *__dst, const _Word_t *__src, std::size_t __n) {
    std::memcpy(__dst, __src, __n * sizeof(_Word_t));
}

/* Return the quotient and remainder of __n , 64 */
inline auto div_mod(_Word_t __n) {
    struct {
        _Word_t div;
        _Word_t mod;
    } __ret = {__n / __WBits, __n % __WBits};
    return __ret;
}

/* Return ceiling of __n / 64 */
inline _Word_t div_ceil(_Word_t __n) {
    return (__n + __WBits - 1) / __WBits;
}

/* Return ceiling of __n / 64 - 1, last available word. */
inline _Word_t div_down(_Word_t __n) {
    return (__n - 1) / __WBits;
}

/* Return 64 - __n, where __n is less than 64 */
inline _Word_t rev_bits(_Word_t __n) {
    return ((__WBits - 1) ^ __n) + 1;
}

/* Make the last word valid. */
inline void validate(_Word_t *__dst, std::size_t __n) {
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
    void realloc(std::size_t __n) { head = alloc_none(__n); buffer = __n; }

    /* Deallocate memory. */
    void dealloc() { std::free(head); }

    /* Deallocate memory. */
    static void dealloc(_Word_t *__ptr) { std::free(__ptr); }

    /* Reset the storage. */
    void reset() { head = nullptr; buffer = 0; length = 0; }

  public:
    /* ctor & operator section. */

    ~dynamic_storage()  noexcept { this->dealloc(); }
    dynamic_storage()   noexcept { this->reset();   }

    dynamic_storage(std::size_t __n) {
        head = alloc_none(__n);
        buffer = length = __n;
    }

    dynamic_storage(std::size_t __n, std::nullptr_t) {
        head = alloc_zero(__n);
        buffer = __n;
        length = div_ceil(__n) * __WBits;
    }

    dynamic_storage(const dynamic_storage &rhs)
        : dynamic_storage(rhs.length) {
        word_copy(head, rhs.head, rhs.word_count());
    }

    dynamic_storage(dynamic_storage &&rhs) noexcept {
        head   = rhs.head;
        buffer = rhs.buffer;
        length = rhs.length;
        rhs.reset();
    }

    dynamic_storage &operator = (const dynamic_storage &rhs) {
        if (this == &rhs) return *this;
        if (this->capacity() < rhs.word_count()){
            this->dealloc();
            this->realloc(rhs.length);
            length = rhs.length;
        }
        word_copy(head, rhs.head, rhs.word_count());
        return *this;
    }

    dynamic_storage &operator = (dynamic_storage &&rhs)
    noexcept { return this->swap(rhs); }

  public:
    /* Function section. */

    /* Return the real word in the bitmap */
    std::size_t word_count() const { return div_ceil(length); }
    /* Return the capacity of the storage. */
    std::size_t capacity()   const { return buffer; }

    dynamic_storage &swap(dynamic_storage &rhs) {
        std::swap(head, rhs.head);
        std::swap(buffer, rhs.buffer);
        std::swap(length, rhs.length);
        return *this;
    }

    _Word_t *data() const { return head; }
    _Word_t  data(std::size_t __n) const { return head[__n]; }
    _Word_t &data(std::size_t __n)       { return head[__n]; }

    /* Grow the size by one, and fill with given value in the back. */
    void grow_full(bool __val) {
        const auto __size = length / __WBits;
        const auto __capa = this->capacity();
        if (__size == __capa) {
            auto *__temp = head;
            this->realloc(__capa << 1 | !__capa);
            word_copy(head, __temp, __capa);
            this->dealloc(__temp);
        }
        data(__size) = __val;
    }

    /* Pop one element. */
    void pop_back() noexcept {
        __detail::__bitset::validate(this->data(), --length);
    }

    /* Clear to empty. */
    void clear() noexcept { length = 0; }
};

inline void do_and(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    for (std::size_t i = 0; i != __div; ++i)
        __dst[i] &= __rhs[i];
    if (__mod != 0)
        __dst[__div] &= __rhs[__div] | mask_top(__mod);
}

inline void do_or_(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
    const auto [__div, __mod] = div_mod(__n);
    for (std::size_t i = 0; i != __div; ++i)
        __dst[i] |= __rhs[i];
    if (__mod != 0)
        __dst[__div] |= __rhs[__div] & mask_low(__mod);
}

inline void do_xor(_Word_t *__dst, const _Word_t *__rhs, std::size_t __n) {
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

inline void byte_lshift(vec2 __vec, std::size_t __n, std::size_t __count) {
    if (__count == 0) return;
    auto [__dst, __src] = __vec;

    const auto __size = sizeof(_Word_t);              // Word size.
    const auto __tail = (__n + __size - 1) / __size;  // Byte last.

    auto *__raw = reinterpret_cast <std::byte *> (__dst);
    std::memmove(__raw + __count, __src, __tail - __count);
    std::memset(__raw, 0, __count);
}

inline void byte_rshift(vec2 __vec, std::size_t __n, std::size_t __count) {
    if (__count == 0) return;
    auto [__dst, __src] = __vec;

    const auto __size = sizeof(_Word_t);              // Word size.
    const auto __tail = (__n + __size - 1) / __size;  // Byte last.

    auto *__raw = reinterpret_cast <const std::byte *> (__src);
    std::memmove(__dst, __raw + __count, __tail);
}

/* Perform left shift operation without validation. */
inline void do_lshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift % CHAR_BIT == 0)
        return byte_lshift(__vec, __n, __shift / CHAR_BIT);

    const auto [__div, __mod] = div_mod(__shift);
    const auto [__dst, __src] = __vec;

    const auto __rev = rev_bits(__mod); // 64 - __mod
    const auto __len = div_down(__n);

    _Word_t __pre = __src[__len - __div];

    for (std::size_t i = __len ; i != __div ; --i) {
        _Word_t __cur = __src[i - 1 - __div];
        __dst[i] = __pre << __mod | __cur >> __rev;
        __pre = __cur; // Update the previous word.
    }

    __dst[__div] = __pre << __mod;
    std::memset(__dst, 0, __div * sizeof(_Word_t));
}

/* Perform right shift operation without validation. */
inline void do_rshift(vec2 __vec, std::size_t __n, std::size_t __shift) {
    if (__shift % CHAR_BIT == 0)
        return byte_rshift(__vec, __n, __shift / CHAR_BIT);

    const auto [__dst, __src] = __vec;
    const auto [__div, __mod] = div_mod(__shift);

    const auto __rev = (__mod ^ (__WBits - 1)) + 1; // _WBits - __mod
    const auto __len = div_down(__n + __shift) - __div;
    _Word_t __pre = __src[__div];

    for (std::size_t i = 0 ; i != __len ; ++i) {
        _Word_t __cur = __src[i + 1 + __div];
        __dst[i] = __pre >> __mod | __cur << __rev;
        __pre = __cur; // Update the previous word.
    }

    __dst[__len] = __pre >> __mod;
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

    static _Word_t min(_Word_t __x, _Word_t __y) { return __x < __y ? __x : __y; }
  public:
    /* ctor and operator section. */

    dynamic_bitset() = default;
    ~dynamic_bitset() = default;

    dynamic_bitset(const dynamic_bitset &) = default;
    dynamic_bitset(dynamic_bitset &&) noexcept = default;

    dynamic_bitset &operator = (const dynamic_bitset &) = default;
    dynamic_bitset &operator = (dynamic_bitset &&) noexcept = default;

    dynamic_bitset(std::size_t __n) : _Base_t(__n, nullptr) {}

    dynamic_bitset(std::size_t __n, bool __x) : _Base_t(__n) {
        std::memset(this->data(), -__x, this->word_count() * sizeof(_Word_t));
        if (__x) __detail::__bitset::validate(this->data(), length);
    }

    _Bitset &operator |= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_or_(this->data(), __rhs.data(), __min);
        return *this;
    }

    _Bitset &operator &= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_and(this->data(), __rhs.data(), __min);
        return *this;
    }

    _Bitset &operator ^= (const _Bitset &__rhs) {
        const auto __min = this->min(length, __rhs.length);
        __detail::__bitset::do_xor(this->data(), __rhs.data(), __min);
        return *this;
    }

    _Bitset &operator <<= (std::size_t __n) {
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
        if (__head != __data) this->dealloc(__head);
        return *this;
    }

    _Bitset &operator >>= (std::size_t __n) {
        if (__n < length) {
            length -= __n;
            const auto __data = this->data();
            __detail::__bitset::do_rshift({__data, __data}, length, __n);
            __detail::__bitset::validate(__data, length);
        } else this->clear();
        return *this;
    }

    _Bitset operator ~() const;

  public:
    /* Section of member functions that won't bring size changes. */

    _Bitset &set() {
        const auto __size = this->word_count();
        std::memset(this->data(), -1, __size * sizeof(_Word_t));
        __detail::__bitset::validate(this->data(), length);
        return *this;
    }

    _Bitset &flip();
    _Bitset &reset();

    /* Return whether there is any bit set to 1. */
    bool any() const { return !this->none(); }
    /* Return whether all bits are set to 1. */
    bool all() const {
        auto [__div, __mod] = __detail::__bitset::div_mod(length);
        for (std::size_t i = 0 ; i != __div ; ++i)
            if (~data(i) != 0) return false;
        return data(__div) == __detail::__bitset::mask_low(__mod);
    }
    /* Return whether all bits are set to 0. */
    bool none() const {
        auto __top = this->word_count();
        for (std::size_t i = 0 ; i != __top ; ++i)
            if (data(i) != 0) return false;
        return true;
    }

    /* Return the number of bits set to 1. */
    std::size_t count() const {
        std::size_t __cnt = 0;
        auto __top = this->word_count();
        for (std::size_t i = 0 ; i != __top ; ++i)
            __cnt += std::popcount(data(i));
        return __cnt;
    }

    void set(std::size_t __n)       { (*this)[__n].set();     }
    void reset(std::size_t __n)     { (*this)[__n].reset();   }
    void flip(std::size_t __n)      { (*this)[__n].flip();    }

    bool test(std::size_t __n) const {
        auto [__div, __mod] = __detail::__bitset::div_mod(__n);
        return (data(__div) >> __mod) & 1;
    }

    std::size_t size()  const { return length; }

    reference operator [] (std::size_t __n) {
        auto [__div, __mod] = __detail::__bitset::div_mod(__n);
        return reference(data() + __div, __mod);
    }
    reference at(std::size_t __n) { this->range_check(__n); return (*this)[__n]; }

    bool operator [] (std::size_t __n) const { return test(__n); }
    bool at(std::size_t __n) const { range_check(__n); return test(__n); }

    reference front() { return (*this)[0]; }
    reference back()  { return (*this)[length - 1]; }

    bool front() const { return test(0); }
    bool back()  const { return test(length - 1); }

    std::size_t find_first();
    std::size_t find_next(std::size_t);

  public:
    /* Section of member functions that may bring size changes. */

    void push_back(bool __x) {
        using namespace __detail::__bitset;
        if (const auto __mod = length++ % __WBits) {
            data(div_down(length)) |= __x << (length % __WBits - 1);
        } else { // Full word, so grow the storage by 1.
            this->grow_full(__x);
        }
    }

    void pop_back() noexcept { return _Base_t::pop_back(); }
    void clear()    noexcept { return _Base_t::clear();    }

    void assign(std::size_t __n, bool __x) {
        length = __n;

        const auto __size = this->word_count();
        const auto __capa = this->capacity();

        if (__capa < __size) {
            this->dealloc();
            this->realloc(__size + __capa);
        }

        const auto __data = this->data();
        std::memset(__data, -__x, __size * sizeof(_Word_t));

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

    void range_check(std::size_t __n) const {
        if (__n >= length)
            throw std::out_of_range("dynamic_bitset::range_check");
    }
};


} // namespace dark
