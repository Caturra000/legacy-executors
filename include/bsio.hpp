#pragma once
#include "execution.hpp"
#include "property.hpp"

namespace bsio {

// asio-like interfaces
// TODO executor concept:   bsio::Executor auto &&ex

inline auto dispatch(auto &&ex, std::invocable auto &&f) {
    return bsio::require(std::forward<decltype(ex)>(ex), execution::blocking.never)
        .execute(std::forward<decltype(f)>(f));
}

inline auto post(auto &&ex, std::invocable auto &&f) {
    return bsio::require(std::forward<decltype(ex)>(ex), execution::blocking.possibly)
        .execute(std::forward<decltype(f)>(f));
}

inline auto defer(auto &&ex, std::invocable auto &&f) {
    return bsio::prefer(
            bsio::require(std::forward<decltype(ex)>(ex),
                execution::blocking.never),
            execution::relationship.continuation)
        .execute(std::forward<decltype(f)>(f));
}

} // namespace bsio
