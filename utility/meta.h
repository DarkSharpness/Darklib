#pragma once
#include <string_view>
#include <source_location>

namespace dark::meta {

namespace __detail::__meta {

template <auto _Val>
consteval auto func_info() {
    return std::string_view {
        std::source_location::current().function_name()
    };
}

template <class _Tp>
consteval auto func_info() {
    return std::string_view {
        std::source_location::current().function_name()
    };
}

consteval auto cstrlen(const char *ptr) {
    const char *end = ptr;
    while (*end) ++end;
    return end - ptr;
}

consteval auto get_type_blank() {
    struct info { std::size_t left_omit, right_omit; };
    const auto result = func_info <void>();
    const auto l = result.find("void");
    const auto r = result.size() - l - cstrlen("void");
    return info { l , r };
}

consteval auto get_value_blank() {
    struct info { std::size_t left_omit, right_omit; };
    const auto result = func_info <nullptr> ();
    const auto l = result.find("nullptr");
    const auto r = result.size() - l - cstrlen("nullptr");
    return info { l , r };
}

} // namespace __detail::__meta

template <auto _Val>
constexpr auto value_string() {
    const auto func = __detail::__meta::func_info <_Val> ();
    const auto [l , r] = __detail::__meta::get_value_blank();
    auto name = func.substr(l); name.remove_suffix(r);
    return name;
}

template <class _Tp>
constexpr auto type_string() {
    const auto func = __detail::__meta::func_info <_Tp> ();
    const auto [l , r] = __detail::__meta::get_type_blank();
    auto name = func.substr(l); name.remove_suffix(r);
    return name;
}

/* Name of the type in constexpr context. */
template <auto _Val>
constexpr auto nameof() { return value_string <_Val> (); }

/* Name of the type in constexpr context. */
template <class _Tp>
constexpr auto nameof() { return type_string <_Tp> (); }

/* A function-programming style class, remove all scopes.  */
inline constexpr static struct remove_scope_t {} remove_scope;

/* We utilize the fact that npos == -1 */
static_assert(std::string_view::npos + 1 == 0);

constexpr auto operator | (remove_scope_t, std::string_view name) {
    const auto pos = name.find_last_of(':');
    return name.substr(pos + 1);
}

constexpr auto operator | (std::string_view name, remove_scope_t) {
    const auto pos = name.find_last_of(':');
    return name.substr(pos + 1);
}


} // namespace dark
