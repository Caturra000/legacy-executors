#pragma once

inline auto Then_execution_context::Then_executor_impl::then_execute(std::invocable auto &&f) {
    using R = typename fluent::FunctionTraits<decltype(f)>::ReturnType;
    fluent::Promise<R> promise(_context);
    auto future = promise.get();
    assert(bsio::query(_context->executor(), bsio::execution::directionality.oneway));
    auto ex = bsio::require(_context->executor(), bsio::execution::blocking.possibly);
    ex.execute([f = std::move(f), p = std::move(promise)]() mutable {
        p.setValue(f(), true);
    });
    return future;
}
