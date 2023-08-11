#pragma once

#include <type_traits>
#include <utility>

namespace dark {

/**
 * @brief A node with value and additional tags.
 * 
 * @tparam node_base Node base type.
 * @tparam T The data type within.
 * @tparam tag Custom tagging(s).
 */
template <class node_base,class T,class ...tag>
struct value_node : node_base, tag...{
    using base_type  = node_base;
    using value_type = T;

    T data;
    value_node(value_node &&) = default;
    value_node(const value_node &) = default;
    ~value_node() = default;

    value_node(T &&__val)
        noexcept(std::is_nothrow_move_constructible_v <T>)
    : data(std::move(__val)) {}

    value_node(const T &__val)
        noexcept(std::is_nothrow_copy_constructible_v <T>)
    : data(__val) {}

    template <class U>
    requires
        std::is_constructible_v <T,U>
        && (!std::is_same_v <std::decay_t <U>,value_node>)
        && (!std::is_same_v <std::decay_t <U>,T>)
    value_node(U &&__val)
        noexcept(std::is_nothrow_constructible_v <T,U>)
    : data(std::forward <U> (__val)) {}

    template <class ...Args>
        requires std::is_constructible_v <T,Args...>
    value_node(Args &&...__args)
        noexcept(std::is_nothrow_constructible_v <T,Args...>)
    : data(std::forward <Args> (__args)...) {}

    value_node &operator = (value_node &&) = default;
    value_node &operator = (const value_node &) = default;
};




}