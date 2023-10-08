#pragma once

#include "utility/bitset_base.h"

#include <array>
#include <cstdint>
#include <initializer_list>

namespace dark {

/**
 * @brief A fast and simple fix-sized bitset.
 * @tparam _Nm The number of bits in the bitset.
 * @tparam _Word The word type used to store the bits.
 * @note It behaves like a normal array of bools, just as std::bitset.
 * However, this bitset utilizes the feature of little endian.
 * It will call memcpy/memmove/memset/memcmp to do some bit operations.
 * 
 * In addition, this bitset features uninitialized constructior.
 * Use nullptr to construct an uninitialized bitset.
 * 
 * 
 */
template <size_t _Nm, typename _Word = std::uint64_t>
requires std::unsigned_integral <_Word> && (is_little_endian())
struct bitset {
  protected:
    using _Base     = bitset_base <_Word>;
    using reference = typename _Base::reference;

    inline constexpr static size_t _Len = _Base::array_length(_Nm);
    inline constexpr static size_t _Rem = _Base::waste_length(_Nm);

    _Word word[_Len];

    /* Mask those useless bits to 0. */
    constexpr void do_mask_high_bits() {
        if constexpr (_Rem != 0) word[_Len - 1] &= _Word(~_Word(0)) >> _Rem;
    }

    /* A completely uninitialized bitset. */
    explicit constexpr bitset(std::nullptr_t,std::nullptr_t) noexcept {}

    constexpr void do_lshift(size_t __n) {
        return _Base::template do_lshift <true> (word,word,__n,_Len);
    }

    constexpr void do_rshift(size_t __n) {
        return _Base::template do_rshift <true> (word,word,__n,_Len);
    }

    constexpr void do_lshift(const bitset &__rhs,size_t __n) {
       return _Base::template do_lshift <false> (word,__rhs.word,__n,_Len);
    }

    constexpr void do_rshift(const bitset &__rhs,size_t __n) {
       return _Base::template do_rshift <false> (word,__rhs.word,__n,_Len);
    }

  public:
    /* Basic member functions. */

    /* Normal initialization filling with 0. */
    constexpr bitset() noexcept : word() {}

    /* An uninitialized bitset except that the high bits are set 0. */
    constexpr bitset(std::nullptr_t) noexcept {
        if constexpr (_Rem != 0) word[_Len - 1] = 0;
    }

    /**
     * @brief Use initializer_list to initialize the bitset.
     * @param __arr The initializer_list. 
     */
    template <typename _Up>
    requires std::integral <_Up>
    bitset(std::initializer_list <_Up> __arr) noexcept {
        const size_t __min = std::min(__arr.size() * sizeof(_Up),_Len * sizeof(_Word));
        std::memcpy(word,__arr.begin(),__min);
        this->do_mask_high_bits();
    }

    constexpr bitset(const bitset &) noexcept = default;
    constexpr bitset &operator = (const bitset &) noexcept = default;
    constexpr bitset &operator = (std::nullptr_t) = delete;

    constexpr ~bitset() noexcept = default;

    template <typename _Up>
    requires std::unsigned_integral <_Up> && (sizeof(_Up) <= sizeof(_Word))
    constexpr bitset(_Up __val) noexcept : word({__val}) {
        if constexpr (integer_base::bit_width <_Up> () > _Nm) {
            this->do_mask_high_bits();
        }
    }

    template <typename _Up>
    requires std::signed_integral <_Up>
    constexpr bitset(_Up __val) noexcept : bitset(static_cast <_Word> (__val)) {}

    template <typename _Up>
    requires std::unsigned_integral <_Up> && (sizeof(_Up) > sizeof(_Word))
    constexpr bitset(_Up __val) noexcept : word() {
        for (size_t __i = 0 ; __i < _Len ; __i++) {
            word[__i] = __val;
            __val >>= _Base::default_width;
        }
    }

  public:
    /* Bitset functions. */

    static consteval size_t size() noexcept { return _Nm; }

    constexpr bool all() const noexcept { return _Base::test_all(word,_Len); }

    constexpr bool any() const noexcept { return _Base::test_any(word,_Len); }

    constexpr bool none() const noexcept { return !_Base::test_any(word,_Len); }

    constexpr size_t count() const noexcept { return _Base::do_count(word,_Len); }

    constexpr bitset& set() noexcept {
        _Base::do_set(word,_Len);
        this->do_mask_high_bits();
        return *this;
    }

    constexpr bitset& flip() noexcept {
        _Base::do_not (word,word,_Len);
        this->do_mask_high_bits();
        return *this;
    }

    constexpr bitset& reset() noexcept {
        _Base::do_reset(word,_Len);
        // this->do_mask_high_bits();  // No need to mask high bits.
        return *this;
    }

    constexpr bitset operator << (size_t __n) const noexcept {
        bitset __ret {nullptr,nullptr}; // Completely uninitialized.
        if (__builtin_expect(__n != 0,1)) {
            if (__builtin_expect(__n < _Nm,1)) {
                __ret.do_lshift(*this,__n);
                __ret.do_mask_high_bits();
            } else {
                _Base::do_reset(__ret.word,_Len);
            }
        } else {
            __ret = *this;
        }
        return __ret;
    }

