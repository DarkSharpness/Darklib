#include <concepts>

namespace dark {

template <class __Iter>
concept valid_iterator = requires (__Iter __it) {
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
&&  std::convertible_to <std::decay_t <_Tp>, typename __Iter::value_type>;



}