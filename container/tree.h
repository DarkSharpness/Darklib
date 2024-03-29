#pragma once
#include "node.h"

namespace dark {

namespace __detail::__tree {

enum Direction  : bool  { LT = 0, RT = 1 };
enum Color      : bool  { WHITE = 0, BLACK = 1 };

inline constexpr Direction operator !(Direction __dir) {
    return static_cast <Direction> (!static_cast <bool> (__dir));
}

struct node {
    union {
        struct {
            Color   color;  // Color of the node.
            unsigned size;  // Size of the subtree.
        };
        size_t info; // Binded information, not available in constexpr.
    };
    node *parent;      // Parent node.
    node *child[2];    // Left and right child.


    /* Return whether the node is root or header. */
    constexpr bool is_special() const {
        return parent->parent == this;
    }
    /* Return whether the node is header. */
    constexpr bool is_header() const {
        return this->is_special() && color == WHITE;
    }
    /* Update parent's children and return parent. */
    constexpr auto update_parent(node *__next) {
        auto *__restrict __head = parent;
        if (__head->parent == this) // Parent is header.
            __head->parent = __next;
        else // Dummy case, normal modification.
            __head->child[__head->child[0] != this] = __next;
        return __head;
    }
};

static_assert(sizeof(node) == 32);

template <typename _Tp>
using value_node = __node::value_node<_Tp, node>;

template <Direction _Dir>
inline constexpr auto get_most(node *__node) -> node * {
    while (auto __next = __node->child[_Dir])
        __node = __next;
    return __node;
}

template <Direction _Dir>
inline constexpr auto advance(node *__node) -> node * {
    if (auto __next = __node->child[_Dir])
        return get_most <!_Dir> (__next);
    while (auto __head = __node->parent) {
        if (__head->child[_Dir] != __node) {
            /**
             * There are 2 special cases:
             * 
             * 1. Normal case, where __head has never gone to
             * header node of this tree. Just return __head.
             * 
             * 2. Special case, where __head has gone to header,
             * and unluckily, __head->child[_Dir] == __node.
             * Since only root's parent points to header, the loop
             * will stop when __head = root and __node = header.
             * 
             * So, if __node is header, return __node.
             * Otherwise, trivially return __head.
             */
            return __node->is_header() ? __node : __head;
        }
        __node = __head;
    }
    panic("Tree advance failed.");
}

inline constexpr void
swap_info(node *__restrict __node, node *__restrict __next) {
    if (std::is_constant_evaluated()) {
        std::swap(__node->size, __next->size);
        std::swap(__node->color, __next->color);
    } else {
        std::swap(__node->info, __next->info);
    }
}

inline constexpr auto
swap_parent(node *__restrict __node, node *__restrict __next) -> node * {
    auto __head = __node->update_parent(__next);
    auto __temp = __next->parent;
    __next->parent = __head;
    return __temp;
}

inline constexpr void
swap_left(node *__restrict __node, node *__restrict __next) {
    auto __left = __node->child[LT];
    __left->parent    = __next;
    __next->child[LT] = __left;
    __node->child[LT] = nullptr;
}

/**
 * @note
 * __node->child[RT] == __next  &&
 * __next->parent    == __node  &&
 * __next->child[LT] == nullptr
 */
inline constexpr void
swap_next_adjacent(node *__node, node *__next) {
    swap_info(__node, __next);

    /* Swap parent information */
    swap_parent(__node, __next);
    __node->parent = __next;

    /* Swap left information */
    swap_left(__node, __next);

    /* Swap right information */
    __node->child[RT] = __next->child[RT];
    if (auto __succ = __next->child[RT])
        __succ->parent = __node;
    __next->child[RT] = __node;
}

/**
 * @note
 * __next != __node->child[RT]          &&
 * __next == __next->parent->child[LT]  &&
 * __next->child[LT] == nullptr
 */
inline constexpr void
swap_next_distant(node *__restrict __node, node * __restrict __next) {
    swap_info(__node, __next);

    /* Swap parent information */
    auto __head = swap_parent(__node, __next);
    __node->parent = __head;
    __head->child[LT] = __node; // Must be left son.

    /* Swap left information */
    swap_left(__node, __next);

    /* Swap right information */
    auto __node_right = __node->child[RT];
    auto __next_right = __next->child[RT];

    __next->child[RT] = __node_right;
    __node->child[RT] = __next_right;

    if (__next_right != nullptr)
        __next_right->parent = __node;
    if (__node_right != nullptr)
        __node_right->parent = __next;
    else // This should never happen!
        panic("Invalid tree in swap_next.");
}

/* Find the successor and swap with it. */
inline constexpr void swap_next(node *__restrict __node) {
    auto __temp = __node->child[RT];
    if (__temp->child[LT] == nullptr)
        return swap_next_adjacent(__node, __temp);

    auto __next = get_most <LT> (__temp->child[LT]);
    return swap_next_distant(__node, __next);
}

/**
 * @note
 * __x != root &&
 * __x->parent->child[__dir] == __x
 */
inline constexpr void rotate(node *__x, Direction __dir) {
    auto *__p = __x->parent;        // Parent.
    auto &__a = __p->child[__dir];  // This side. (old = __x)
    auto &__b = __x->child[!__dir]; // Opposite side.

    /* Relink the opposite side's son to old parent. */
    __a = __b;
    __b = __p;
    if (__a != nullptr) __a->parent = __p;

    /* Update parent related information. */
    __x->parent = __p->update_parent(__x);
    __p->parent = __x;
}

} // namespace __detail::__tree


} // namespace dark
