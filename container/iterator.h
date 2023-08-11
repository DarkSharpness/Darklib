#pragma once

#include <version>
#include <iterator>
#include <concepts>

namespace dark {

template <class traits>
concept advance_iterator = requires (typename traits::node_type *__ptr) {
    traits::advance(__ptr);
};

template <class traits>
concept backtrace_iterator = requires (typename traits::node_type *__ptr) {
    traits::backtrace(__ptr);
};


template <class traits,bool dir>
concept advance_iterator_dir =
    ( dir && advance_iterator   <traits>) 
||  (!dir && backtrace_iterator <traits>);

template <class traits,bool dir>
concept backtrace_iterator_dir =
    ( dir && backtrace_iterator <traits>)
||  (!dir && advance_iterator   <traits>);


template <class traits>
concept random_access_iterator = requires (
    typename traits::node_type *__ptr,
    typename traits::difference_type __n) {
    traits::advance(__ptr,__n);
};


template <class traits>
concept normal_iterator = requires (typename traits::node_type *__ptr) {
    {traits::value_address(__ptr)} -> std::same_as <typename traits::value_type *>;
    typename traits::value_type;
    typename traits::node_type;
    typename traits::difference_type;
    typename traits::iterator_category;
};


template <class traits,bool is_const,bool dir = true>
requires normal_iterator <traits> && advance_iterator_dir <traits,dir>
struct iterator {
    /* Basic 4 requirements. */
    using difference_type   = traits::difference_type;
    using value_type        = traits::value_type;
    using iterator_category = traits::iterator_category; 
    using node_type         = std::conditional_t
        <is_const, const typename traits::node_type, typename traits::node_type>;

    using reference         = std::conditional_t
        <is_const, const value_type &, value_type &>;
    using pointer           = std::conditional_t
        <is_const, const value_type *, value_type *>;

    node_type *data;

    template <bool __dir>
    inline void advance() noexcept {
        if constexpr (__dir) {
            traits::advance(data);
        } else {
            traits::backtrace(data);
        }
    }

    template <bool __dir>
    inline void advance(difference_type __n) noexcept {
        if constexpr (__dir) {
            traits::advance(data,__n);
        } else {
            traits::advance(data,-__n);
        }
    }

  public:

    iterator() noexcept = default;
    iterator(const iterator &) noexcept = default;
    iterator &operator = (const iterator &) noexcept = default;

    explicit iterator(node_type *__data) noexcept : data(__data) {}
    node_type *base() const noexcept { return data; }


    template <void * = nullptr>
    requires advance_iterator_dir <traits,dir>
    iterator &operator ++(void) noexcept { advance <dir> (); return *this; }

    template <void * = nullptr>
    requires advance_iterator_dir <traits,dir>
    iterator operator ++(int) noexcept {
        iterator __tmp = *this; this->operator++(); return __tmp;
    }

    template <void * = nullptr>
    requires backtrace_iterator_dir <traits,dir>
    iterator &operator --(void) noexcept { advance <!dir> (); return *this; }

    template <void * = nullptr>
    requires backtrace_iterator_dir <traits,dir>
    iterator operator --(int) noexcept {
        iterator __tmp = *this; this->operator--(); return __tmp;
    }

    template <void * = nullptr>
    requires random_access_iterator <traits>
    iterator operator += (difference_type __n) noexcept {
        advance <dir> (__n); return *this;
    }

    template <void * = nullptr>
    requires random_access_iterator <traits>
    iterator operator -= (difference_type __n) noexcept {
        advance <!dir> (__n); return *this;
    }

    reference operator * (void) const noexcept { return *traits::value_address(data); }
    pointer   operator ->(void) const noexcept { return  traits::value_address(data); }

    template <void * = nullptr>
    requires random_access_iterator <traits>
    reference operator [] (difference_type __n) const noexcept {
        iterator __tmp = *this; return *(__tmp += __n);
    }

};


template <class traits,bool k1,bool k2,bool dir>
bool operator == (const iterator <traits,k1,dir> &__lhs,
                  const iterator <traits,k2,dir> &__rhs) noexcept {
    return __lhs.base() == __rhs.base();
}


template <class traits,bool k1,bool k2,bool dir>
bool operator != (const iterator <traits,k1,dir> &__lhs,
                  const iterator <traits,k2,dir> &__rhs) noexcept {
    return __lhs.base() != __rhs.base();
}



template <class traits,bool is_const,bool dir>
requires random_access_iterator <traits>
iterator <traits,is_const,dir> operator + (
    iterator <traits,is_const,dir> __lhs,
    typename iterator <traits,is_const,dir>::difference_type __n
) noexcept {
    auto __tmp = __lhs;
    return __tmp += __n;
};


template <class traits,bool is_const,bool dir>
requires random_access_iterator <traits>
iterator <traits,is_const,dir> operator + (
    typename iterator <traits,is_const,dir>::difference_type __n,
    iterator <traits,is_const,dir> __rhs
) noexcept {
    auto __tmp = __rhs;
    return __tmp += __n;
};


template <class traits,bool is_const,bool dir>
requires random_access_iterator <traits>
iterator <traits,is_const,dir> operator - (
    iterator <traits,is_const,dir> __lhs,
    typename iterator <traits,is_const,dir>::difference_type __n
) noexcept {
    auto __tmp = __lhs;
    return __tmp -= __n;
};


template <class T>
struct pointer_iterator_trait {
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::contiguous_iterator_tag;
    using node_type         = T;
    using value_type        = T;

    static void advance(T *&__ptr) noexcept { ++__ptr; }
    static void backtrace(T *&__ptr) noexcept { --__ptr; }
    static void advance(T *&__ptr,difference_type __n) noexcept { __ptr += __n; }
    static value_type *value_address(node_type *__ptr) noexcept { return __ptr; }

    /* You can not create an entity of trait class. */
    pointer_iterator_trait() = delete;
};


template <class T,bool is_const,bool dir = true>
using pointer_iterator = iterator <pointer_iterator_trait <T>,is_const,dir>;



}


