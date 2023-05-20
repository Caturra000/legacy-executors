#pragma once
#include <cassert>
#include <functional>
#include <memory>

namespace bsio {
namespace impl {

// TODO void(context)
using Function_signature = void();


// Any invocable function
using Function = 
#if __cplusplus >= 202300L // Well, I guess...
            // Better support for move-only callable objects
            // For example, the object below cannot be a stored target to `std::function`:
            //     std::function<void()> f = [p = std::promise<void>{}]{};
            std::move_only_function<Function_signature>;
#else
            std::function<Function_signature>;
#endif


// Intrusive but RAII-supported template
template <typename Derived> struct Intrusive_list;


// Function with callable object and sibling relationship
// Node may be FIFO or LIFO
struct Function_node;


// Type of list_head, without data field
using Function_intrusive_list = Intrusive_list<Function_node>;


// Type of queue node, with managed and movable resource
using Function_node_handle = std::unique_ptr<Function_node>;


template <typename Derived>
struct Intrusive_list {

    // TODO remove template<>
    // N3974 - Polymorphic Deleter for Unique Pointers
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3974.pdf
    //
    // -> struct Intrusive_list { n3974::unique_ptr<void> _next; };

    std::unique_ptr<Derived> _next;
};


struct Function_node: Function_intrusive_list {
    Function_node() = default;
    Function_node(std::invocable auto func): _func(std::move(func)) {}
    Function_node(std::invocable auto func, Function_intrusive_list next)
        : Intrusive_list{std::move(next)}, _func(std::move(func)) {}

    Function _func;
};


struct Function_node_access {
    bool empty(Function_intrusive_list &list_head) const;

    // Insert new_node to list_head
    void push(Function_intrusive_list &list_head,
              Function_node_handle new_node) const;

    // Merge a full list [new_node_first, ... , new_node_last] to list_head
    // Time Complexity: O(1)
    // Note: new_node_last is NOT a sentinel,
    //       and new_node_last->_next is expected to be nullptr
    void push(Function_intrusive_list &list_head,
              Function_node_handle new_node_first,
              Function_node_handle &new_node_last) const;

    // Note: there must be at least one node in list_head
    auto consume_one(Function_intrusive_list &list_head) const
            -> Function_node_handle;
};

inline bool Function_node_access::
empty(Function_intrusive_list &list_head) const { return !list_head._next; }

// Insert new_node to list_head
inline void Function_node_access::
push(Function_intrusive_list &list_head,
     Function_node_handle new_node) const {
    new_node->_next = std::move(list_head._next);
    list_head._next = std::move(new_node);
}

inline void Function_node_access::
push(Function_intrusive_list &list_head,
     Function_node_handle new_node_first,
     Function_node_handle &new_node_last) const {
    assert(new_node_last);
    new_node_last->_next = std::move(list_head._next);
    list_head._next = std::move(new_node_first);
}

inline auto Function_node_access::
consume_one(Function_intrusive_list &list_head) const
        -> Function_node_handle {
    auto consumed = std::move(list_head._next);
    list_head._next = std::move(consumed->_next);
    return consumed;
}

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

} // namespace impl
} // namespace bsio