#pragma once


#include "linked_node.h"
#include "allocator.h"

#include <initializer_list>
#include <ext/alloc_traits.h>


namespace dark {

namespace list_base {

void push_front(linked_node <size_t> *__header,
                linked_node_base     *__node)
noexcept { ++__header->data; link_after(__header, __node); }

void push_back(linked_node <size_t> *__header,
               linked_node_base     *__node)
noexcept { ++__header->data; link_before(__header, __node); }

auto pop_front(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_after(__header); }

auto pop_back(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_before(__header); }


template <class T,class __Alloc>
struct list_traits {
    using node_base     = linked_node_base;
    using node_type     = linked_node <T>;
    using value_type    = T;
    using reference     = T &;
    using pointer       = T *;
    using T_Alloc       = __gnu_cxx::__alloc_traits<__Alloc>::template rebind<T>::other;
    using Alloc         = __gnu_cxx::__alloc_traits<T_Alloc>::template rebind<node_type>::other;

    using iterator               = list_iterator <node_type,false>;
    using const_iterator         = list_iterator <node_type, true>;
    using reverse_iterator       = list_iterator <node_type,false, false>;
    using const_reverse_iterator = list_iterator <node_type, true, false>;
};

}






using T         = int;
using __Alloc   = allocator<T>;

template <class T,class __Alloc = allocator <T>>
struct list : public list_base::list_traits <T,__Alloc> {
    using traits = typename list_base::list_traits <T,__Alloc>;
    using typename traits :: node_base;
    using typename traits :: node_type;
    using typename traits :: value_type;
    using typename traits :: reference;
    using typename traits :: pointer;
    using typename traits :: Alloc;
    using typename traits :: iterator;
    using typename traits :: const_iterator;
    using typename traits :: reverse_iterator;
    using typename traits :: const_reverse_iterator;
    
  protected:

    /* Real allocator. */
    [[no_unique_address]] Alloc     alloc;
    /* Header information. */
    list_base::linked_node <size_t> header;

    void remove_data() noexcept {
        auto *__ptr = static_cast <node_type *> (header.next);
        while (__ptr != static_cast <node_base *> (&header)) {
            auto *__tmp = __ptr->next;
            destroy <T> (&__ptr->data);
            alloc.deallocate(__ptr);
            __ptr = static_cast <node_type *> (__tmp);
        }
    }

  public:

    list() noexcept : header(0) { header.link[0] = header.link[1] = &header; }

    void push_front(const T &__val) {
        node_type *__node = alloc.allocate();
        construct_forward(&__node->data, __val);
        list_base::push_front(&header, __node);
    }

    void push_front(T &&__val) {
        node_type *__node = alloc.allocate();
        construct_forward(&__node->data, std::move(__val));
        list_base::push_front(&header, __node);
    }

    template <class ...Args>
    requires std::is_constructible_v <T,Args...>
    void emplace_front(Args &&...__args) {
        node_type *__node = alloc.allocate();
        construct_forward(__node, std::forward <Args>(__args)...);
        list_base::push_front(&header, __node);
    }

    void push_back(const T &__val) {
        node_type *__node = alloc.allocate();
        construct_forward(&__node->data, __val);
        list_base::push_back(&header, __node);
    }

    void push_back(T &&__val) {
        node_type *__node = alloc.allocate();
        construct_forward(&__node->data, std::move(__val));
        list_base::push_back(&header, __node);
    }

    template <class ...Args>
    requires std::is_constructible_v <T,Args...>
    void emplace_back(Args &&...__args) {
        node_type *__node = alloc.allocate();
        construct_forward(__node, std::forward <Args>(__args)...);
        list_base::push_back(&header, __node);
    }

    void pop_front() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_front(&header));
        destroy <T> (&__ptr->data);
        alloc.deallocate(__ptr);
    }

    void pop_back() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_back(&header));
        destroy <T> (&__ptr->data);
        alloc.deallocate(__ptr);
    }

    size_t size() const noexcept { return  header.data; }
    bool empty()  const noexcept { return !header.data; }

    iterator begin() noexcept { return iterator(header.next); }

    iterator end()   noexcept { return iterator(&header); }

    const_iterator begin() const noexcept { return const_iterator(header.next); }
    const_iterator end()   const noexcept { return const_iterator(&header); }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    reverse_iterator rbegin() noexcept { return reverse_iterator(header.prev); }
    reverse_iterator rend()   noexcept { return reverse_iterator(&header); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(header.prev); }
    const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(&header); }

    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend()   const noexcept { return rend();   }

    void clear() noexcept {
        remove_data();
        header.prev = header.next = &header; header.data = 0;
    }

    ~list() noexcept { remove_data(); }
};









}