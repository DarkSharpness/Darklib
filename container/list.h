#pragma once


#include "linked_node.h"
#include "allocator.h"

#include <initializer_list>
#include <ext/alloc_traits.h>


namespace dark {

namespace list_base {

auto push_front(linked_node <size_t> *__header,
                linked_node_base     *__node)
noexcept { ++__header->data; return link_after(__header, __node); }

auto push_back(linked_node <size_t> *__header,
               linked_node_base     *__node)
noexcept { ++__header->data; return link_before(__header, __node); }

auto pop_front(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_after(__header); }

auto pop_back(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_before(__header); }

auto insert(linked_node <size_t> *__header,
            linked_node_base     *__pos,
            linked_node_base     *__node)
noexcept { ++__header->data; return link_before(__pos, __node); }

auto erase(linked_node <size_t> *__header,linked_node_base *__node)
noexcept { --__header->data; return unlink_node(__node); }


template <class T,class _Alloc_Type>
struct list_traits {
    using node_base     = linked_node_base;
    using node_type     = linked_node <T>;
    using value_type    = T;
    using reference     = T &;
    using pointer       = T *;
    using T_Alloc       = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<T>::other;
    using _Alloc        = __gnu_cxx::__alloc_traits<T_Alloc>::template rebind<node_type>::other;

    using iterator               = list_iterator <node_type,false ,true>;
    using const_iterator         = list_iterator <node_type, true, true>;
    using reverse_iterator       = list_iterator <node_type,false, false>;
    using const_reverse_iterator = list_iterator <node_type, true, false>;
};

}



template <class _Tp,class _Alloc_Type = allocator <_Tp>>
struct list : public list_base::list_traits <_Tp,_Alloc_Type> {
    using traits = typename list_base::list_traits <_Tp,_Alloc_Type>;
    using typename traits::node_base;
    using typename traits::node_type;
    using typename traits::value_type;
    using typename traits::reference;
    using typename traits::pointer;
    using typename traits::_Alloc;
    using typename traits::iterator;
    using typename traits::const_iterator;
    using typename traits::reverse_iterator;
    using typename traits::const_reverse_iterator;

  protected:
    /* Real allocator. */
    [[no_unique_address]] _Alloc     alloc;
    /* Header information. */
    list_base::linked_node <size_t> header;

    /* Remove all inner data without changing the size. */
    void remove_data() noexcept {
        auto *__ptr = static_cast <node_type *> (header.next);
        while (__ptr != static_cast <node_base *> (&header)) {
            auto *__tmp = __ptr->next;
            destroy(std::addressof(__ptr->data));
            alloc.deallocate(__ptr);
            __ptr = static_cast <node_type *> (__tmp);
        }
    }

    /* Copy the data from another list. */
    void copy_data(const list &rhs) noexcept {
        header.data = rhs.size();
        node_base *__last = &header;
        for(auto &&__val : rhs) {
            node_type *__node = alloc.allocate();
            construct_forward(std::addressof(__node->data), __val);
            __last->next = __node;
            __node->prev = __last;
            __last       = __node;
        }
        __last->next = &header;
        header.prev  = __last;
    }

  public:

    list() noexcept {
        header.link[0] = header.link[1] = &header;
        header.data = 0;
    }

    list(const list &rhs) { copy_data(rhs); }

    list(list &&rhs) noexcept {
        header.data = rhs.size();
        if(empty()) return;
        header.next = rhs.header.next;
        header.prev = rhs.header.prev;
        header.next->prev = &header;
        header.prev->next = &header;
        rhs.header.next = rhs.header.prev = &rhs.header;
    }

    list &operator =(const list &rhs) {
        if(this != &rhs) {
            remove_data();
            construct(this,rhs);
        } return *this;
    }

    list &operator =(list &&rhs) noexcept {
        if(this != &rhs) {
            remove_data();
            construct(this,std::move(rhs));
        } return *this;
    }

    void swap(list &rhs) noexcept {
        std::swap(header,rhs.header);
        header.next->prev = &header;
        header.prev->next = &header;
        rhs.header.next->prev = &rhs.header;
        rhs.header.prev->next = &rhs.header;
    }

    void push_front(const _Tp &__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data), __val);
        list_base::push_front(&header, __node);
    }

    void push_front(_Tp &&__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data), std::move(__val));
        list_base::push_front(&header, __node);
    }

    template <class ...Args>
    requires std::is_constructible_v <_Tp,Args...>
    reference emplace_front(Args &&...__args) {
        node_type *__node = alloc.allocate();
        construct_forward(std::addressof(__node->data), std::forward <Args>(__args)...);
        list_base::push_front(&header, __node);
        return __node->data;
    }

    void push_back(const _Tp &__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data), __val);
        list_base::push_back(&header, __node);
    }

    void push_back(_Tp &&__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data), std::move(__val));
        list_base::push_back(&header, __node);
    }

    template <class ...Args>
    requires std::is_constructible_v <_Tp,Args...>
    reference emplace_back(Args &&...__args) {
        node_type *__node = alloc.allocate();
        construct_forward(std::addressof(__node->data), std::forward <Args>(__args)...);
        list_base::push_back(&header, __node);
        return __node->data;
    }

    void insert(const_iterator __pos,const _Tp &__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data),__val);
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
    }

    void insert(const_iterator __pos,_Tp &&__val) {
        node_type *__node = alloc.allocate();
        construct(std::addressof(__node->data),std::move(__val));
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
    }

    template <class ...Args>
    requires std::is_constructible_v <_Tp,Args...>
    reference emplace(const_iterator __pos,Args &&...__args) {
        node_type *__node = alloc.allocate();
        construct_forward(std::addressof(__node->data),std::forward <Args>(__args)...);
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
        return __node->data;
    }

    void pop_front() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_front(&header));
        destroy(std::addressof(__ptr->data));
        alloc.deallocate(__ptr);
    }

    void pop_back() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_back(&header));
        destroy(std::addressof(__ptr->data));
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

    void clear() noexcept { remove_data(); construct(this); }

    ~list() noexcept { remove_data(); }

  public:
    /* Some algorithms related functions. */

    /**
     * @brief  Reverse the order of elements in the list in linear time.
     * @return Reference to current list.
     */
    void reverse() noexcept {
        node_base *__ptr = &header;
        do {
            std::swap(__ptr->next,__ptr->prev);
            __ptr = __ptr->prev;
        } while(__ptr != &header);
    }


    template <void * = nullptr>
    requires requires (_Tp __val) { __val == __val; }
    size_t unique() {
        iterator __ptr = begin();
        iterator __cur = __ptr;
        iterator __end = end();

        node_base __pool;
        __pool.prev = nullptr;

        while (++__ptr != __end) {
            if (*__ptr == *__cur) {
                auto *__del = static_cast <node_type *> 
                    ( list_base::erase(&header, __ptr.base()) );
                __del->prev = __pool.prev;
                __pool.prev = __del;
            } else __cur = __ptr;
        }

        size_t __count = 0;
        while (auto &__prev = __pool.prev) {
            auto *__node = static_cast <node_type *> (__prev);
            __prev = __node->prev;
            destroy(std::addressof(__node->data));
            alloc.deallocate(__node);
            ++__count;
        }

        return __count;
    }
};


}


template <class T,class _Alloc_Type>
void ::std::swap(dark::list <T,_Alloc_Type> &lhs,dark::list <T,_Alloc_Type> &rhs)
noexcept { return static_cast <void> (lhs.swap(rhs)); }

