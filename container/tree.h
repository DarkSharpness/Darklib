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

    /* Return true if the node is root or header. */
    constexpr bool is_special() const {
        return parent->parent == this;
    }
    /* Return true if the node is header. */
    constexpr bool is_header() const {
        return this->is_special() && color == WHITE;
    }
    /* Return the direction of the node. */
    constexpr auto direction() const {
        return static_cast <Direction> (parent->child[0] != this);
    }
    /* Return whether the node is on certain direction. */
    constexpr bool is_direction(Direction _Dir) const {
        return parent->child[_Dir] == this;
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

template <bool _Has_Child>
struct link_leaf {
    const bool _Dir;
    constexpr link_leaf(bool _Dir) : _Dir(_Dir) {}
    /* Safely link on non-leaf cases. */
    constexpr void operator()
        (node *__restrict __node, node *__restrict __temp) const {
        if constexpr (!_Has_Child) {
            /* __temp equals to nullptr */
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


/* Helper functions. */
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
 * @tparam _Has_Child Whether the node has child[_Dir].
 * @note
 * __x != root &&
 * __x->parent->child[_Dir] == __x
 */
template <bool _Has_Child>
inline constexpr void rotate(node *__restrict __x, Direction _Dir) {
    auto __p = __x->parent;         // Parent.
    relink_parent(__p, __x);        // Relink parent.

    /* Safely relink the parent's children. */
    link_leaf<_Has_Child>{!_Dir}(__p, __x->child[_Dir]);
    /* Link the new children of x. */
    link_child{_Dir}(__x, __p);
}

/**
 * @tparam _Has_Child Whether the node has both children.
 * @note
 * __x != root &&
 * __x->parent->child[_Dir] == __x
 */
template <bool _Has_Child>
inline constexpr void zigzag(node *__restrict __x, Direction _Dir) {
    auto __p = __x->parent;         // Parent.
    auto __g = __p->parent;         // Grandparent.
    relink_parent(__g, __x);        // Relink grandparent.

    link_leaf<_Has_Child>{!_Dir}(__g, __x->child[ _Dir]);
    link_leaf<_Has_Child>{ _Dir}(__p, __x->child[!_Dir]);

    link_child{ _Dir}(__x, __g);
    link_child{!_Dir}(__x, __p);
}

/**
 * @return True if the node is white or nullptr.
 * @attention Leaf nodes may be nullptr,
 * and we treat them just as white nodes.
 */
template <bool _Is_Leaf>
inline constexpr bool non_black(node *__x) {
    if constexpr (_Is_Leaf) return !__x;
    else return __x->color == WHITE;
}

/**
 * @return True if the node is black or nullptr.
 * @attention Leaf nodes may be nullptr,
 * and we treat them just as white nodes.
 */
template <bool _Is_Leaf>
inline constexpr bool non_white(node *__x) {
    if constexpr (_Is_Leaf) return !__x;
    else return __x->color != WHITE;
}

} // namespace __detail::__tree

/* Insert functions. */
namespace __detail::__tree {

/* Special case that both __x and its parent is white. */
template <bool _Is_Leaf>
inline constexpr void insert_fix(node *__x) {
    auto __p = __x->parent;     // Parent node.
    auto __g = __p->parent;     // Grandparent node.
    __g->color = WHITE;         // Grand is always white.

    auto _Dir = __x->direction();
    if (__p->is_direction(_Dir)) {
        __p->color = BLACK;
        return rotate <!_Is_Leaf> (__p, _Dir);
    } else { // Zig-zag case.
        __x->color = BLACK;
        return zigzag <!_Is_Leaf> (__x, _Dir);
    }
}

template <bool _Is_Leaf>
inline constexpr void insert(node *__x) {
    if (__x->parent->color != WHITE) return;

    // __x is root node. Only happen when not leaf.
    if (!_Is_Leaf && __x->is_special())
        return __x->color = BLACK, void();

    auto __p = __x->parent;     // Parent node.
    auto __u = __p->other();    // Uncle/Aunt node.

    if (non_black <_Is_Leaf> (__u))
        return insert_fix <_Is_Leaf> (__x);

    __p->color = __u->color = BLACK;
    __p->parent->color = WHITE;
    return insert <false> (__p->parent);
}


} // namespace __detail::__tree


/* Erase functions. */
namespace __detail::__tree {

/* Replace x with its only child y. */
inline constexpr void erase_single(node *__x, node *__y) {
    __y->color = BLACK;
    relink_parent(__x, __y);
}

/* Clear the branch containing current node. */
inline constexpr void erase_bare(node * __node) {
    auto *__restrict __head = __node->parent;
    if (__head->parent == __node) // __node is root.
        __head->parent = nullptr;
    else
        __head->child[__node->direction()] = nullptr;
}

/* Fix the tree after erasing a black leaf. */
template <bool _Is_Leaf>
inline constexpr void erase(node *__x) {
    if (__x->is_special()) return; // x is root.

    auto __p = __x->parent;     // Parent node.
    auto __b = __p->other();    // Brother/Sister node.
    auto _Dir = __x->direction();

    if (__b->color == WHITE) {
        __b->color = BLACK;
        __p->color = WHITE;
        rotate <true> (__b, !_Dir);
        __b = __p->child[!_Dir];
    }
    /* Now the brother must be black. */

    /* Inner (zig-zag) side child. */
    if (auto __i = __b->child[_Dir] ; !non_white <_Is_Leaf> (__i)) {
        __i->color = __p->color;
        __p->color = BLACK;
        return zigzag <!_Is_Leaf> (__i, _Dir);
    }

    /* Outer (!_Dir) side child. */
    if (auto __o = __b->child[!_Dir]; !non_white <_Is_Leaf> (__o)) {
        __o->color = __p->color;
        return rotate <!_Is_Leaf> (__b, !_Dir);
    }

    /* Now __b has only black sons. */
    if (__p->color == WHITE) {
        __p->color = BLACK;
        __b->color = WHITE;
        return; // Done.
    }

    return erase <false> (__p);
}

inline constexpr bool erase_adjust(node *__x) {
    auto __lchild = __x->child[LT];
    auto __rchild = __x->child[RT];

    if (__lchild) {
        if (!__rchild) {
            erase_single(__x, __lchild);
            return true;
        }
        swap_next(__x);
        __rchild = __x->child[RT];
    }
    if (__rchild) {
        erase_single(__x, __rchild);
        return true;
    }

    return false;
}


/* Erase the node. */
inline constexpr void erase_at(node *__x) {
    if (erase_adjust(__x)) return;
    erase <true> (__x);
    return erase_bare(__x);
}


} // namespace __detail::__tree




} // namespace dark
