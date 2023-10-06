#pragma once

#include "allocator.h"

#include <cstring>
#include <stddef.h>
#include <concepts>
#include <initializer_list>
#include <ext/alloc_traits.h>
#include <bits/stl_algobase.h>
#include <bits/stl_uninitialized.h>


namespace dark {



namespace buffered_deque_base {


template <class _Tp,size_t _Nm,bool _Const,bool _Dir>
struct iterator {
  protected:
    using node_type         = std::conditional_t <_Const, const _Tp, _Tp>;
    using _Np               = node_type;
  public:
    using mutable_iterator  = iterator <_Tp,_Nm,false,_Dir>;
    using reverse_iterator  = iterator <_Tp,_Nm,_Const,!_Dir>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type        = node_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = node_type *;
    using reference         = node_type &;
    using compare_type      = std::__detail::__synth3way_t <pointer>;

    _Np * cur_node; /* Current node                 */
    _Np * buf_node; /* First node of the buffer.    */
    _Tp **map_node; /* Current position in the map. */

    /* Offset to the front of the buffer. */
    inline difference_type get_offset() const noexcept { return cur_node - buf_node; }

  protected:
    /* Some helper functions. */

    /* Move the pointer forward or backward. */
    template <bool _Real>
    void advance_pointer() {
        if constexpr (_Real == false) {
            if (cur_node != buf_node) { --cur_node; return; }
            buf_node = *(--map_node);
            cur_node = buf_node + (_Nm - 1);
        } else {
            if (++cur_node != buf_node + _Nm) { return; }
            cur_node = buf_node = *(++map_node);
        }
    }

    /* Move the pointers steps forward/backward. */
    void advance_pointer(difference_type __n) {
        /* Since we use the features of (un)signed integer to optimize. */
        static_assert(std::is_signed_v <difference_type> && std::is_unsigned_v <size_t>,
            "buffered_deque::iterator: a builtin error occurred!\n"
            "The difference type of deque iterator must be signed!");

        const auto __offset = get_offset() + __n;
        /* __offset in the range [0,_Nm) */
        if (static_cast <size_t> (__offset) < _Nm) { cur_node += __n; return ; }

        /* The division number. */
        constexpr auto _Div = static_cast <difference_type> (_Nm);

        /* Move the map pointer otherwise. */
        const auto __map_offset = __offset >= 0 ?
            __offset / _Div : (__offset + 1) / _Div - 1;

        map_node += __map_offset;
        buf_node = *map_node;
        cur_node = buf_node + (__offset - __map_offset * _Div);
    }

  public:
    /* Construct from 3 pointers explicitly. */
    explicit iterator(_Np *__cur, _Np *__buf, _Tp **__map)
    noexcept : cur_node(__cur), buf_node(__buf), map_node(__map) {}

    /* Construct from nothing. */
    iterator() noexcept : iterator(nullptr,nullptr,nullptr) {}

    /**
     * @brief Construct from an non_const_iterator to a const_iterator.
     * Of course, the reverse is not allowed.
     */
    template <void * = nullptr> requires (_Const == true)
    iterator(const mutable_iterator & __rhs)
    noexcept : iterator(__rhs.cur_node, __rhs.buf_node,__rhs.map_node) {}

    iterator(const iterator &) noexcept = default;
    iterator &operator = (const iterator &) noexcept = default;

    reference operator  *() const noexcept { return *cur_node; }
    pointer   operator ->() const noexcept { return  cur_node; }

    iterator &operator ++() noexcept {
        advance_pointer <_Dir> ();
        return *this;
    }

    iterator &operator --() noexcept {
        advance_pointer <!_Dir> ();
        return *this;
    }

    iterator &operator += (difference_type __n) noexcept {
        if constexpr (_Dir == false) { __n = -__n; }
        advance_pointer(__n);
        return *this;
    }

    iterator &operator -= (difference_type __n) noexcept {
        if constexpr (_Dir == true) { __n = -__n; }
        advance_pointer(__n);
        return *this;
    }

    friend difference_type operator -
        (const iterator &__lhs, const iterator &__rhs) noexcept {
        return static_cast <difference_type> (
            (__lhs.get_offset() - __rhs.get_offset())
          + (__lhs.map_node - __rhs.map_node) * static_cast <difference_type> (_Nm)
        );
    }

    friend bool operator == (const iterator &__lhs, const iterator &__rhs) noexcept {
        return __lhs.cur_node == __rhs.cur_node;
    }