    constexpr bitset operator >> (size_t __n) const noexcept {
        bitset __ret {nullptr,nullptr}; // Completely uninitialized.
        if (__builtin_expect(__n != 0,1)) {
            if (__builtin_expect(__n < _Nm,1)) {
                __ret.do_mask_high_bits(); // Due to uninitialized.
                __ret.do_rshift(*this,__n);
            } else {
                _Base::do_reset(__ret.word,_Len);
            }
        } else {
            __ret = *this;
        }
        return __ret;
    }

    friend constexpr bitset operator & (const bitset& __lhs, const bitset& __rhs)
    noexcept {
        bitset __ret {nullptr,nullptr};
        _Base::do_and(__ret.word,__lhs.word,__rhs.word,_Len);
        return __ret;
    }

    friend constexpr bitset operator | (const bitset& __lhs, const bitset& __rhs)
    noexcept {
        bitset __ret {nullptr,nullptr};
        _Base::do_or(__ret.word,__lhs.word,__rhs.word,_Len);
        return __ret;
    }

    friend constexpr bitset operator ^ (const bitset& __lhs, const bitset& __rhs)
    noexcept {
        bitset __ret {nullptr,nullptr};
        _Base::do_xor(__ret.word,__lhs.word,__rhs.word,_Len);
        return __ret;
    }

  public:
    /* Bitset operators. */

    constexpr bitset& operator &= (const bitset& __val) noexcept {
        _Base::do_and(word,word,__val.word,_Len);
        return *this;
    }

    constexpr bitset& operator |= (const bitset& __val) noexcept {
        _Base::do_or(word,word,__val.word,_Len);
        return *this;
    }

    constexpr bitset& operator ^= (const bitset& __val) noexcept {
        _Base::do_xor(word,word,__val.word,_Len);
        return *this;
    }

    constexpr bitset& operator <<= (size_t __pos) noexcept {
        if (__builtin_expect(__pos != 0,1)) {
            if (__builtin_expect(__pos < _Nm,1)) {
                this->do_lshift(__pos);
                this->do_mask_high_bits();
            } else {
                _Base::do_reset(word,_Len);
            }
        }
        return *this;
    }

    constexpr bitset& operator >>= (size_t __pos) noexcept {
        if (__builtin_expect(__pos != 0,1)) {
            if (__builtin_expect(__pos < _Nm,1)) {
                this->do_rshift(__pos);
            } else {
                _Base::do_reset(word,_Len);
            }
        }
        return *this;
    }

    constexpr bitset operator ~ () const noexcept {
        bitset __ret;
        _Base::do_not(__ret.word,word,_Len);
        return __ret;
    }

    constexpr bool operator == (const bitset& __val) const noexcept {
        return _Base::test_equal(word,__val.word,_Len);
    }

    char *to_string(char * __buf) const noexcept {
        for(size_t i = 0 ; i < _Nm ; i++)
            __buf[i] = this->operator[](i) + '0';
        return std::addressof(__buf[_Nm] = 0);
    }

    /* Return the inner data array. */
    _Word *data()             noexcept { return word; }
    /* Return the inner data array. */
    const _Word *data() const noexcept { return word; }

  public:
    /* Bitwise operators and functions. */

    constexpr reference operator[](size_t __pos) noexcept {
        return reference { word , __pos };
    }

    constexpr bool operator[](size_t __pos) const noexcept {
        const size_t __wshift = __pos / _Base::default_width;
        const size_t __bshift = __pos % _Base::default_width;
        return (word[__wshift] >> __bshift) & _Word(1);
    }

    constexpr bool test(size_t __pos) const noexcept { return this->operator[](__pos); }

    constexpr bitset& set(size_t __pos,bool __val = true) noexcept {
        this->operator[](__pos) = __val;
        return *this;
    }

    constexpr bitset& reset(size_t __pos) noexcept {
        this->operator[](__pos) = false;
        return *this;
    }

    constexpr bitset& flip(size_t __pos) noexcept {
        this->operator[](__pos).flip();
        return *this;
    }

  public:
    /* Range related function. */

    size_t find_first() const noexcept {
        return _Base::do_find_first(word,_Nm);
    }

    size_t find_next(size_t __pos) const noexcept {
        return _Base::do_find_next(word,__pos,_Nm);
    }


    constexpr bitset& set_range(size_t __beg,size_t __end,bool __val = true) noexcept {
        if (__val) _Base::do_set_bits  (word,__beg,__end);
        else       _Base::do_reset_bits(word,__beg,__end);
        return *this;
    }

    constexpr bitset& reset_range(size_t __beg,size_t __end) noexcept {
        return this->set_range(__beg,__end,false);
    }

    constexpr bitset& flip_range(size_t __beg,size_t __end) noexcept {
        _Base::do_flip_bits(word,__beg,__end);
        return *this;
    }

};


}
