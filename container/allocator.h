#pragma once

#include "common.h"
#include "../basic/memory.h"

#include <cstdlib>
#include <type_traits>
#include <utility>

namespace dark {


#ifdef _DARK_DEBUG
class allocator_debugger;

class allocator_debug_helper {
  protected:
    struct debug_pack {
        allocator_debug_helper *pointer;
        size_t count;
    };

    friend class allocator_debugger;

    size_t refer_count = 0;
    size_t alloc_count = 0;
    size_t freed_count = 0;
    size_t alloc_times = 0;
    size_t freed_times = 0;    

    inline static allocator_debug_helper *single = nullptr;
    inline static constexpr size_t offset = sizeof(debug_pack);

    allocator_debug_helper() = delete;
    allocator_debug_helper(std::nullptr_t) noexcept {
        normal("Debug allocator is enabled!");
    }

    static size_t pack_length(size_t __n) noexcept {
        return (__n + (offset - 1)) / offset + 2;
    }

    /* Aligned to offset. */
    static size_t real_size(size_t __n) noexcept {
        return pack_length(__n) * offset;
    }

    void *allocate(void *__ptr,size_t __n) {
        if(!__ptr) throw std::bad_alloc {};

        alloc_times += 1;
        alloc_count += __n;

        debug_pack __val = {this, __n};
        auto * const __tmp = static_cast <debug_pack *> (__ptr);
        *__tmp = __val;
        *(__tmp + pack_length(__n) - 1) = __val;
        return __tmp + 1;
    }

    void *deallocate(void *__ptr) {
        if(!__ptr) return nullptr;
        auto * const __tmp = static_cast <debug_pack *> (__ptr) - 1;
        debug_pack __val = *__tmp;

        if (__val.pointer != this)
            throw error("Invalid deallocation!");

        auto *__end = __tmp + pack_length(__val.count) - 1;
        if (__end->pointer != this
        ||  __end->count != __val.count)
            throw error("Invalid deallocation!");

        freed_times += 1;
        freed_count += __val.count;

        return __tmp;
    }

    static allocator_debug_helper *get_object() {
        if(single == nullptr) single = new allocator_debug_helper(nullptr);
        ++single->refer_count;
        return single;
    }

    ~allocator_debug_helper() noexcept(false) {
        auto __str = std::format (
                // "Memory leak detected!\n"
                "Allocated: {} bytes, {} times.\n"
                "Freed: {} bytes, {} times.\n"
                "Total loss: {} bytes, {} times.\n",
                alloc_count, alloc_times,
                freed_count, freed_times,
                alloc_count - freed_count,
                alloc_times - freed_times
            );
        if(alloc_count != freed_count || alloc_times != freed_times) {
            error("Memory leak detected!\n" + __str);
        } else {
            normal("No memory leak is found!\n" + __str);
        }
    }
};

/* Debug allocator (It will leak only a size of 16 bytes). */
class allocator_debugger {
  private:
    allocator_debug_helper *__obj;
  public:

    allocator_debugger() noexcept {
        __obj = allocator_debug_helper::get_object();
    }
    ~allocator_debugger() noexcept(false) {
        if(!--__obj->refer_count) delete __obj;
    }
    void *allocate(size_t __n) {
        return __obj->allocate(::std::malloc(__obj->real_size(__n)),__n);
    }
    void deallocate(void *__ptr) noexcept {
        ::std::free(__obj->deallocate(__ptr));
    }
};

inline void *malloc(size_t __n) noexcept {
    static allocator_debugger __obj;
    return __obj.allocate(__n);
}

inline void free(void *__ptr) noexcept {
    static allocator_debugger __obj;
    return __obj.deallocate(__ptr);
}

/* A special class for debug use. */
template <class _Tp>
struct leaker {
    _Tp *ptr;

    leaker() : ptr((_Tp*) ::dark::malloc(sizeof(_Tp)))
    { ::dark::construct(ptr); }

    template <class _Up>
    requires std::constructible_from <_Tp, _Up> &&
        (!std::is_same_v <_Tp, std::decay_t <_Up>>)
    leaker(_Up &&__val) : ptr((_Tp*) ::dark::malloc(sizeof(_Tp))) {
        ::dark::construct(ptr,std::forward <_Up> (__val));
    }
    template <class ..._Args>
    requires std::constructible_from <_Tp, _Args...>
    leaker (_Args &&...__args) : ptr((_Tp*) ::dark::malloc(sizeof(_Tp))) {
        ::dark::construct(ptr,std::forward <_Args>(__args)...);
    }

    leaker(const leaker &__rhs) : leaker() {
        *ptr = *__rhs.ptr;
    }
    leaker(leaker &&__rhs) noexcept : ptr(__rhs.ptr) {
        __rhs.ptr = nullptr;
    }

    leaker &operator = (const leaker &__rhs) {
        *ptr = *__rhs.ptr;
        return *this;
    }
    leaker &operator = (leaker &&__rhs) noexcept {
        reset();
        ptr = __rhs.ptr;
        __rhs.ptr = nullptr;
        return *this;
    }

    void reset() noexcept { this->~leaker(); ptr = nullptr; }
    ~leaker() noexcept { if (ptr) { destroy(ptr); ::dark::free(ptr); } }

    bool operator == (const leaker &__rhs) const {
        return (!ptr && !__rhs.ptr) || (*ptr == *__rhs.ptr);
    }

    auto operator <=> (const leaker &__rhs) const {
        return *ptr <=> *__rhs.ptr;
    }
};

#else

using ::std::malloc;
using ::std::free;

#endif


/* A simple allocator. */
template <class T>
struct allocator {
    inline static constexpr size_t __N = sizeof(T);

    template <class U>
    struct rebind { using other = allocator<U>; };

    using size_type         = size_t;
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T *;
    using reference         = T &;
    using const_pointer     = const T*;
    using const_reference   = const T&;

    static T *allocate(size_t __n) { return static_cast <T *> (::dark::malloc(__n * __N)); }
    static void deallocate(T *__ptr, [[maybe_unused]] size_t __n) noexcept { ::dark::free(__ptr); }
};



} // namespace dark