    friend compare_type operator <=>
        (const iterator &__lhs, const iterator &__rhs) noexcept {
        if (auto __cmp = __lhs.map_node <=> __rhs.map_node; __cmp != 0) {
            return __cmp;
        } else {
            return __lhs.cur_node <=> __rhs.cur_node;
        }
    }

    /**
     * @return Reverse iterator pointing to the same data.
     */
    reverse_iterator reverse() const noexcept {
        return iterator <_Tp,_Nm,_Const,!_Dir> { cur_node, buf_node, map_node };
    }

    /**
     * @return Non-const iterator pointing to the same data.
     * @attention Be aware of what you are doing before abusing this function.
     */
    template <void * = nullptr> requires (_Const == true)
    mutable_iterator remove_const() const noexcept {
        return mutable_iterator {
            const_cast <_Tp *>  (cur_node),
            const_cast <_Tp *>  (buf_node),
            const_cast <_Tp **> (map_node)
        };
    }

  public:
    /* All functions below are generated from the basic. */

    value_type &operator [](difference_type __n) const noexcept {
        iterator __tmp { *this };
        return *(__tmp += __n);
    }

    iterator operator ++(int) noexcept {
        iterator __tmp { *this };
        this->operator++();
        return __tmp;
    }

    iterator operator --(int) noexcept {
        iterator __tmp { *this };
        this->operator--();
        return __tmp;
    }

    friend iterator operator +
        (const iterator &__lhs, difference_type __n) noexcept {
        iterator __tmp { __lhs };
        return __tmp += __n;
    }

    friend iterator operator +
        (difference_type __n, const iterator &__rhs) noexcept {
        iterator __tmp { __rhs };
        return __tmp += __n;
    }

    friend iterator operator -
        (const iterator &__lhs, difference_type __n) noexcept {
        iterator __tmp { __lhs };
        return __tmp -= __n;
    }

};


template <class _Tp,class _Alloc_Type,size_t _Nm>
struct deque_traits {
    using value_type        = _Tp;
    using _Buf_Pointer      = _Tp *;
    using _Map_Pointer      = _Tp **;
    using difference_type   = ptrdiff_t;

    using _Buf_Alloc        = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<_Tp >::other;
    using _Map_Alloc        = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<_Tp*>::other;

    using iterator                  = buffered_deque_base::iterator <_Tp,_Nm,false,true>;
    using const_iterator            = buffered_deque_base::iterator <_Tp,_Nm,true,true>;
    using reverse_iterator          = buffered_deque_base::iterator <_Tp,_Nm,false,false>;
    using const_reverse_iterator    = buffered_deque_base::iterator <_Tp,_Nm,true,false>;
};


} // namespace deque_base


/**
 * @param __size Size of the type.
 * @return Length for the buffered_deque's buffer.
 */
inline consteval size_t deque_buffer_size(size_t size) {
    enum { _S_buffer_size = 256 , _S_min_length = 4 };
    if (size * _S_min_length > _S_buffer_size) {
        return _S_min_length;
    } else {
        return _S_buffer_size / size;
    }
}


/**
 * @brief A buffered deque similar to std::deque.
 * Elements are stored in the buffer array.
 * Pointers to buffer arrays are managed in a map.
 * 
 * It's special that users can customize the buffer size.
 * In addition, its iterator has less space occupation.
 * It only requires 3 necessary pointers to maintain.
 * 
 * @attention Right value operations on the deque are special.
 * To ensure noexcept right value operations, the right value
 * deque will be set in a destruction-only state.
 * In other words, any operation on a moved-from deque is UB,
 * except for the destructor which will be naturally called.
 * 
 * In fact, destructor on moved_away containers will do nothing,
 * so it's safe to call it multiple times (even 0 times).
 * 
 * However, if you still wants to use the moved-from deque,
 * call reset() function to reset it to a valid state.
 * Nevertheless, we recommend you to avoid this situation
 * by calling swap instead of move assignment/constructor.
 * 
 */
template <
    class _Tp,
    size_t _Nm = deque_buffer_size(sizeof(_Tp)),
    class _Alloc_Type = allocator <_Tp>
>
struct buffered_deque : public buffered_deque_base::deque_traits <_Tp,_Alloc_Type,_Nm> {
  public:
    using _Traits = buffered_deque_base::deque_traits <_Tp,_Alloc_Type,_Nm>;
    using typename _Traits::value_type;
    using typename _Traits::difference_type;
    using typename _Traits::iterator;
    using typename _Traits::const_iterator;
    using typename _Traits::reverse_iterator;
    using typename _Traits::const_reverse_iterator;

