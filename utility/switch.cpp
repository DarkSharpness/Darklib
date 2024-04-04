// #undef _DEBUG
#include "switch.h"
#include <array>
#include <iostream>

void demo0();
void demo1();

signed main() {
    demo0();
    demo1();
    return 0;
}

void demo0() {
    constexpr auto array = std::array {
        "hello", "world", "dark", "sharpness",
    };

    // Automatically pick a suitable hash.
    // If fails, panic.
    constexpr auto h = dark::make_switch(array);

    // This is another way to make a string switch.
    // Do not worry that you input a sequence (e.g. "Hell0")
    // which does not match those strings in switch statement.
    // It will fail at compile time in switch if conflict.
    [[maybe_unused]]
    constexpr auto g = dark::make_switch("hello", "world", "dark", "sharpness");

    std::string str = "world";
    switch (h(str)) {
        case h(array[0]): // hello
            std::cout << "Hello, world!" << std::endl;
            break;
        case h(array[1]): // world
            std::cout << "World, hello!" << std::endl;
            break;
        default:
            std::cout << "Unknown input!" << std::endl;
    }
}

void demo1() {
    // Make sure the hash is unique by yourself.
    // e.g. if you input -1, this will not compile.
    constexpr auto h = dark::switch_string(79);

    std::string str = "wtf";
    switch (h(str)) {
        case h("hello"):
            std::cout << "Hello, world!" << std::endl;
            break;
        case h("world"):
            std::cout << "World, hello!" << std::endl;
            break;
        default:
            std::cout << "Unknown input!" << std::endl;
    }
}
