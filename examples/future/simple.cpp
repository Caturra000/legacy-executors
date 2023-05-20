#include <iostream>
#include <chrono>
#include <cassert>
#include "execution.hpp"

int main() {
    bsio::Static_thread_pool background(1);
    auto executor = bsio::require(background.executor(), bsio::execution::directionality.twoway);
    std::future<void> future = executor.twoway_execute([] {
        std::cout << "Hi" << std::endl;
    });
    future.wait();
    std::cout << "Bye" << std::endl;

    std::future<int> guess_number = executor.twoway_execute([] {
        return 1926 & 817;
    });
    std::cout << guess_number.get() << std::endl;

    return 0;
}