    using pointer           = value_type *;
    using reference         = value_type &;
    using const_pointer     = const value_type *;
    using const_reference   = const value_type &;

  protected:
    inline static constexpr size_t _S_init_map_size = 8;

    using typename _Traits::_Buf_Pointer;
    using typename _Traits::_Map_Pointer;
    using typename _Traits::_Buf_Alloc;
    using typename _Traits::_Map_Alloc;

    [[no_unique_address]] _Buf_Alloc buf_alloc;
    [[no_unique_address]] _Map_Alloc map_alloc;

    _Map_Pointer map_head;  /* Pointer map.     */
    size_t       map_size;  /* Size of the map. */
    /* Head iterator pointing to the first element. */
    iterator    head;
    /* Tail iterator pointing to one past the last. */
    iterator    tail;

    /**
     * @brief Destroy data in the range.
     * @param __first Iterator pointing to first element.
     * @param __last  Iterator pointing to one past the last.
     * @attention No buffer is touched. Only elements are destroyed.
     */
    template <void * = nullptr>
    requires (!std::is_trivially_destructible_v <_Tp>)
    void destroy_data(const iterator &__first,const iterator &__last) {
        if (__first.map_node == __last.map_node) {
            dark::destroy(__first.cur_node,__last.cur_node);
        } else {
            dark::destroy(__first.cur_node,__first.buf_node + _Nm);
            dark::destroy(__last.buf_node,__last.cur_node);
            for(_Map_Pointer __beg = __first.map_node + 1; 
                __beg != __last.map_node ; ++__beg)
               dark::destroy(*__beg, _Nm);
        }
    }

    template <void * = nullptr>
    requires (std::is_trivially_destructible_v <_Tp>)
    void destroy_data(const iterator &,const iterator &) noexcept {}


    /* Deallocate the buffer in a range. */
    void deallocate_buffer(_Map_Pointer __beg, _Map_Pointer __end) {
        while(__beg != __end) buf_alloc.deallocate(*(__beg++),_Nm);
    }

    /* Deallocate the map. */
    void deallocate_map() noexcept {
        map_alloc.deallocate(map_head,map_size);        
    }

    /**
     * @brief Fill the inner map with given buffers.
     * It will set the head and tail pointers to where they should be.
     * @param __min_size The minimum size of the buffer.
     */
    void initialize_map_buffer(size_t __min_size) {
        map_size = std::max(__min_size + 2,_S_init_map_size);
        map_head = map_alloc.allocate(map_size);

        /**
         * For better performance, there is at least one available
         * map space at the front and back of the map.
         */

        _Map_Pointer __head = map_head + (map_size - __min_size) / 2;
        _Map_Pointer __tail =  __head  + __min_size;

        /* After loop, __tail = __head - 1. */
        while(__tail-- != __head) *__tail = buf_alloc.allocate(_Nm);

        head.buf_node = *(head.map_node = __head);
        tail.buf_node = *(tail.map_node = __tail + __min_size);
    }

    /**
     * @brief Inner initialization that fill the map
     * with given enougth node capacity and buffers.
     *  
     * It will allocate the buffers required and set the
     * head and tail pointers to where they should be.
     * 
     * @param __node_size Count of elements to be reserved.
     */
    void initialize_map(size_t __node_size) {
        initialize_map_buffer(__node_size / _Nm + 1);

        const size_t __mod    = __node_size % _Nm;
        const size_t __offset = (_Nm - __mod) / 2;

        head.cur_node = head.buf_node + __offset;
        tail.cur_node = tail.buf_node + __offset + __mod;
    }

