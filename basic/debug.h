#pragma once

#include <iostream>

namespace dark {


struct error {
    std::string data;
    explicit error(error *) {}

    explicit error(std::string __s) : data(std::move(__s))
    { std::cerr << "\n\033[31mFatal error: " << data << "\n\033[0m"; }

    const char *what() const noexcept { return data.c_str(); }
};


struct warning {
    std::string data;
    explicit warning(warning *) {}

    warning(std::string __s) {
        std::cerr << "\033[33mWarning: " << __s << "\n\033[0m";
    }

    const char *what() const noexcept { return data.c_str(); }
};



}