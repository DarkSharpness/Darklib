#pragma once
// Default to use DBG LIB.
#ifdef _DEBUG
#ifndef _USEDBG
#define _USEDBG
#endif
#endif

#ifdef _USEDBG
#include "console.h"
#include <source_location>

namespace dark::debug {

namespace __detail::__debug {

template <console::Color _Color>
struct printer : std::exception {
    std::string message;
    explicit printer(std::string __msg)
    noexcept : message(std::move(__msg)) {}
    const char *what() const noexcept override { return message.c_str(); }
    const void print() const { return console::print(message, _Color); }
};

} // namespace __detail::__debug

using error     = __detail::__debug::printer <console::Color::RED>;
using warning   = __detail::__debug::printer <console::Color::YELLOW>;
using normal    = __detail::__debug::printer <console::Color::GREEN>;

[[noreturn]] inline void panic_handler(
    std::string_view __msg, std::source_location __loc) {
    error { std::format("Panic at {}:{}:{}: {}",
        __loc.file_name(),
        __loc.line(),
        __loc.column(), __msg) }.print();
    std::terminate();
}

} // namespace dark::debug

#ifndef panic_handler
/**
 * @brief Custom panic handler.
 * It will print out where the panic happened.
 */
#define panic_handler(x,y) do { ::dark::debug::panic_handler(x,y); } while (0)
#endif // panic_handler

#endif // _USEDBG

#ifndef panic_handler
// Define the default panic handler.
#define panic_handler(x,y) __builtin_unreachable()
#endif // panic_handler

#ifndef panic
/**
 * @brief Define the panic macro.
 * @note By default, this macro will call the panic_handler macro.
 * 
 * If user does not define the panic_handler, the panic macro
 * will expand to __builtin_unreachable() in release mode.
 * It will turn to default panic_handler in debug mode.
 * 
 * If you just want to use the feature of debug library,
 * but don't want to use the debug-mode panic_handler, you have 2 options:
 * 
 * 1. Define your own panic_handler, place it in the top of your code.
 * 2. Define _USEDBG to manually enable debug helper.
 * 
 * @attention This function will never return!
 */
#define panic(x) panic_handler(x, std::source_location::current())
#endif // panic
