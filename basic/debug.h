#pragma once

#include <string>
#include <format>
#include <iostream>

namespace dark {


/* Error code. */
struct error {
    std::string data;
    explicit error(std::string __s) noexcept : data(std::move(__s))
    { std::cerr << "\n\033[31mFatal error: " << data << "\n\033[0m"; }
    const char *what() const noexcept { return data.c_str(); }
};


/* Warning code. */
struct warning {
    std::string data;
    explicit warning(std::string __s) noexcept : data(std::move(__s))
    { std::cerr << "\033[33mWarning: " << data << "\n\033[0m"; }
    const char *what() const noexcept { return data.c_str(); }
};


/* Normal code. */
struct normal {
    std::string data;
    explicit normal(std::string __s) noexcept : data(std::move(__s))
    { std::cerr << "\033[32m" << data << "\n\033[0m"; }
    const char *what() const noexcept { return data.c_str(); }
};



}
