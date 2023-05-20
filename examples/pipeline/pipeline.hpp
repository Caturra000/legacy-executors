#pragma once
#include <future>
#include <tuple>
#include <type_traits>
#include "Function_tratis.hpp"
#include "queue.hpp"
#include "System_executor.hpp"

// Final stage in a pipeline
template <typename T, typename F>
std::future<void> pipeline(In<T> in, F &&f) {
    using Value_type = typename std::tuple_element_t<0, typename Function_traits<F>::Args_tuple>::Value_type;

    std::packaged_task<void()> task {
        [in, f = std::move(f)]() mutable { f(in); }
    };

    std::future<void> future = task.get_future();

    auto ex = [] {
        System_executor ex1;
        // In this property, user can wait for EOF without std::future<>.wait()
        // auto ex2 = bsio::require(ex1, bsio::execution::blocking.possibly);
        auto ex2 = bsio::require(ex1, bsio::execution::blocking.never);
        assert(bsio::query(ex2, bsio::execution::blocking.never));
        return ex2;
    } ();
    ex.execute(std::move(task));

    return future;
}

// Intermediate stage in a pipeline
template <typename T, typename F, typename ...Tail>
std::future<void> pipeline(In<T> in, F &&f, Tail &&...tail) {
    using Value_type = typename std::tuple_element_t<0, typename Function_traits<F>::Args_tuple>::Value_type;

    auto shared_queue = std::make_shared<Queue<Value_type>>();
    In<Value_type> next_in{shared_queue};
    Out<Value_type> out{shared_queue};

    auto ex = System_executor::require(bsio::execution::blocking.never);
    ex.execute([in, out, f = std::move(f)]() mutable {
        f(in, out);
        out.stop();
    });

    return pipeline(next_in, std::forward<Tail>(tail)...);
}

// First stage in a pipeline
template <typename F, typename ...Tail>
std::future<void> pipeline(F &&f, Tail &&...tail) {
    using First_arguemnt_args_tuple = typename Function_traits<F>::Args_tuple;
    using Value_type = typename std::tuple_element_t<0, First_arguemnt_args_tuple>::Value_type;

    auto shared_queue = std::make_shared<Queue<Value_type>>();
    In<Value_type> next_in{shared_queue};
    Out<Value_type> out{shared_queue};

    auto ex = System_executor::require(bsio::execution::blocking.never);
    ex.execute([out, f = std::move(f)]() mutable {
        f(out);
        // Stop blocking pop() request
        out.stop();
    });

    return pipeline(next_in, std::forward<Tail>(tail)...);
}