#pragma once

#include "allocator.h"
#include "utility/list_base.h"

#include <initializer_list>
#include <ext/alloc_traits.h>

namespace dark {



template <class _Tp,class _Alloc_Type = allocator <_Tp>>
struct list : public list_base::list_traits <_Tp,_Alloc_Type,list <_Tp,_Alloc_Type>> {
  public:
    friend class list_base::list_traits <_Tp,_Alloc_Type,list>;

    using _Traits = typename list_base::list_traits <_Tp,_Alloc_Type,list>;
    using typename _Traits::value_type;
    using typename _Traits::iterator;
    using typename _Traits::const_iterator;
    using typename _Traits::reverse_iterator;
    using typename _Traits::const_reverse_iterator;

    using pointer         = value_type *;
    using reference       = value_type &;
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;

  protected:

    using typename _Traits::node_base;
    using typename _Traits::node_type;
    using typename _Traits::_Alloc;
    using typename _Traits::_Sized_Node;

    /* Header information. */
    _Sized_Node header;

  public:
    list() noexcept : header(0) {
        header.prev = header.next = std::addressof(header);
    }
    ~list() noexcept { this->remove_data(); }

    list(const list &rhs) : header(rhs.size()) {
        header.prev = header.next = std::addressof(header);
        this->insert(this->end(),rhs.begin(),rhs.end());
    }

    list(list &&rhs) noexcept {
        /* If empty , just normally initialize. */
        if(rhs.empty()) { ::dark::construct(this); return; }

        header.data = rhs.size();
        (header.next = rhs.head()->next)->prev = this->head();
        (header.prev = rhs.head()->prev)->next = this->head();
        ::dark::construct(std::addressof(rhs));
    }

    list(std::initializer_list <_Tp> __list) {
        ::dark::construct(this);
        this->insert(this->end(),__list.begin(),__list.end());
    }

    list &operator = (const list &rhs) {
        if(this != &rhs) {
            this->assign(rhs.begin(),rhs.end());
        } return *this;
    }

    list &operator = (list &&rhs) noexcept {
        if(this != &rhs) {
            this->remove_data();
            ::dark::construct(this,std::move(rhs));
        } return *this;
    }

    list &operator = (std::initializer_list <_Tp> __list) {
        this->assign(__list.begin(),__list.end());
        return *this;
    }

    /**
     * @brief Assign the list with certain copies of value.
     * @param __count Copy count.
     * @param __val Value to assign and fill the list.
     * @note O(n) complexity.
     * At most max(__count,size()) element destructions
     * and copy constructions/assignments are performed.
     */
    void assign(size_t __count,const _Tp &__val) {
        if (__count == 0) return this->clear();
        if (__count < size()) {
            auto __cur = this->begin();
            while (__count--) *(__cur++) = __val;
            this->erase(__cur,this->end());
        } else {
            for(auto &&__old : *this) __old = __val;
            this->insert(this->end(),__count - size(),__val);
        }
    }

    /**
     * @brief Assign the list with a range of elements.
     * @param __first First iterator. 
     * @param __last  Last iterator.
     * @note O(n) complexity.
     * At most max(__count,size()) element destructions
     * and custom constructions/assignments are performed.
     */
    template <class _Iter>
    requires type_iterator <_Tp,_Iter>
    void assign(_Iter __first,_Iter __last) {
        if (__first == __last) return this->clear();
        auto __cur = this->begin();
        do {
            if (__cur == this->end())
                return static_cast <void>
                    (this->insert(this->end(),__first,__last));
            *(__cur++) = *(__first++);
        } while (__first != __last);
        this->erase(__cur,this->end());
    }

    /**
     * @brief Assign the list with a initializer list.
     * @param __list Target initializer list.
     * @note O(n) complexity.
     * At most max(__count,size()) element destructions
     * and copy constructions/assignments are performed.
     */
    void assign(std::initializer_list <_Tp> __list) {
        assign(__list.begin(),__list.end());
    }

  public:
    /**
     * @return Return the size of the list in constant time.
     */
    size_t size() const noexcept { return  header.data; }
    /**
     * @return Return whether the list is empty in constant time.
     */
    bool empty()  const noexcept { return !header.data; }
  public:
    /* Some algorithms related functions. */

    /**
     * @brief Remove the consecutive duplicate elements from the list
     * and leave out just one copy of those identical element.
     * @return Count of removed elements.
     * @attention No iterators or references are invalidated.
     * Only those erased elements are invalidated (of course).
     * @note O(n) complexity. (n element destructions)
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
     * @note O(n) complexity. (m (m < n) element destructions + n pointer operation)
     */
    template <class _Equal>
    size_t unique(_Equal &&__eq) {
        iterator __ptr = this->begin();
        iterator __cur = __ptr;
        iterator __end = this->end();
        if (__ptr == __end) return 0;

        node_base __pool = { .prev = nullptr };

        while (++__ptr != __end) {
            if (__eq(*__ptr,*__cur)) {
                auto *__del = static_cast <node_type *> (list_base::erase(__ptr.base()));
                __del->prev = __pool.prev;
                __pool.prev = __del;
            } else __cur = __ptr;
        }

        size_t __count = 0;
        while (auto &__prev = __pool.prev) {
            auto *__node = static_cast <node_type *> (__prev);
            __prev = __node->prev;
            this->deallocate(__node);
            ++__count;
        }

        return __count;
    }
};


} // namespace dark


template <class T,class _Alloc_Type>
void ::std::swap(dark::list <T,_Alloc_Type> &lhs,dark::list <T,_Alloc_Type> &rhs)
noexcept { return static_cast <void> (lhs.swap(rhs)); }

