#pragma once
#include <version>
#include <type_traits>

namespace dark {

using size_t    = std::size_t;
using ssize_t   = std::make_signed_t<size_t>;
using ptrdiff_t = std::ptrdiff_t;

} // namespace dark
