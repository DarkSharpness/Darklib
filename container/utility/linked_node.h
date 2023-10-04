#pragma once

#include "node.h"
#include "iterator.h"


/* Class defines. */
namespace dark::list_base {

/* Node for forward list. */
struct forward_node_base {
    /* The target node of current node. */
    forward_node_base *node;
};

/* Node for linked list. */
struct linked_node_base {
    union {
        struct {
            linked_node_base *next; /* Successor node in the list. */
            linked_node_base *prev; /* Previous  node in the list. */
        };
        linked_node_base *link[2]; /* An alias of prev and next. */
    };
};

}


/* Functions. */
namespace dark::list_base {

/**
 * @brief Link a node after a node.
 * @param __node Current node. 
 * @param __prev Previous node.
 */
inline void link_after(forward_node_base *__node,
                forward_node_base *__prev) noexcept {
    __node->node = __prev->node;
    __prev->node = __node;
}


/**
 * @brief Link target node before or after current node.
 * 
 * @tparam __dir False if linked after, true if linked before.
 * @param __node Current node.
 * @param __targ Target node.
 */
template <bool __dir>
inline void link_dir(linked_node_base *__node, linked_node_base *__targ)
noexcept {
    __targ->link[ __dir] = __node->link[__dir];
    __targ->link[!__dir] = __node;

    __node->link[__dir]->link[!__dir] = __targ;
    __node->link[__dir] = __targ;
}

/**
 * @brief Link a node after current node.
 * 
 * @param __node Current node.
 * @param __next Node to be linked.
 */
inline void link_after(linked_node_base *__node, linked_node_base *__next)
noexcept { return link_dir <false> (__node,__next); }

/**
 * @brief Link a node before current node.
 * 
 * @param __node Current node.
 * @param __prev Node to be linked.
 */
inline void link_before(linked_node_base *__node,linked_node_base *__prev)
noexcept { return link_dir <true> (__node,__prev); }

/**
 * @brief Unlink the node from before or after current node.
 * 
 * @tparam __dir False if unlink after, true if unlink before.
 * @param __node Current node (which will not be unlinked.)
 * @return Return the unlinked node.
 */
template <bool __dir>
inline linked_node_base *unlink_dir(linked_node_base *__node) noexcept {
    linked_node_base *__link = __node->link[__dir];
    __node->link[__dir] = __link->link[__dir];
    __link->link[__dir]->link[!__dir] = __node;
    return __link;
}

/**
 * @brief Unlink the node after current node.
 * 
 * @param __node Current node.
 * @return Return the unlinked node.
 */
inline linked_node_base *unlink_after(linked_node_base *__node)
noexcept {  return unlink_dir <false> (__node); }

/**
 * @brief Unlink current node.
 * @param __node The node to be unlinked.
 * @return The unlinked node.
 */
inline linked_node_base *unlink_node(linked_node_base *__node) noexcept {
    linked_node_base *__prev = __node->prev;
    linked_node_base *__next = __node->next;
    __prev->next = __next;
    __next->prev = __prev;
    return __node;
}


/**
 * @brief Unlink the node before current node.
 * 
 * @param __node Current node.
 * @return Return the unlinked node.
 */
inline linked_node_base *unlink_before(linked_node_base *__node)
noexcept { return unlink_dir <true> (__node); }


}


/* Valued classes. */
namespace dark::list_base {

/* Empty tagging , marking it is a valued node. */
struct list_node_tag {};

template <class T>
using forward_node = value_node <forward_node_base,T,list_node_tag>;

template <class T>
using linked_node  = value_node <linked_node_base,T,list_node_tag>;


}


/* Iterators. */
namespace dark::list_base {

namespace helper {

struct list_pointer {
    static void advance (forward_node_base *&__ptr)         noexcept { __ptr = __ptr->node; }
    static void advance (const forward_node_base *&__ptr)   noexcept { __ptr = __ptr->node; }
    static void advance  (linked_node_base *&__ptr)         noexcept { __ptr = __ptr->next; }
    static void advance (const linked_node_base *&__ptr)    noexcept { __ptr = __ptr->next; }

    static void backtrace(linked_node_base *&__ptr)         noexcept { __ptr = __ptr->prev; }
    static void backtrace(const linked_node_base *&__ptr)   noexcept { __ptr = __ptr->prev; }
};


} // namespace helper


template <class _List_Node>
requires std::is_base_of_v <list_node_tag, _List_Node>
struct list_iterator_trait : helper::list_pointer {
    using node_type         = typename _List_Node::base_type;
    using value_type        = typename _List_Node::value_type;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using pointer           = value_type *;
    using reference         = value_type &;
    using compare_type      = void;

    static value_type &dereference(node_type *__ptr) noexcept {
        return static_cast <_List_Node *> (__ptr)->data;
    }
    static const value_type &dereference(const node_type *__ptr) noexcept {
        return static_cast <const _List_Node *> (__ptr)->data;
    }
};


template <class T,bool is_const,bool dir = true>
using list_iterator = basic_iterator <list_iterator_trait <T>,is_const,dir>;


}