    /**
     * @brief Reserve pointer to buffers.
     * @param __added Count of buffers to be added.
     * @param __dir True if at end, false if at front.
     * 
     * @note
     * If map size is twice more than the new size,
     * then we will just move those buffer pointers.
     * 
     * Else, we will reallocate a space for all these map pointers.
     * For better performance, the new map size will be set as
     * map_size + std::max(new_size + 1,map_size).
     * (a.k.a map_size will grow to at least 2 times bigger.)
     * 
     */
    void reallocate_map(size_t __added, bool __dir) {
        const size_t __old_count = allocated_buffer_size();
        const size_t __new_count = __old_count + __added;
        _Map_Pointer __new_start; /* New start pointer. */

        if (map_size > (__new_count << 1)) {
            /* Overlapping memory (Use memmove). */
            __new_start = map_head + (map_size - __new_count) / 2 + (__dir ? 0 : __added); 
            std::memmove(__new_start,head.map_node,__old_count * sizeof(_Buf_Pointer));
        } else {
            const size_t __new_size = map_size +
                (map_size > __new_count ? map_size : __new_count + 1);
            _Map_Pointer __new_map  = map_alloc.allocate(__new_size);

            /* No overlapping memory (Use memcpy). */
            __new_start = __new_map + (__new_size - __new_count) / 2 + (__dir ? 0 : __added);
            std::memcpy(__new_start,head.map_node,__old_count * sizeof(_Buf_Pointer));

            /* No destruction for trivial type. */
            deallocate_map();

            map_head = __new_map;
            map_size = __new_size;
        }

        head.map_node = __new_start;
        tail.map_node = __new_start + (__old_count - 1);
    }

    /* Reserve one space required to the front. */
    _Tp *allocate_front() {
        if (head.cur_node != head.buf_node) return --head.cur_node;
        if (head.map_node == map_head) reallocate_map(1,false);
        head.buf_node = *(--head.map_node) = buf_alloc.allocate(_Nm);
        return head.cur_node = head.buf_node + (_Nm - 1);
    }

    /* Reserve one space required to the back. */
    _Tp *allocate_back() {
        if (tail.cur_node != tail.buf_node + (_Nm - 1)) return tail.cur_node++;
        if (tail.map_node + 1 == map_head + map_size) reallocate_map(1,true);
        _Tp *__retval = tail.cur_node;
        tail.cur_node = tail.buf_node = *(++tail.map_node) = buf_alloc.allocate(_Nm);
        return __retval;
    }

    template <class _Iter>
    requires std::__is_random_access_iter <_Iter>::value
    void naive_assign(_Iter __first,_Iter __last) {
        const difference_type __old_size = size();
        if (__old_size > __last - __first) {
            erase_back(std::copy(__first,__last,begin()));
        } else {
            _Iter __mid = __first + __old_size;
            std::copy(__first,__mid,begin());
            insert_back(__mid,__last);
        }
    }

    template <class _Iter>
    requires (!std::__is_random_access_iter <_Iter>::value)
    void naive_assign(_Iter __first,_Iter __last) {
        iterator __cur = begin();
        for (; __cur != end() && __first != __last ;
                (void)++__cur, ++__first) *__cur = *__first;

        if (__first == __last) {
            if (__cur != end()) erase_back(__cur);
        } else {
            insert_back(__first,__last);
        }
    }

  public:
    /* Construct from nothing. */
    buffered_deque() { initialize_map(0); }

    /**
     * @brief Copy constructor.
     * @param __rhs Data source.
     * @note O(n) complexity. (O(n) element copy constructions)
     */
    buffered_deque(const buffered_deque &__rhs) {
        initialize_map_buffer(__rhs.allocated_buffer_size());
        head.cur_node = head.buf_node + __rhs.head.get_offset();
        tail.cur_node = tail.buf_node + __rhs.tail.get_offset();
        if (head.buf_node == tail.buf_node) {
            std::uninitialized_copy
                (__rhs.head.cur_node, __rhs.tail.cur_node, head.cur_node);
        } else {
            std::uninitialized_copy
                (__rhs.head.cur_node, __rhs.head.buf_node + _Nm, head.cur_node);
            std::uninitialized_copy
                (__rhs.tail.buf_node, __rhs.tail.cur_node,       tail.buf_node);

            for(_Map_Pointer __beg = __rhs.head.map_node + 1,
                             __cur = head.map_node;
                __beg != __rhs.tail.map_node ; ++__beg)
               std::uninitialized_copy(*__beg, *__beg + _Nm, *(++__cur));
        }
    }

