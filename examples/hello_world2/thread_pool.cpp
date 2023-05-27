#include "execution.hpp"
#include "property.hpp"
#include <iostream>
#include <atomic>
std::atomic<int> sum {0};
void cal(int i) {
    sum.fetch_add(i, std::memory_order_relaxed);
}

int main() {
    bsio::Static_thread_pool pool(std::thread::hardware_concurrency());
    constexpr auto blocking = bsio::execution::blocking;
    constexpr auto relationship = bsio::execution::relationship;

    auto ex1 = pool.executor();
    auto ex2 = ex1.require(blocking.never);
    auto ex3 = ex2.require(relationship.continuation);
    auto ex4 = ex1.require(blocking.always);

    static_assert(ex2.query(blocking.never), "executor-2 must have a never-block property");
    static_assert(ex3.query(relationship.continuation));

    constexpr static size_t TEST_ROUND = 100000;

    for(size_t i = 0; i < TEST_ROUND; ++i) {
        ex1.execute([=] {cal(1);});
        ex2.execute([=] {cal(1);});
        ex3.execute([=] {cal(1);});
    }

    std::function<void()> recursive = [&, ex = ex3, r = std::make_shared<std::atomic<size_t>>()]() mutable {
        size_t v = r->fetch_add(1, std::memory_order_relaxed);
        if(v < TEST_ROUND) {
            sum.fetch_add(1, std::memory_order_relaxed);
            ex.execute(std::ref(recursive));
            // Greedy multishot
            if(v * v < TEST_ROUND) {
                ex.execute(std::ref(recursive));
            }
        }
    };

    ex4.execute(recursive);

    pool.wait();

    std::cout << sum.load() << std::endl;

    return 0;
}