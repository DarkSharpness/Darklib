#pragma once

#include <type_traits>
#include <utility>

namespace dark {

/**
 * @brief A node with value and additional tags.
 * 
 * @tparam _Node_Type Node base type.
 * @tparam _Tp  The data type within.
 * @tparam _Tags Custom tagging(s).
 */
template <class _Node_Type, class _Tp, class ..._Tags>
struct value_node : _Node_Type, _Tags... {
    using base_type  = _Node_Type;
    using value_type = _Tp;

    _Tp data;
    value_node() = default;
    value_node(value_node &&) = default;
    value_node(const value_node &) = default;
    ~value_node() = default;

    value_node(_Tp &&__val)
        noexcept(std::is_nothrow_move_constructible_v <_Tp>)
    : data(std::move(__val)) {}

    value_node(const _Tp &__val)
        noexcept(std::is_nothrow_copy_constructible_v <_Tp>)
    : data(__val) {}

    template <class U>
    requires
        std::is_constructible_v <_Tp,U>
        && (!std::is_same_v <std::decay_t <U>,value_node>)
        && (!std::is_same_v <std::decay_t <U>,_Tp>)
    value_node(U &&__val)
        noexcept(std::is_nothrow_constructible_v <_Tp,U>)
    : data(std::forward <U> (__val)) {}

    template <class ...Args>
        requires std::is_constructible_v <_Tp,Args...>
    value_node(Args &&...__args)
        noexcept(std::is_nothrow_constructible_v <_Tp,Args...>)
    : data(std::forward <Args> (__args)...) {}

    value_node &operator = (value_node &&) = default;
    value_node &operator = (const value_node &) = default;
};




}