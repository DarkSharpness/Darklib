#pragma once

#include "utility/linked_node.h"
#include "allocator.h"

#include <initializer_list>
#include <ext/alloc_traits.h>


namespace dark {


namespace list_base {

inline auto
push_front(linked_node <size_t> *__header,
           linked_node_base     *__node)
noexcept { ++__header->data; return link_after(__header, __node); }

inline auto
push_back(linked_node <size_t> *__header,
          linked_node_base     *__node)
noexcept { ++__header->data; return link_before(__header, __node); }

inline auto
pop_front(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_after(__header); }

inline auto
pop_back(linked_node <size_t> *__header)
noexcept { --__header->data; return unlink_before(__header); }

inline auto
insert (linked_node <size_t> *__header,
        linked_node_base     *__pos,
        linked_node_base     *__node)
noexcept { ++__header->data; return link_before(__pos, __node); }

inline auto
erase(linked_node <size_t> *__header,linked_node_base *__node)
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

} // namespace list_base


template <class _Tp,class _Alloc_Type = allocator <_Tp>>
struct list : public list_base::list_traits <_Tp,_Alloc_Type> {
  public:
    using _Traits = typename list_base::list_traits <_Tp,_Alloc_Type>;
    using typename _Traits::value_type;
    using typename _Traits::reference;
    using typename _Traits::pointer;
    using typename _Traits::iterator;
    using typename _Traits::const_iterator;
    using typename _Traits::reverse_iterator;
    using typename _Traits::const_reverse_iterator;

  protected:
    using typename _Traits::node_base;
    using typename _Traits::node_type;
    using typename _Traits::_Alloc;

    /* Real allocator. */
    [[no_unique_address]] _Alloc     alloc;
    /* Header information. */
    list_base::linked_node <size_t> header;

    /* Remove all inner data without changing the size. */
    void remove_data() noexcept {
        auto *__ptr = static_cast <node_type *> (header.next);
        while (__ptr != static_cast <node_base *> (&header)) {
            auto *__tmp = __ptr->next;
            ::dark::destroy(__ptr);
            alloc.deallocate(__ptr,1);
            __ptr = static_cast <node_type *> (__tmp);
        }
    }

    /* Copy the data from another list. */
    void copy_data(const list &rhs) noexcept {
        header.data = rhs.size();
        node_base *__last = &header;
        for(auto &&__val : rhs) {
            node_type *__node = alloc.allocate(1);
            ::dark::construct(__node, __val);
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

    ~list() noexcept { remove_data(); }

    list(list &&rhs) noexcept {
        /* If empty , just normally initialize. */
        if(rhs.empty()) { ::dark::construct(this); return; }

        header.data = rhs.size();
        header.next = rhs.header.next;
        header.prev = rhs.header.prev;
        header.next->prev = &header;
        header.prev->next = &header;
        rhs.header.next = rhs.header.prev = &rhs.header;
    }

    list(std::initializer_list <_Tp> __list) {
        ::dark::construct(this);
        for(auto &&__val : __list) push_back(__val);
    }

    list &operator = (const list &rhs) {
        if(this != &rhs) {
            remove_data();
            ::dark::construct(this,rhs);
        } return *this;
    }

    list &operator = (list &&rhs) noexcept {
        if(this != &rhs) {
            remove_data();
            ::dark::construct(this,std::move(rhs));
        } return *this;
    }

    /**
     * @brief Swap the content of two lists.
     * @param rhs Another list to swap with.
     * @attention No iterators or references are invalidated.
     * But note that the allocator is not swapped!
     * @note O(1) complexity (swap some pointers).
     */
    void swap(list &rhs) noexcept {
        std::swap(header,rhs.header);
        header.next->prev = &header;
        header.prev->next = &header;
        rhs.header.next->prev = &rhs.header;
        rhs.header.prev->next = &rhs.header;
    }

    /**
     * @brief Push one element to the front of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element copy constructor).
     */
    void push_front(const _Tp &__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, __val);
        list_base::push_front(&header, __node);
    }

    /**
     * @brief Push one element to the front of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element move constructor).
     */
    void push_front(_Tp &&__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::move(__val));
        list_base::push_front(&header, __node);
    }

    /**
     * @brief Construct an element in-place at the front of the list.
     * @param __args The arguments to be passed to the constructor.
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element custom constructor).
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    reference emplace_front(Args &&...__args) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::forward <Args> (__args)...);
        list_base::push_front(&header, __node);
        return __node->data;
    }

    /**
     * @brief Push one element to the back of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element copy constructor).
     */
    void push_back(const _Tp &__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, __val);
        list_base::push_back(&header, __node);
    }

