#pragma once

#include "common.h"

#include <cstddef>
#include <utility>
#include <type_traits>


namespace dark {

/* Basic construction part (Default/Copy/Move) */


template <class _Tp>
requires std::constructible_from <_Tp>
inline void construct(_Tp *__ptr)
noexcept(noexcept(::new (__ptr) _Tp()))
{ ::new(__ptr) _Tp(); }

template <class _Tp>
requires std::copy_constructible <_Tp>
inline void construct(_Tp *__ptr, const _Tp &__val)
noexcept(noexcept(::new(__ptr) _Tp(__val)))
{ ::new(__ptr) _Tp(__val); }

template <class _Tp>
requires std::move_constructible <_Tp>
inline void construct(_Tp *__ptr, _Tp &&__val)
noexcept(noexcept(::new(__ptr) _Tp(std::move(__val))))
{ ::new (__ptr) _Tp(std::move(__val)); }


/* Non-basic construction part. */


template <class _Tp,class _Up>
requires std::constructible_from <_Tp, _Up> && (!std::is_same_v <_Tp, _Up>)
inline void construct(_Tp *__ptr, _Up &&__val)
noexcept(noexcept(::new (__ptr) _Tp(std::forward <_Up>(__val))))
{ ::new (__ptr) _Tp(std::forward <_Up>(__val)); }

template <class _Tp,class ..._Args>
requires std::constructible_from <_Tp, _Args...>
inline void construct(_Tp *__ptr, _Args &&...__val)
noexcept(noexcept(::new (__ptr) _Tp(std::forward <_Args>(__val)...)))
{ ::new (__ptr) _Tp(std::forward <_Args>(__val)...); }


/* Destruction part. */

/**
 * @brief Special version for trivially destructible type.
 * It can be used to avoid the overhead of calling destructor.
 */
template <class _Tp>
requires std::is_trivially_destructible_v <_Tp>
inline void destroy([[maybe_unused]] _Tp *) noexcept {}

/**
 * @brief Special version for trivially destructible type.
 * It can be used to avoid the overhead of calling destructor.
 * It avoids a loop when the type is trivially destructible.
 */
template <class _Tp>
requires std::is_trivially_destructible_v <_Tp>
inline void destroy([[maybe_unused]] _Tp *, [[maybe_unused]] size_t) noexcept {}

/**
 * @brief Special version for trivially destructible type.
 * It can be used to avoid the overhead of calling destructor.
 * It avoids a loop when the type is trivially destructible.
 */
template <class _Tp>
requires std::is_trivially_destructible_v <_Tp>
inline void destroy([[maybe_unused]] _Tp *, [[maybe_unused]] _Tp *) noexcept {}

/**
 * @brief Destory the object pointed by __ptr.
 * @param __ptr Pointer to the object. 
 */
template <class _Tp>
requires (!std::is_trivially_destructible_v <_Tp>)
inline void destroy(_Tp *__ptr)
noexcept(std::is_nothrow_destructible_v <_Tp>)
{ __ptr->~_Tp(); }

/**
 * @brief Destroys the objects in the range [__ptr, __ptr + __n).
 * @param __ptr Pointer to the range of objects.
 * @param __n   Number of objects to destroy.
 */
template <class _Tp>
requires (!std::is_trivially_destructible_v <_Tp>)
inline void destroy(_Tp *__ptr, size_t __n)
noexcept(std::is_nothrow_destructible_v <_Tp>)
{ while(__n--) (__ptr++)->~_Tp(); }

/**
 * @brief Destroys the objects in the range [__ptr, __ptr + __n).
 * @param __ptr Pointer to the range of objects.
 * @param __n   Number of objects to destroy.
 */
template <class _Tp>
requires (!std::is_trivially_destructible_v <_Tp>)
inline void destroy(_Tp *__beg, _Tp *__end)
noexcept(std::is_nothrow_destructible_v <_Tp>)
{ while(__beg != __end) (__beg++)->~_Tp(); }


} // namespace dark
