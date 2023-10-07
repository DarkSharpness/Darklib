#pragma once

#include "list_node.h"
#include "iterator.h"
#include "../../basic/memory.h"

/* Iterators. */
namespace dark::list_base {


struct list_pointer {
    static void advance (forward_node_base *&__ptr)         noexcept { __ptr = __ptr->node; }
    static void advance (const forward_node_base *&__ptr)   noexcept { __ptr = __ptr->node; }
    static void advance  (linked_node_base *&__ptr)         noexcept { __ptr = __ptr->next; }
    static void advance (const linked_node_base *&__ptr)    noexcept { __ptr = __ptr->next; }

    static void backtrace(linked_node_base *&__ptr)         noexcept { __ptr = __ptr->prev; }
    static void backtrace(const linked_node_base *&__ptr)   noexcept { __ptr = __ptr->prev; }
};


template <class _List_Node>
requires std::is_base_of_v <list_node_tag, _List_Node>
struct list_iterator_trait : list_pointer {
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



} // namespace dark::list_base


/* Functions and core list traits. */
namespace dark::list_base {

inline auto
push_front(linked_node_base *__head,
           linked_node_base *__node)
noexcept { return link_after(__head, __node); }

inline auto
push_back(linked_node_base *__head,
          linked_node_base *__node)
noexcept { return link_before(__head, __node); }

inline auto
pop_front(linked_node_base *__head)
noexcept { return unlink_after(__head); }

inline auto
pop_back(linked_node_base *__head)
noexcept { return unlink_before(__head); }

inline auto
insert (linked_node_base *__pos,
        linked_node_base *__node)
noexcept { return link_before(__pos, __node); }

inline auto erase(linked_node_base *__node)
noexcept { return unlink_node(__node); }

template <class _Tp,class _Alloc_Type,class list>
struct list_traits {
  public:
    using node_base         = linked_node_base;
    using node_type         = linked_node <_Tp>;
    using value_type        = _Tp;
    using pointer           = value_type *;
    using reference         = value_type &;
    using const_pointer     = const value_type *;
    using const_reference   = const value_type &;
    using T_Alloc           = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<_Tp>::other;
    using _Alloc            = __gnu_cxx::__alloc_traits<T_Alloc>::template rebind<node_type>::other;

    using iterator               = list_iterator <node_type,false ,true>;
    using const_iterator         = list_iterator <node_type, true, true>;
    using reverse_iterator       = list_iterator <node_type,false, false>;
    using const_reverse_iterator = list_iterator <node_type, true, false>;

  protected:
    using _Sized_Node = linked_node <size_t>;

    /* Real allocator. */
    [[no_unique_address]] _Alloc alloc;

    auto *head() noexcept {
        return std::addressof(static_cast <list *> (this)->header);
    }

    const auto *head() const noexcept {
        return std::addressof(static_cast <const list *> (this)->header);
    }

    static consteval bool is_sized_list() noexcept {
        return std::is_same_v <_Sized_Node,decltype(list::header)>;
    }

    /* Increase the size. */
    inline void inc_size(size_t __cnt) noexcept {
        if constexpr (is_sized_list()) head()->data += __cnt;        
    }

    /* Set the size to target. */
    inline void set_size(size_t __cnt) noexcept {
        if constexpr (is_sized_list()) head()->data = __cnt;
    }

    /* Remove all inner data without changing the size. */
    void remove_data() noexcept {
        auto *__ptr = static_cast <node_type *> (head()->next);
        while (__ptr != static_cast <node_base *> (head())) {
            auto *__tmp = __ptr;
            __ptr = static_cast <node_type *> (__ptr->next);
            ::dark::destroy(__tmp);
            alloc.deallocate(__tmp,1);
        }
    }

    iterator allocate_front() {
        node_type *__node = alloc.allocate(1);
        list_base::push_front(head(), __node);
        inc_size(1);
        return iterator {__node};
    }

    iterator allocate_back() {
        node_type *__node = alloc.allocate(1);
        list_base::push_back(head(), __node);
        inc_size(1);
        return iterator {__node};
    }

    iterator allocate_at(node_base *__pos) {
        node_type *__node = alloc.allocate(1);
        list_base::insert(__pos, __node);
        inc_size(1);
        return iterator {__node};
    }

    /**
     * @brief Just deallocate the node.
     * @param __node 
     */
    void deallocate(node_type *__node) {
        ::dark::destroy(__node);
        alloc.deallocate(__node,1);
        inc_size(-1);
    }


