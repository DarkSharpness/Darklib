#include <iostream>
#include <tuple>
#include <format>

namespace play {

template <size_t _Nm = 0,class ..._Tp>
void read_tuple(std::tuple <_Tp...> &__t) {
    if constexpr (_Nm >= sizeof...(_Tp)) return;
    else {
        std::cin >> std::get <_Nm> (__t);
        return read_tuple <_Nm + 1> (__t);
    }
}

template <class ..._Tp>
std::tuple <_Tp...> read() {
    std::tuple <_Tp...> __t;
    read_tuple(__t);
    return __t;
}

template <std::size_t _Nm>
consteval std::array <char,_Nm * 3> __get_fmt () {
    std::array <char,_Nm * 3> __ret {};
    char *__ptr = __ret.data();
    std::size_t __cnt = _Nm;
    while(__cnt --> 0) { *__ptr++ = '{'; *__ptr++ = '}'; *__ptr++ = ' '; }
    *(__ptr - 1) = '\0';
    return __ret;
}

template <std::size_t _Nm>
consteval std::array <char,_Nm * 3 + 1> __get_fmtln () {
    std::array <char,_Nm * 3 + 1> __ret {};
    char *__ptr = __ret.data();
    std::size_t __cnt = _Nm;
    while(__cnt --> 0) { *__ptr++ = '{'; *__ptr++ = '}'; *__ptr++ = ' '; }
    *__ptr-- = '\0'; *__ptr = '\n'; 
    return __ret;
}

template <class ..._Tp>
void print (const _Tp &...__args) {
    static constexpr auto __fmt = __get_fmt <sizeof...(_Tp)> ();
    std::cout << std::format(__fmt.data() , __args...);
}

template <class ..._Tp>
void println (const _Tp &...__args) {
    static constexpr auto __fmt = __get_fmtln <sizeof...(_Tp)> ();
    std::cout << std::format(__fmt.data() , __args...);
}

}

signed main() {
    auto [x,y] = play::read <long,int> ();
    play::println(x + y,"test1");
    play::println(x - y,"test2");
    return 0;
}
