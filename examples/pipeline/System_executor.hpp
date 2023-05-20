#pragma once
#include <thread>
#include "execution.hpp"
#include "property.hpp"

struct System_executor {
    template <typename F>
    void execute(F f) {
        if(!inplace) {
            std::thread t {std::move(f)};
            synchronized ? t.join() : t.detach();
        } else {
            f();
        }
    }

    bool inplace;
    bool synchronized;

    auto query(bsio::execution::Blocking::Never) { return !inplace && !synchronized; }
    auto query(bsio::execution::Blocking::Possibly) { return !inplace && synchronized; }
    auto query(bsio::execution::Blocking::Always) { return inplace; }
    static constexpr auto require(bsio::execution::Blocking::Never) { return System_executor{false, false}; }
    static constexpr auto require(bsio::execution::Blocking::Possibly) { return System_executor{false, true}; }
    static constexpr auto require(bsio::execution::Blocking::Always) { return System_executor{true}; }
};
