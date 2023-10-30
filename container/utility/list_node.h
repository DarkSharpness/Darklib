#pragma once

#include "node.h"
#include "iterator.h"
#include "../../basic/memory.h"


/* Class defines. */
namespace dark::list_base {

/* Node for forward list. */
struct forward_node_base { forward_node_base *node; /* Targeted node. */ };

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

} // namespace dark::list_base


/* Functions. */
namespace dark::list_base {

/**
 * @brief Make a link between two nodes.
 * @param __prev Previous node.
 * @param __next Next node.
 */
inline void hook(linked_node_base *__prev,linked_node_base *__next) {
    __prev->next = __next;
    __next->prev = __prev;
}


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
 * @brief Unlink the node before current node.
 * 
 * @param __node Current node.
 * @return Return the unlinked node.
 */
inline linked_node_base *unlink_before(linked_node_base *__node)
noexcept { return unlink_dir <true> (__node); }

/**
 * @brief Unlink current node.
 * @param __node The node to be unlinked.
 * @return The unlinked node.
 */
inline linked_node_base *unlink_node(linked_node_base *__node) noexcept {
    hook(__node->prev,__node->next);
    return __node;
}




} // namespace dark::list_base


/* Valued classes. */
namespace dark::list_base {

/* Empty tagging , marking it is a valued node. */
struct list_node_tag {};

template <class T>
using forward_node = value_node <forward_node_base,T,list_node_tag>;

template <class T>
using linked_node  = value_node <linked_node_base,T,list_node_tag>;

} // namespace dark::list_base
