#pragma once

#include "node.h"
#include "iterator.h"


/* Class defines. */
namespace dark::list_base {

/* Node for forward list. */
struct forward_node_base  {
    /* The target node of current node. */
    forward_node_base *node;
    inline forward_node_base *get_next() const noexcept { return node; }
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

    inline linked_node_base *get_next() const noexcept { return next; }
    inline linked_node_base *get_prev() const noexcept { return prev; }
};

}


/* Functions. */
namespace dark::list_base {

/**
 * @brief Link a node after a node.
 * @param __node Current node. 
 * @param __prev Previous node.
 */
void link_after(forward_node_base *__node,
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
void link_dir(linked_node_base *__node, linked_node_base *__targ)
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
linked_node_base *unlink_dir(linked_node_base *__node) noexcept {
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


template <class list_node_type>
requires std::is_base_of_v <list_node_tag,list_node_type>
struct list_iterator_trait {
    list_iterator_trait() = delete;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using node_type         = typename list_node_type::base_type;
    using value_type        = typename list_node_type::value_type;

    static void advance  (node_type *&__node)
    noexcept { __node = static_cast <list_node_type *> (__node)->get_next(); }

    template <void * = nullptr>
    requires requires(node_type *__node) { __node->get_prev(); }
    static void backtrace(node_type *&__node)
    noexcept { __node = static_cast <list_node_type *> (__node)->get_prev(); }

    static value_type *value_address(node_type *__node)
    noexcept { return &static_cast <list_node_type *> (__node)->data; }

};


template <class T,bool is_const,bool dir = true>
using list_iterator = iterator <list_iterator_trait <T>,is_const,dir>;


}


