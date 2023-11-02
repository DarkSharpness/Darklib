#include <iostream>
#include <memory>

namespace play {

template <class _Tp, class _Deleter = std::default_delete <_Tp>>
struct unique_reference {
    [[no_unique_address]] _Deleter deleter;
    _Tp *data;

    operator _Tp  &()   const noexcept { return *data; }
    _Tp *operator &()   const noexcept { return  data; }
    _Tp &get()          const noexcept { return *data; }

    unique_reference() noexcept = delete;
    explicit unique_reference(_Tp *__ptr) noexcept : deleter{}, data(__ptr) {}

    unique_reference(const unique_reference &) = delete;
    unique_reference &operator =(const unique_reference &) = delete;

    unique_reference(unique_reference &&rhs)
    noexcept : deleter(std::move(rhs.deleter)), data(rhs.data) { rhs.data = nullptr; }

    unique_reference &operator = (unique_reference &&rhs) noexcept {
        if (this != std::addressof(rhs)) {
            deleter(data);
            data     = rhs.data;
            deleter  = std::move(rhs.deleter);
            rhs.data = nullptr;
        } return *this;
    }

    template <class _Up>
    requires requires (_Tp &__lhs, _Up &&__rhs) { __lhs = std::forward <_Up> (__rhs); }
    unique_reference &operator = (_Up &&__x) noexcept {
        *data = std::forward <_Up> (__x); return *this;
    }

    void reset(_Tp *__ptr) noexcept { deleter(data); data = __ptr; }

    ~unique_reference() noexcept { deleter(data); }
};

template <class _Tp, class _Deleter = std::default_delete <_Tp>>
unique_reference <_Tp,_Deleter> make_refence(_Tp *__ptr) noexcept {
    return unique_reference <_Tp,_Deleter> (__ptr);
}

}


void func(const std::string &str) {
    std::cout << str << '\n';
}


signed main() {
    play::unique_reference <std::string> str(new std::string(10,'a'));
    func(str);
    str = "Hello World!";
    str.get().pop_back();
    func(str);
    str.reset(new std::string(10,'b'));
    func(str);
    return 0;
}
