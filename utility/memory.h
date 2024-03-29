#pragma once
#include "basic.h"
#include <cstdlib>


namespace dark {

#ifdef _DEBUG


#else // No debug mode.

void *malloc(size_t __size) {
    void *ptr = std::malloc(__size);
    if (!ptr) panic("malloc: Bad allocation.");
    return ptr;
}

void *calloc(size_t __count, size_t __size) {
    void *ptr = std::calloc(__count, __size);
    if (!ptr) panic("calloc: Bad allocation.");
    return ptr;
}

void *realloc(void *__ptr, size_t __size) {
    void *ptr = std::realloc(__ptr, __size);
    if (!ptr) panic("realloc: Bad allocation.");
    return ptr;
}

void free(void *__ptr) { std::free(__ptr); }

#endif


} // namespace dark
