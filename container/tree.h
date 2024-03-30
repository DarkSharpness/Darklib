#pragma once
#include "node.h"

namespace dark {

namespace __detail::__tree {

enum Direction  : bool  { LT = 0, RT = 1 };
enum Color      : bool  { WHITE = 0, BLACK = 1 };

inline constexpr Direction operator !(Direction _Dir) {
    return static_cast <Direction> (!static_cast <bool> (_Dir));
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
    /* Return the direction of the node. */
    constexpr auto direction() const {
        return static_cast <Direction> (parent->child[0] != this);
    }
    /* Return whether the direction is true. */
    constexpr bool is_direction(Direction __d) const {
        return parent->child[__d] == this;
    }
    /* Return the brother/sister of the node. */
    constexpr auto other() const {
        return parent->child[!direction()];
    }
};
static_assert(sizeof(node) == 32);

template <typename _Tp>
using value_node = __node::value_node<_Tp, node>;

template <auto _Dir>
struct link_child {
    /**
     * @brief Set node's child as targeted and make link.
     * @param __node Parent node. Of course non-null.
     * @param __temp New child node. Must be non-null.
     * @attention __temp should not be a nullptr.
     */
    constexpr link_child
        (node *__restrict __node, node *__restrict __temp)
    { __node->child[_Dir] = __temp; __temp->parent = __node; }
};

template <auto _Dir>
struct try_link_child {
    /**
     * @brief Set node's child as targeted and make link.
     * @param __node Parent node. Of course non-null.
     * @param __temp New child node. Can be nullptr.
     * @attention The only difference from link_child is that
     * this function allows the child to be nullptr.
     */
    constexpr try_link_child
        (node *__restrict __node, node *__restrict __temp)
    { __node->child[_Dir] = __temp; if (__temp) __temp->parent = __node; }
};

template <>
struct link_child <nullptr> {
    const bool _Dir;
    constexpr link_child(bool _Dir) : _Dir(_Dir) {}
    constexpr void operator()
        (node *__restrict __node, node *__restrict __temp)
    const { __node->child[_Dir] = __temp; __temp->parent = __node; }
};

template <>
struct try_link_child <nullptr> {
    const bool _Dir;
    constexpr try_link_child(bool _Dir) : _Dir(_Dir) {}
    constexpr void operator()
        (node *__restrict __node, node *__restrict __temp)
    const { __node->child[_Dir] = __temp; if (__temp) __temp->parent = __node; }
};

template <bool _Is_Leaf>
struct link_leaf {
    const bool _Dir;
    constexpr link_leaf(bool _Dir) : _Dir(_Dir) {}
    /* Safely link on non-leaf cases. */
    constexpr void operator()
        (node *__restrict __node, node *__restrict __temp) const {
        if constexpr (_Is_Leaf) {
            /* __temp is a leaf's son: nullptr */
            __node->child[_Dir] = nullptr;
        } else {
            __node->child[_Dir] = __temp;
            __temp->parent      = __node;
        }
    }
};

/* Dynamicly binding the direction. */
link_child(Direction) -> link_child <nullptr>;
/* Dynamicly binding the direction. */
try_link_child(Direction) -> try_link_child <nullptr>;

} // namespace __detail::__tree


namespace __detail::__tree {

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

/**
 * @brief Update __node's parent's son __node to __next
 * @attention __next'parent will be set to __node's parent.
 */
inline constexpr void
relink_parent(node *__restrict __node, node *__restrict __next) {
    auto *__restrict __head = __node->parent;
    if (__head->parent == __node) // __node is root.
        __head->parent = __next;
    else
        __head->child[__node->direction()] = __next;
    __next->parent = __head;
}

/* Swap the information only. */
inline constexpr void
swap_info(node *__restrict __node, node *__restrict __next) {
    if (std::is_constant_evaluated()) {
        std::swap(__node->size, __next->size);
        std::swap(__node->color, __next->color);
    } else {
        std::swap(__node->info, __next->info);
    }
}

/**
 * @note
 * __node->child[RT] == __next  &&
 * __next->parent    == __node  &&
 * __next->child[LT] == nullptr
 */
inline constexpr void
swap_next_adjacent(node *__restrict __node, node *__restrict __next) {
    swap_info(__node, __next);

    /* Swap parent information */
    relink_parent(__node, __next);

    auto __left = __node->child[LT];    // node's left
    auto __succ = __next->child[RT];    // node's right

    link_child <LT> (__next, __left);
    link_child <RT> (__next, __node);
    try_link_child <LT> (__node, nullptr);
    try_link_child <RT> (__node, __succ);
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
    auto __head = __next->parent;
    relink_parent(__node, __next);
    link_child <LT> (__head, __node);

    auto __left = __node->child[LT];    // node's left
    auto __temp = __next->child[RT];    // node's right
    auto __succ = __next->child[RT];    // next's right

    link_child <LT> (__next, __left);
    link_child <RT> (__next, __temp);
    try_link_child <LT> (__node, nullptr);
    try_link_child <RT> (__node, __succ);
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
 * __x->parent->child[_Dir] == __x
 */
template <bool _Is_Leaf>
inline constexpr void rotate(node *__restrict __x, Direction _Dir) {
    auto __p = __x->parent;         // Parent.
    relink_parent(__p, __x);        // Relink parent.

    /* Safely relink the parent's children. */
    link_leaf<_Is_Leaf>{!_Dir}(__p, __x->child[_Dir]);
    /* Link the new children of x. */
    link_child{_Dir}(__x, __p);
}

/**
 * @note
 * __x != root &&
 * __x->parent->child[_Dir] == __x
 */
template <bool _Is_Leaf>
inline constexpr void zigzag(node *__restrict __x, Direction _Dir) {
    auto __p = __x->parent;         // Parent.
    auto __g = __p->parent;         // Grandparent.
    relink_parent(__g, __x);        // Relink grandparent.

    link_leaf<_Is_Leaf>{!_Dir}(__g, __x->child[_Dir]);
    link_leaf<_Is_Leaf>{ _Dir}(__p, __x->child[!_Dir]);

    link_child{ _Dir}(__x, __g);
    link_child{!_Dir}(__x, __p);
}

/* Special case that both __x and its parent is white. */
template <bool _Is_Leaf>
inline constexpr void insert_fix(node *__x) {
    auto __p = __x->parent;     // Parent node.
    auto __g = __p->parent;     // Grandparent node.
    __g->color = WHITE;         // Grand is always white.

    auto _Dir = __x->direction();
    if (__p->is_direction(_Dir)) {
        __p->color = BLACK;
        return rotate <_Is_Leaf> (__p, _Dir);
    } else { // Zig-zag case.
        __x->color = BLACK;
        return zigzag <_Is_Leaf> (__x, _Dir);
    }
}

/* Insert at a none-leaf inner node. */
inline constexpr void insert_none(node *__x) {
    // The header is always white, while the root should be black.
    while (__x->parent->color == WHITE) {
        if (__x->is_special())      // __x is root case
            return __x->color = BLACK, void();

        /* Because parent is white , parent can't be root. */
        auto __p = __x->parent;     // Parent node.
        auto __u = __p->other();    // Uncle node.
        if (__u->color == WHITE)    // Double red.
            return insert_fix <false> (__x);

        __p->color = __u->color = BLACK;
        (__x = __p->parent)->color = WHITE; // Move up.
    }
}

/* Insert at a non-root leaf node. */
inline constexpr void insert_leaf(node *__x) {
    if (__x->parent->color != WHITE) return;
    auto __p = __x->parent;     // Parent node.
    auto __u = __p->other();    // Uncle node.
    if (__u == nullptr)
        return insert_fix <true> (__x);

    __p->color = __u->color = BLACK;
    __p->parent->color = WHITE;
    return insert_none(__p->parent);
}


} // namespace __detail::__tree


} // namespace dark
