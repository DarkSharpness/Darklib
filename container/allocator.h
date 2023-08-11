#pragma once

#include "../basic/debug.h"

#include <cstdlib>
#include <type_traits>


namespace dark {

#ifdef _DEBUG

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

    inline static allocator_debug_helper *single = nullptr;
    inline static constexpr size_t offset = sizeof(debug_pack);

    allocator_debug_helper() = delete;
    allocator_debug_helper(std::nullptr_t) noexcept {
        std::cerr << "\033[32mDebug allocator is enabled!\n\033[0m";
    }

    void *allocate(void *__ptr,size_t __n) {
        if(!__ptr) return nullptr;
        alloc_count += __n;
        debug_pack *__tmp = static_cast <debug_pack *> (__ptr);
        __tmp->pointer = this;
        __tmp->count   = __n;
        return __tmp + 1;
    }

    void *deallocate(void *__ptr) {
        if(!__ptr) return nullptr;
        debug_pack *__tmp = static_cast <debug_pack *> (__ptr) - 1;
        if(__tmp->pointer != this) throw error("Deallocate Mismatch!");
        freed_count += __tmp->count;
        return __tmp;
    }

    static allocator_debug_helper *get_object() {
        if(single == nullptr) single = new allocator_debug_helper(nullptr);
        ++single->refer_count;
        return single;
    }

    ~allocator_debug_helper() noexcept(false) {
        if(alloc_count != freed_count)
            throw error("Memory leak! " + std::to_string(alloc_count - freed_count) + " bytes leaked!");
        std::cerr << "\n\033[32mNo memory leak is found!\n\033[0m";
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
    void *allocate(size_t __n) noexcept {
        return __obj->allocate(::malloc(__n + __obj->offset),__n);
    }

    void deallocate(void *__ptr) noexcept {
        ::free(__obj->deallocate(__ptr));
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

#else

using ::malloc;
using ::free;

#endif


/* Trivial allocator. */
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

    [[__gnu__::__always_inline__]] static T *allocate() {
        return static_cast <T *> (dark::malloc(__N));
    }
    static T *allocate(size_t __n) {
        return static_cast <T *> (malloc(__n * __N));
    }
    static void deallocate(T *__ptr) { dark::free(__ptr); }
    static void deallocate(T *__ptr,size_t __n) { free(__ptr); }
};


template <class T>
requires std::is_copy_constructible_v <T>
inline void construct(T *__ptr, const T &__val) 
noexcept(noexcept(::new(__ptr) T(__val)))
{ ::new(__ptr) T(__val); }


template <class T>
requires std::is_move_constructible_v <T>
inline void construct(T *__ptr, T &&__val)
noexcept(noexcept(::new(__ptr) T(std::move(__val))))
{ ::new (__ptr) T(std::move(__val)); }


template <class T,class ...Args>
requires std::is_constructible_v <T, Args...>
inline void construct_forward(T *__ptr, Args &&...__val)
{ ::new (__ptr) T(std::forward <Args>(__val)...); }


template <class T>
requires std::is_trivially_destructible_v <T>
inline void destroy(T *) noexcept {}

template <class T>
requires (!std::is_trivially_destructible_v <T>)
inline void destroy(T *__ptr) noexcept { __ptr->~T(); }

}