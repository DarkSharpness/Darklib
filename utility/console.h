#include <iostream>
#include <format>

namespace dark::console {

/* Color enumeration. */
enum class Color : int {
    RED     = 31,
    GREEN   = 32,
    YELLOW  = 33,
    BLUE    = 34,
    MAGENTA = 35,
    CYAN    = 36,
    WHITE   = 37,
};

/* Atomic colored print function. */
inline static void print(std::string_view __msg, Color __color) {
    std::cerr << std::format("\033[1;{}m{}\033[0m\n",
        static_cast <int> (__color), __msg);
}

} // namespace dark::console
