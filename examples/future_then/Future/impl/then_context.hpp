#pragma once
#include "execution.hpp"

struct Then_execution_context: bsio::Static_thread_pool {
    explicit Then_execution_context(size_t threads): bsio::Static_thread_pool(threads) {}

    struct Then_executor_impl {
        constexpr auto require(bsio::execution::Directionality::Then) const { return Then_executor_impl{_context}; }
        constexpr auto prefer(auto) const { return Then_executor_impl{_context}; }
        static constexpr bool query(bsio::execution::Directionality::Then) { return true; }
        static constexpr auto query(bsio::execution::Directionality) { return bsio::execution::directionality.then; }
        Then_execution_context* query(bsio::execution::Context) { return _context; }
        const Then_execution_context* query(bsio::execution::Context) const { return _context; }

        auto then_execute(std::invocable auto &&f);

        Then_execution_context *_context;
    };

    using Then_executor_type = Then_executor_impl;

    Then_executor_type then_executor() {
        return Then_executor_type{this};
    }
};