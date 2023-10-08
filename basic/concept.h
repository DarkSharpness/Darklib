#pragma once

#include <bit>
#include <concepts>

namespace dark {

template <class __Iter>
concept valid_iterator = std::is_pointer_v <__Iter> ||  requires (__Iter __it) {
    typename __Iter::value_type;
    typename __Iter::reference;
    typename __Iter::pointer;
    typename __Iter::difference_type;
    typename __Iter::iterator_category;

    { __it.operator->() } -> std::convertible_to <typename __Iter::pointer>;
    { __it.operator*() }  -> std::convertible_to <typename __Iter::reference>;
    { __it == __it }      -> std::convertible_to <bool>;
    { ++__it }            -> std::same_as <__Iter &>;
};


template <class _Tp,class __Iter>
concept type_iterator = 
    valid_iterator <__Iter>
&&  requires (__Iter __it) {
    typename std::enable_if_t <
        std::is_constructible_v <
            std::decay_t <_Tp>, decltype (*__it)
        >,void *>;
};

consteval bool is_little_endian() noexcept {
    return std::endian::native == std::endian::little;
}

consteval bool is_big_endian() noexcept {
    return std::endian::native == std::endian::big;
}






}