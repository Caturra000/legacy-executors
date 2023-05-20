#pragma once
#include <type_traits>
#include <functional>
#include <tuple>

template <typename T>
struct Function_traits;

template <typename T>
struct Function_traits<T&>: public Function_traits<T> {};

template <typename T>
struct Function_traits<T&&>: public Function_traits<T> {};

template <typename Ret, typename ...Args>
struct Function_traits<Ret(Args...)> {
    constexpr static size_t args_size = sizeof...(Args);
    using Return_type = Ret;
    using Args_tuple = std::tuple<Args...>;
};

template <typename Ret, typename ...Args>
struct Function_traits<Ret(*)(Args...)>: public Function_traits<Ret(Args...)> {};

template <typename Ret, typename ...Args>
struct Function_traits<std::function<Ret(Args...)>>: public Function_traits<Ret(Args...)> {};

template <typename Ret, typename C, typename ...Args>
struct Function_traits<Ret(C::*)(Args...)>: public Function_traits<Ret(Args...)> {};

template <typename Ret, typename C, typename ...Args>
struct Function_traits<Ret(C::*)(Args...) const>: public Function_traits<Ret(Args...)> {};

template <typename Callable>
struct Function_traits: public Function_traits<decltype(&Callable::operator())> {};