  public:

    /**
     * @brief Push one element to the front of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element copy construction)
     */
    void push_front(const _Tp &__val) {
        iterator __iter = allocate_front();
        ::dark::construct(std::addressof(*__iter), __val);
    }

    /**
     * @brief Push one element to the front of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element move construction)
     */
    void push_front(_Tp &&__val) {
        iterator __iter = allocate_front();
        ::dark::construct(std::addressof(*__iter), std::move(__val));
    }

    /**
     * @brief Construct an element in-place at the front of the list.
     * @param __args The arguments to be passed to the constructor.
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element custom construction)
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    reference emplace_front(Args &&...__args) {
        iterator __iter = allocate_front();
        ::dark::construct(std::addressof(*__iter), std::forward <Args> (__args)...);
        return *__iter;
    }

    /**
     * @brief Push one element to the back of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element copy construction)
     */
    void push_back(const _Tp &__val) {
        iterator __iter = allocate_back();
        ::dark::construct(std::addressof(*__iter), __val);
    }

    /**
     * @brief Push one element to the back of the list.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element move construction)
     */
    void push_back(_Tp &&__val) {
        iterator __iter = allocate_back();
        ::dark::construct(std::addressof(*__iter), std::move(__val));   
    }

    /**
     * @brief Construct an element in-place at the back of the list.
     * @param __args The arguments to be passed to the constructor.
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element custom construction)
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    reference emplace_back(Args &&...__args) {
        iterator __iter = allocate_back();
        ::dark::construct(std::addressof(*__iter), std::forward <Args> (__args)...);
        return *__iter;
    }

    /**
     * @brief Insert one element before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element copy construction)
     */
    iterator insert(const_iterator __pos,const _Tp &__val) {
        iterator __iter = allocate_at(__pos.remove_const().base());
        ::dark::construct(std::addressof(*__iter), __val);
        return __iter;
    }

    /**
     * @brief Insert one element before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __val The value to be inserted.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element move construction)
     */
    iterator insert(const_iterator __pos,_Tp &&__val) {
        iterator __iter = allocate_at(__pos.remove_const().base());
        ::dark::construct(std::addressof(*__iter), std::move(__val));
        return __iter;
    }

    /**
     * @brief Construct an element in-place before the specified position.
     * @param __pos Iterator to the position before which the element will be inserted.
     * @param __args The arguments to be passed to the constructor. 
     * @return Reference to the constructed element.
     * @attention No iterators or references are invalidated.
     * @note O(1) complexity. (1 element custom construction)
     */
    template <class ...Args>
    requires std::constructible_from <_Tp,Args...>
    iterator emplace(const_iterator __pos,Args &&...__args) {
        iterator __iter = allocate_at(__pos.remove_const().base());
        ::dark::construct(std::addressof(*__iter), std::forward <Args> (__args)...);
        return __iter;
    }

    /**
     * @brief Pop one element from the front of the list.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity. (1 element destruction)
     */
    void pop_front() noexcept {
        deallocate(static_cast <node_type *> (list_base::pop_front(head())));
    }

    /**
     * @brief Pop one element from the back of the list.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity. (1 element destruction)
     */
    void pop_back() noexcept {
        deallocate(static_cast <node_type *> (list_base::pop_back(head())));
    }

    /**
     * @brief Clear the list.
     * @note O(n) complexity. (n element destructions)
     */
    void clear() noexcept {
        remove_data();
        set_size(0);
        ::dark::construct(static_cast <list *> (this));
    }

  public:
    /* Range related functions. */

    template <class __Iter>
    requires type_iterator <_Tp,__Iter>
    iterator insert(const_iterator __pos,__Iter __first,__Iter __last) noexcept {
        if (__first == __last) return __pos.remove_const();
        constexpr bool __is_random_access = std::__is_random_access_iter <__Iter>::value;
        if constexpr (__is_random_access) { inc_size(__last - __first); }

        auto *__next = __pos.remove_const().base();
        auto *__temp = __next->prev;

        do {
            if constexpr (!__is_random_access) { inc_size(1); }
            auto *__node = alloc.allocate(1);
            ::dark::construct(std::addressof(__node->data), *(__first++));
            __node->prev = __temp;
            __temp->next = __node;
            __temp = __node;
        } while (__first != __last);

        auto *__prev = __next->prev;
        __temp->next = __next;
        __next->prev = __temp;

        return iterator {__prev->next};
    }

