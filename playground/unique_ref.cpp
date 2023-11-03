#include <iostream>
#include <memory>

namespace play {

template <class _Tp, class _Up>
concept assignable_from = requires (_Tp &__lhs, _Up &&__rhs) { __lhs = std::forward <_Up> (__rhs); };

template <class _Tp, class _Deleter = std::default_delete <_Tp>>
struct unique_ref {
    [[no_unique_address]] _Deleter deleter;
    _Tp *data;

    operator _Tp  &()   const noexcept { return *data; }
    _Tp *operator &()   const noexcept { return  data; }
    _Tp &operator ()()  const noexcept { return *data; }
    _Tp &get()          const noexcept { return *data; }

    unique_ref() noexcept = delete;
    explicit unique_ref(_Tp *__ptr) noexcept : deleter{}, data(__ptr) {}

    unique_ref(const unique_ref &) = delete;
    unique_ref &operator =(const unique_ref &) = delete;

    unique_ref(unique_ref &&rhs)
    noexcept : deleter(std::move(rhs.deleter)), data(rhs.data) { rhs.data = nullptr; }

    unique_ref &operator = (unique_ref &&rhs) noexcept {
        if (this != std::addressof(rhs)) {
            deleter(data);
            data     = rhs.data;
            deleter  = std::move(rhs.deleter);
            rhs.data = nullptr;
        } return *this;
    }

    template <class _Up>
    requires assignable_from <_Tp, _Up>
    unique_ref &operator = (_Up &&__x) noexcept {
        *data = std::forward <_Up> (__x); return *this;
    }

    void reset(_Tp *__ptr) noexcept { deleter(data); data = __ptr; }

    std::unique_ptr <_Tp, _Deleter> release() noexcept {
        auto *__temp = data; data = nullptr;
        return std::unique_ptr <_Tp, _Deleter> (__temp, deleter);
    }

    void swap(unique_ref &rhs) noexcept {
        std::swap(deleter, rhs.deleter);
        std::swap(data, rhs.data);
    }

    ~unique_ref() noexcept { deleter(data); }
};


template <class _Tp,class ..._Up>
requires std::is_constructible_v <_Tp,_Up...>
unique_ref <_Tp> make_ref(_Up &&...args) noexcept {
    return unique_ref { new _Tp(std::forward <_Up> (args)...) };
}

}

void func(const std::string &str) {
    std::cout << str << '\n';
}

struct Base {
    virtual void foo() = 0;
    virtual ~Base() = default;
};

struct Derived : Base {
    void foo() override { std::cout << "Derived::foo()\n"; }
    ~Derived() override { std::cout << "~Derived()\n"; }
};


signed main() {
    play::unique_ref <std::string> str(new std::string(10,'a'));
    func(str);
    str = "Hello World!";
    str.get().pop_back();
    func(str);
    str = play::make_ref <std::string> (10,'b');
    func(str);
    str.release();

    play::unique_ref <Base> base(new Derived);
    base().foo();

    return 0;
}
