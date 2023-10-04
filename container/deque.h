#pragma once

#include "allocator.h"

#include <initializer_list>
#include <ext/alloc_traits.h>


namespace dark {


namespace buffered_deque_base {


template <class _Tp,class _Alloc_Type>
struct deque_traits {
    using value_type        = _Tp;
    using _Buf_Pointer      = _Tp*;
    using _Map_Pointer      = _Tp**;
    using reference         = _Tp&;
    using pointer           = _Tp*;
    using difference_type   = ptrdiff_t;

    using _Buf_Alloc        = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<_Tp >::other;
    using _Map_Alloc        = __gnu_cxx::__alloc_traits<_Alloc_Type>::template rebind<_Tp*>::other;
};


} // namespace deque_base


using _Tp           = size_t;
using _Alloc_Type   = allocator <_Tp>;

constexpr size_t BUF_SIZE = 512;
constexpr size_t ARR_SIZE = BUF_SIZE / sizeof(_Tp);


struct buffered_deque : public buffered_deque_base::deque_traits <_Tp,_Alloc_Type> {
  public:
    using _Traits = buffered_deque_base::deque_traits <_Tp,_Alloc_Type>;
    using typename _Traits::value_type;
    using typename _Traits::reference;
    using typename _Traits::pointer;
    using typename _Traits::difference_type;

  protected:
    using typename _Traits::_Buf_Pointer;
    using typename _Traits::_Map_Pointer;
    using typename _Traits::_Buf_Alloc;
    using typename _Traits::_Map_Alloc;

    [[no_unique_address]] _Buf_Alloc buf_alloc;
    [[no_unique_address]] _Map_Alloc map_alloc;

    _Map_Pointer map;   /* Beginning of the map. */
    size_t  map_size;   /* Size of the map.      */

    /**
     * @brief Reserve pointer to buffers.
     * @param __count Count of buffers to reserve.
     * @param __dir True if at end, false if at front.
     * 
     * @note
     * If map size is twice more than the new size,
     * then we will just move those buffer pointers.
     * 
     * Else, we will reallocate a space for all these map pointers.
     * For better performance, the new map size will be set as
     * twice more than the new size (new_size + map_size)
     * 
     */
    void reallocate_map(size_t __count,bool __dir) {

    }

  public:



};






} // namespace dark

