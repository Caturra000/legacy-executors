#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <cassert>
#include <array>
#include "execution.hpp"
#include "property.hpp"
#include "priority.hpp"

int main() {
    Priority_execution_context context;
    auto ex = context.executor();
    // Lowest priority
    auto last_step_ex = bsio::require(ex, Priority{low_priority.value()-1});

    std::atomic<size_t> seq {0};

    // Higher priority has a smaller value

    std::atomic<size_t> low_seq_sum {0};
    std::atomic<size_t> normal_seq_sum {0};
    std::atomic<size_t> high_seq_sum {0};

    std::vector<std::thread> threads;

    constexpr size_t num_executor = 3;
    // Worker threads
    constexpr size_t num_worker = 10;
    // Background threads
    constexpr size_t num_bg = 2;
    constexpr size_t num_task = num_executor * num_worker * 1e5;

    auto update_seq_sum = [&seq](std::atomic<size_t> &ex_seq) {
        auto val = seq.fetch_add(1, std::memory_order_acq_rel);
        ex_seq.fetch_add(val, std::memory_order_acq_rel);
    };

    std::thread background[num_bg];
    for(auto &&bg : background) {
        bg = std::thread {&Priority_execution_context::run, &context};
    }

    auto task = [&, ex, ptr = std::addressof(context)] {
        auto low_ex = bsio::require(ex, low_priority);
        auto normal_ex = bsio::require(ex, normal_priority);
        auto high_ex = bsio::require(ex, high_priority);
        assert(bsio::query(high_ex, bsio::execution::context) == ptr);

        for(auto _ = num_task / num_executor / num_worker; _--;) {
            low_ex.execute([&] { update_seq_sum(low_seq_sum); });
            normal_ex.execute([&] { update_seq_sum(normal_seq_sum); });
            high_ex.execute([&] { update_seq_sum(high_seq_sum); });
        }
    };
    for(size_t thread_idx = 0; thread_idx < num_worker; thread_idx++) {
        std::thread {task}.detach();
    }

    last_step_ex.execute([&] {
        // Stop background thread(s)
        context.stop();
        constexpr double num_executor_task = num_task * num_executor;
        auto low_seq_avg = low_seq_sum / num_executor_task;
        auto normal_seq_avg = normal_seq_sum / num_executor_task;
        auto high_seq_avg = high_seq_sum / num_executor_task;
        std::cout << std::fixed;
        std::cout << "low-avg: \t" << low_seq_avg << std::endl;
        std::cout << "normal-avg: \t" << normal_seq_avg << std::endl;
        std::cout << "high-avg: \t" << high_seq_avg << std::endl;
        assert(high_seq_avg < normal_seq_avg);
        assert(normal_seq_avg < low_seq_avg);
        assert(seq == num_task);
    });

    for(auto &&bg : background) bg.join();
}
