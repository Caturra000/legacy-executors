#include <concepts>
#include <iostream>
#include <cassert>
#include "execution.hpp"
#include "property.hpp"

using Blocking = bsio::execution::Blocking;
constexpr auto blocking = bsio::execution::blocking;

struct Inline_executor {
    void execute(std::invocable auto &&func) { func(); }

    static constexpr auto require(Blocking::Always) {
        return Inline_executor{};
    }

    constexpr auto prefer(Blocking::Never) {
        return *this;
    }

    constexpr auto query(Blocking::Always) {
        return true;
    }

    constexpr auto query(Blocking) {
        return blocking.always;
    }

    auto operator<=>(const Inline_executor&) const = default;
};

auto require(Inline_executor e, Blocking::Never) {
    throw std::runtime_error("impossible.");
    return nullptr;
}

auto prefer(Inline_executor e, Blocking::Possibly) {
    return e;
}

int main() {
    // call static Inline_executor::require()
    auto ex = bsio::require(Inline_executor{}, blocking.always);

    // call ::prefer()
    auto ex2 = bsio::prefer(ex, blocking.possibly);
    ex2.execute([] { std::cout << "hi!" << std::endl; });

    // call Inline_executor::query
    std::cout << std::boolalpha << bsio::query(ex2, blocking.always) << std::endl;

    static_assert(ex2 == ex);
    static_assert(blocking.always == bsio::query(ex2, blocking));
}
