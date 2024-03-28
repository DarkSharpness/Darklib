#pragma once
#include "debug.h"
#include <version>
#include <type_traits>

namespace dark {

using size_t    = std::size_t;
using ssize_t   = std::make_signed_t<size_t>;
using ptrdiff_t = std::ptrdiff_t;

/* An unreachable function. */
[[noreturn]] inline void unreachable() { __builtin_unreachable(); }

} // namespace dark