    /**
     * @brief Move constructor.
     * @param __rhs Data source. It will be no longer valid after move.
     * @attention Users should never touch rhs after move.
     * It will be no longer valid (except for destruction).
     * Do not attempt to use it any more or errors will occur!
     * @note O(1) complexity. (O(1) basic operations.)
     */
    buffered_deque(buffered_deque &&__rhs) noexcept {
        if constexpr (std::is_standard_layout_v <buffered_deque>) {
            constexpr size_t __off = offsetof(buffered_deque,map_head);
            constexpr size_t __len = sizeof(buffered_deque) - __off;
            char *__beg = (char *)(&__rhs) + __off;
            std::memcpy(&map_head,__beg,__len);
        } else {
            map_head = __rhs.map_head;
            map_size = __rhs.map_size;
            head     = __rhs.head;
            tail     = __rhs.tail;
        }
        __rhs.map_head = nullptr;
    }

    /**
     * @brief Copy assignment.
     * @param __rhs Data source.
     * @note O(n) complexity. (n element copy constructions)
     */
    buffered_deque &operator = (const buffered_deque &__rhs) {
        if (std::addressof (__rhs) != this) {
            /* Short case. */
            naive_assign(__rhs.begin(),__rhs.end());
        } return *this;
    }

    /**
     * @brief Move assignment.
     * @param __rhs Data source. It will be no longer valid after move.
     * @attention Users should never touch rhs after move.
     * It will be no longer valid (except for destruction).
     * Do not attempt to use it any more or errors will occur!
     * @note O(n) complexity. (O(n) destructions operations.)
     */
    buffered_deque &operator = (buffered_deque &&__rhs) noexcept {
        if (std::addressof (__rhs) != this) {
            this->~buffered_deque();
            dark::construct(this,std::move(__rhs));
        } return *this;
    }

    /**
     * @brief  Reset the deque if it is in a invalid state.
     * @return Whether the deque is reset (a.k.a whether invalid.)
     */
    bool reset() {
        if (is_valid()) {
            return false;
        } else {
            return (void) initialize_map(0), true;
        }
    }

    ~buffered_deque() noexcept {
        if (!map_head) return;
        destroy_data(head,tail);
        deallocate_buffer(head.map_node,tail.map_node + 1);
        deallocate_map();
    }

    void push_front(const _Tp &__val) {
        dark::construct(allocate_front(),__val);
    }

    void push_front(_Tp &&__val) {
        dark::construct(allocate_front(),std::move(__val));
    }

    template <class ..._Args>
    requires std::constructible_from <_Tp,_Args...>
    void emplace_front(_Args &&...__args) {
        dark::construct(allocate_front(),std::forward <_Args> (__args)...);
    }

    void push_back(const _Tp &__val) {
        dark::construct(allocate_back(),__val);
    }

    void push_back(_Tp &&__val) {
        dark::construct(allocate_back(),std::move(__val));
    }

    template <class ..._Args>
    requires std::constructible_from <_Tp,_Args...>
    void emplace_back(_Args &&...__args) {
        dark::construct(allocate_back(),std::forward <_Args> (__args)...);
    }

    void pop_front() noexcept {
        dark::destroy(head.cur_node);
        if (head.cur_node != head.buf_node + (_Nm - 1)) {
            ++head.cur_node;
        } else {
            buf_alloc.deallocate(head.buf_node,_Nm);
            head.buf_node = *(++head.map_node);
            head.cur_node = head.buf_node;
        }
    }

    void pop_back() noexcept {
        if (tail.cur_node != tail.buf_node) {
            --tail.cur_node;            
        } else {
            buf_alloc.deallocate(tail.buf_node,_Nm);
            tail.buf_node = *(--tail.map_node);
            tail.cur_node = tail.buf_node + (_Nm - 1);
        }
        dark::destroy(tail.cur_node);
    }

    /**
     * @brief Erase from a position to the end.
     * @param __pos Position to erase from.
     * @attention Current position will be invalid after erase.
     * @note O(n) complexity. (n element destructions)
     */
    iterator erase_back(const_iterator __pos) noexcept {
        destroy_data(__pos.remove_const(), tail);
        deallocate_buffer(__pos.map_node + 1, tail.map_node + 1);
        return tail = __pos.remove_const();
    }

    /**
     * @brief Erase from a position to the front.
     * @param __pos Position to erase from.
     * @attention Current position is still valid after erase.
     * @note O(n) complexity. (n element destructions)
     */
    iterator erase_front(const_iterator __pos) noexcept {
        destroy_data(head, __pos.remove_const());
        deallocate_buffer(head.map_node, __pos.map_node);
        return head = __pos.remove_const();
    }

