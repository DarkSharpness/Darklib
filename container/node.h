#pragma once
#include "basic.h"

namespace dark {

namespace __detail::__node {

/* Node containing value. */
template <typename _Tp, typename _Node, typename _Tag = void>
struct value_node : _Node { using _Node_t = _Node; using _Tag_t = _Tag; _Tp value; };


} // namespace __detail::__node

} // namespace dark
