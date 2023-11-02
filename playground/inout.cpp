#include <iostream>
#include <tuple>
#include <format>
#include "../basic/libc.h"

namespace play {

template <size_t _Nm = 0,class _Tp, class ..._Up>
void read_tuple(std::tuple <_Tp,_Up...> &__t) {
    if constexpr (_Nm > sizeof...(_Up)) return;
    else {
        std::cin >> std::get <_Nm> (__t);
        return read_tuple <_Nm + 1> (__t);
    }
}

template <class _Tp, class ..._Up>
std::tuple <_Tp,_Up...> read() {
    std::tuple <_Tp,_Up...> __t;
    read_tuple(__t);
    return __t;
}

template <std::size_t _Nm>
consteval std::array <char,_Nm * 3> __get_fmt () {
    std::array <char,_Nm * 3> __ret {};
    char *__ptr = __ret.data();
    std::size_t __cnt = _Nm;
    while(__cnt --> 0) {
        *__ptr++ = '{';
        *__ptr++ = '}';
        *__ptr++ = ' ';
    }
    *(__ptr - 1) = '\0';
    return __ret;
}


template <class _Fill>
struct fill_type {
    const _Fill &data;
    fill_type(const _Fill &__data) : data(__data) {}
};

template <class _Fill>
fill_type(_Fill) -> fill_type <std::decay_t <_Fill>>;

template <class ..._Tp>
void print (const _Tp &...__args) {
    static constexpr auto __fmt = __get_fmt <sizeof...(_Tp)> ();
    std::cout << std::format(__fmt.data() , __args...);
}

}

signed main() {
    auto [x,y] = play::read <int,int> ();
    play::print(x + y,"123");
    play::fill_type __fill {1};
    return 0;
}
