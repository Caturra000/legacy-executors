#if defined(__linux__) && defined(__GNUC__)

#include <ranges>
#include <iostream>
#include <queue>
#include "co.hpp"
#include "Co_executor.hpp"

int main() {
    auto ex = bsio::require(Co_executor{}, bsio::execution::mapping.other);

    Looper looper(ex);

    auto fibonacci = ex.execute([&](size_t n) {
        constexpr size_t shift = 2;
        constexpr size_t mask = 3;
        size_t table[2 + shift] = {0, 1};
        for(auto i : std::views::iota(shift, n + shift)) {
            table[i & mask] = table[i-1 & mask] + table[i-2 & mask];
            std::cout << "[fibonacci]-" << i-shift << ":\t" << table[i & mask] << std::endl;
            looper.yield();
        }
    }, 7);

    auto stirling = ex.execute([&] {
        constexpr size_t table[] = {
            1, 3, 13, 75, 541, 4683, 47293, 545835, 7087261
        };
        for(auto i : std::views::iota(size_t{0}, std::size(table))) {
            std::cout << "[stirling]-" << i << ":\t" << table[i] << std::endl;
            looper.yield();
        }
    });

    return 0;
}

#else // defined(__linux__) && defined(__GNUC__)

int main() {
    static_assert(false, "The platform is not supported");
    return 1;
}

#endif