    iterator insert(const_iterator __pos,size_t __count, const _Tp &__val) noexcept {
        if (__count == 0) return __pos.remove_const();

        inc_size(__count);
        auto *__next = __pos.remove_const().base();
        auto *__temp = __next->prev;

        do {
            auto *__node = this->alloc.allocate(1);
            ::dark::construct(__node, __val);
            __node->prev = __temp;
            __temp->next = __node;
            __temp = __node;
        } while (--__count != 0);

        auto *__prev = __next->prev;
        __temp->next = __next;
        __next->prev = __temp;

        return iterator {__prev->next};
    }

    iterator insert(const_iterator __pos,std::initializer_list <_Tp> __list) noexcept {
        return insert(__pos,__list.begin(),__list.end());
    }

    /**
     * @brief Erase one element from the list.
     * @param __pos Iterator to the element to be erased.
     * @return Iterator to the next element.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(1) complexity. (1 element destruction)
     */
    iterator erase(const_iterator __pos) noexcept {
        auto *__loc = __pos.remove_const().base();
        auto *__tmp = __loc->next;
        auto *__ptr = static_cast <node_type *> (list_base::erase(__loc));
        ::dark::destroy(__ptr);
        this->alloc.deallocate(__ptr,1);
        return iterator(__tmp);
    }

    /**
     * @brief Erase a range of elements from the list.
     * @param __first Iterator to the first element to be erased. 
     * @param __last  Iterator to one past the last element to be erased.
     * @return Iterator to one past the last element erased.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(n) complexity. (n element destructions)
     */
    iterator erase(const_iterator __first,const_iterator __last) noexcept {
        if (__first == __last) return __first.remove_const();

        constexpr bool __is_random_access = std::__is_random_access_iter <const_iterator>::value;
        if constexpr (__is_random_access) { inc_size(__first - __last); }

        auto *__beg = static_cast <node_type *> (__first.remove_const().base());
        auto *__end = static_cast <node_type *> (__last.remove_const().base());

        __end->prev = __beg->prev;
        __beg->prev->next = __end;

        do {
            if constexpr (!__is_random_access) { inc_size(-1); }
            auto *__tmp = __beg;
            __beg = static_cast <node_type *> (__beg->next);
            ::dark::destroy(__tmp);
            this->alloc.deallocate(__tmp,1);
        } while(__beg != __end);

        return iterator(__end);
    }
    
  public:
    /* Data access functions part. */

    /**
     * @return Iterator to the first element in the list in constant time.
     */
    iterator begin() noexcept { return ++end(); }
    /**
     * @return Iterator to one past the last element in the list in constant time.
     */
    iterator end()   noexcept { return iterator(head()); }

    const_iterator begin() const noexcept { return ++end(); }
    const_iterator end()   const noexcept { return const_iterator(head()); }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    /**
     * @return Reverse_iterator to the last element in the list in constant time.
     */
    reverse_iterator rbegin() noexcept { return ++rend(); }
    /**
     * @return Reverse_iterator to one before the first element in the list in constant time.
     */
    reverse_iterator rend()   noexcept { return reverse_iterator(head()); }

    const_reverse_iterator rbegin() const noexcept { return ++rend(); }
    const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(head()); }

    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend()   const noexcept { return rend();   }

    reference front() noexcept { return *begin(); }
    const_reference front() const noexcept { return *begin(); }

    reference back() noexcept { return *rbegin(); }
    const_reference back() const noexcept { return *rbegin(); }

  public:
    /* Some algorithms. */

    /**
     * @brief Swap the content of two lists.
     * @param rhs Another list to swap with.
     * @attention No iterators or references are invalidated.
     * But note that the allocator is not swapped!
     * @note O(1) complexity (swap some pointers).
     */
    void swap(list &rhs) noexcept {
        std::swap(*head(),*rhs.head());
        head()->next->prev       = head();
        head()->prev->next       = head();
        rhs.head()->next->prev   = rhs.head();
        rhs.head()->prev->next   = rhs.head();
    }

    /**
     * @brief  Reverse the order of elements in the list.
     * @return Reference to current list.
     * @attention No iterators or references are invalidated.
     * @note O(n) complexity. (n pointer swaps)
     * No operation are performed on the elements themselves.
     */
    void reverse() noexcept {
        node_base *__ptr = head();
        do {
            std::swap(__ptr->next,__ptr->prev);
            __ptr = __ptr->prev;
        } while(__ptr != head());
    }

};

} // namespace dark::list_base