    /**
     * @brief Push one element to the back of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element move constructor).
     */
    void push_back(_Tp &&__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::move(__val));
        list_base::push_back(&header, __node);
    }

    /**
     * @brief Construct an element in-place at the back of the list.
     * @param __args The arguments to be passed to the constructor.
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element custom constructor).
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    reference emplace_back(Args &&...__args) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::forward <Args> (__args)...);
        list_base::push_back(&header, __node);
        return __node->data;
    }

    /**
     * @brief Insert one element before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element copy constructor).
     */
    void insert(const_iterator __pos,const _Tp &__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, __val);
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
    }

    /**
     * @brief Insert one element before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element move constructor).
     */
    void insert(const_iterator __pos,_Tp &&__val) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::move(__val));
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
    }

    /**
     * @brief Construct an element in-place before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __args The arguments to be passed to the constructor. 
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity (1 element custom constructor).
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    reference emplace(const_iterator __pos,Args &&...__args) {
        node_type *__node = alloc.allocate(1);
        ::dark::construct(__node, std::forward <Args> (__args)...);
        node_base *__ptr = const_cast <node_base *> (__pos.base());
        list_base::insert(&header,__ptr,__node);
        return __node->data;
    }

    /**
     * @brief Pop one element from the front of the list.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity (1 element destructor).
     */
    void pop_front() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_front(&header));
        ::dark::destroy(__ptr);
        alloc.deallocate(__ptr,1);
    }

    /**
     * @brief Pop one element from the back of the list.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity (1 element destructor).
     */
    void pop_back() noexcept {
        auto *__ptr = static_cast <node_type *> (list_base::pop_back(&header));
        ::dark::destroy(__ptr);
        alloc.deallocate(__ptr,1);
    }

    /**
     * @brief Clear the list.
     * @note O(n) complexity (n element destructors).
     */
    void clear() noexcept { remove_data(); ::dark::construct(this); }

    /**
     * @brief Erase one element from the list.
     * @param __pos Iterator to the element to be erased.
     * @return Iterator to the next element.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity (1 element destructor).
     */
    iterator erase(const_iterator __pos) noexcept {
        auto *__loc = const_cast <node_base *> (__pos.base());
        auto *__tmp = __loc->next;
        auto *__ptr = static_cast <node_type *> (list_base::erase(&header,__loc));
        ::dark::destroy(__ptr);
        alloc.deallocate(__ptr,1);
        return iterator(__tmp);
    }

    /**
     * @brief Erase a range of elements from the list.
     * @param __first Iterator to the first element to be erased. 
     * @param __last  Iterator to one past the last element to be erased.
     * @return Iterator to one past the last element erased.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(n) complexity (n element destructors).
     */
    iterator erase(const_iterator __first,const_iterator __last) noexcept {
        auto *__beg = const_cast <node_base *> (__first.base());
        auto *__end = const_cast <node_base *> (__last.base());
        while (__beg != __end) {
            auto *__ptr = static_cast <node_type *> (list_base::erase(&header,__beg));
            __beg = __beg->next;
            ::dark::destroy(__ptr);
            alloc.deallocate(__ptr,1);
        } return iterator(__end);
    }

    /**
     * @return Return the size of the list in constant time.
     */
    size_t size() const noexcept { return  header.data; }
    /**
     * @return Return whether the list is empty in constant time.
     */
    bool empty()  const noexcept { return !header.data; }

    /**
     * @return Iterator to the first element in the list in constant time.
     */
    iterator begin() noexcept { return iterator(header.next); }
    /**
     * @return Iterator to one past the last element in the list in constant time.
     */
    iterator end()   noexcept { return iterator(&header); }

    const_iterator begin() const noexcept { return const_iterator(header.next); }
    const_iterator end()   const noexcept { return const_iterator(&header); }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    /**
     * @return Reverse_iterator to the last element in the list in constant time.
     */
    reverse_iterator rbegin() noexcept { return reverse_iterator(header.prev); }
    /**
     * @return Reverse_iterator to one before the first element in the list in constant time.
     */
    reverse_iterator rend()   noexcept { return reverse_iterator(&header); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(header.prev); }
    const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(&header); }

    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend()   const noexcept { return rend();   }

  public:
    /* Some algorithms related functions. */

    /**
     * @brief  Reverse the order of elements in the list.
     * @return Reference to current list.
     * @attention No iterators or references are invalidated.
     * @note O(n) complexity (n pointer swaps).
     * No operation are performed on the elements themselves.
     */
    void reverse() noexcept {
        node_base *__ptr = &header;
        do {
            std::swap(__ptr->next,__ptr->prev);
            __ptr = __ptr->prev;
        } while(__ptr != &header);
    }

    /**
     * @brief Remove the consecutive duplicate elements from the list
     * and leave out just one copy of those identical element.
     * @return Count of removed elements.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(n) complexity (n element destructors).
     */
    size_t unique() {
        static_assert (requires (_Tp __val) { __val == __val; },
            "std::unique() requires operator == or comparator function"
        );
        return unique(std::equal_to <_Tp> ());
    }

    /**
     * @brief Remove the consecutive duplicate elements from the list
     * and leave out just one copy of those identical element.
     * @param __eq The comparator function which returns whether two elements are equal.
     * @return Count of removed elements.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(n) complexity (m (m < n) element destructors + n pointer operation).
     */
    template <class _Equal>
    size_t unique(_Equal &&__eq) {
        iterator __ptr = begin();
        iterator __cur = __ptr;
        iterator __end = end();
        if (__ptr == __end) return 0;

        node_base __pool = { .prev = nullptr };

        while (++__ptr != __end) {
            if (__eq(*__ptr,*__cur)) {
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
            ::dark::destroy(__node);
            alloc.deallocate(__node,1);
            ++__count;
        }

        return __count;
    }

};


} // namespace dark


template <class T,class _Alloc_Type>
void ::std::swap(dark::list <T,_Alloc_Type> &lhs,dark::list <T,_Alloc_Type> &rhs)
noexcept { return static_cast <void> (lhs.swap(rhs)); }