    /**
     * @brief Erase all elements in the deque.
     * @attention All iterators will be invalid after erase.
     * @note O(n) complexity. (n element destructions)
     */
    void clear() noexcept {
        destroy_data(head,tail);
        deallocate_buffer(head.map_node,tail.map_node);
        tail.cur_node = tail.buf_node + _Nm / 2;
        head = tail;
    }

    /**
     * @brief Insert a range to the back.
     * @param __first First iterator of the range.
     * @param __last  One past the last iterator of the range.
     */
    template <class _Iter>
    requires type_iterator <_Tp,_Iter>
    void insert_back(_Iter __first,_Iter __last) {
        const difference_type __dist  = std::distance(__first,__last);
        const difference_type __delta = __dist - (_Nm - tail.get_offset());
        if (__delta < 0) {
            std::uninitialized_copy(__first,__last,tail.cur_node);
            tail.cur_node += __dist;
        } else {
            const auto __added = __delta / _Nm + 1;
            reallocate_map(__added,true);
            for (size_t __i = 1; __i <= __added ; ++__i)
                tail.map_node[__i] = buf_alloc.allocate(_Nm);
            tail = std::uninitialized_copy(__first,__last,tail);
        }
    }

    /**
     * @brief Insert a range to the front.
     * @param __first First iterator of the range.
     * @param __last  One past the last iterator of the range.
     */
    template <class _Iter>
    requires type_iterator <_Tp,_Iter>
    void insert_front(_Iter __first,_Iter __last) {
        const difference_type __dist  = std::distance(__first,__last);
        const difference_type __delta = __dist - head.get_offset();
        if (__delta <= 0) {
            head.cur_node -= __dist;
            std::uninitialized_copy(__first,__last,head.cur_node);
        } else {
            const auto __added = (__delta + (_Nm - 1)) / _Nm;
            reallocate_map(__added,false);
            for (size_t __i = 0; __i < __added ; ++__i)
                *(--head.map_node) = buf_alloc.allocate(_Nm);
            head.buf_node = *(head.map_node);
            head.cur_node = head.buf_node + (__added * _Nm - __delta);
            std::uninitialized_copy(__first,__last,head);
        }
    }

  public:

    /**
     * @return Count of buffers being used.
     * @attention Even if the deque is empty, it will
     * still occupy one buffer. (a.k.a %retval >= 1)
     * @note O(1) complexity. (O(1) basic operations.)
    */
    size_t allocated_buffer_size() const noexcept {
        return tail.map_node - head.map_node + 1;
    }
    /**
     * @return Count of the inner map size.
     * @attention Even if the deque is empty, it will
     * still occupy one buffer. (a.k.a %retval >= 1)
     * @note O(1) complexity. (access 1 member variable.)
     */
    size_t allocated_map_size() const noexcept { return map_size; }
    /**
     * @return Count of elements in the deque.
     * @note O(1) complexity. (O(1) basic operations.)
     */
    size_t size() const noexcept { return tail - head; }
    /**
     * @return Whether the deque is empty.
     * @note O(1) complexity. (1 pointer comparison.)
     */
    bool empty() const noexcept { return tail == head; }
    /**
     * @return Buffer size (constexpr value).
     * @note (a.k.a inner length of one buffer array).
     */
    consteval size_t buffer_size() const noexcept { return _Nm; }
    /**
     * @return Whether the deque is valid (a.k.a not moved-from) 
     */
    bool is_valid() const noexcept { return map_head != nullptr; }


    iterator begin() noexcept { return head; }
    iterator end()   noexcept { return tail; }

    const_iterator begin() const noexcept { return head; }
    const_iterator end()   const noexcept { return tail; }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    reverse_iterator rbegin() noexcept { return ++(tail.reverse()); }
    reverse_iterator rend()   noexcept { return ++(head.reverse()); }

    const_reverse_iterator rbegin() const noexcept { return ++(tail.reverse()); }
    const_reverse_iterator rend()   const noexcept { return ++(head.reverse()); }    

    const_reverse_iterator crbegin() const noexcept { return rbegin();  }
    const_reverse_iterator crend()   const noexcept { return rend();    }

    reference front() noexcept { return *begin(); }
    const_reference front() const noexcept { return *begin(); }

    reference back()  noexcept { return *rbegin(); }
    const_reference back()  const noexcept { return *rbegin(); }

    reference operator [](size_t __pos) noexcept { return *(head + __pos); }
    const_reference operator [](size_t __pos) const noexcept { return *(head + __pos); }
};




} // namespace dark

