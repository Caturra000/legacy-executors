#include <iostream>
#include "execution.hpp"
#include "property.hpp"

struct Double_executor {
    void execute(std::invocable auto &&func) { func(); func(); }

    auto operator<=>(const Double_executor &) const = default;
};

int main() {
    using namespace bsio::execution;
    using Executor = Polymorphic_executor<Directionality::Oneway>;
    Executor ex;

    bsio::Static_thread_pool pool(1);    
    ex = pool.executor();
    ex.execute([] { std::cout << "foo\n"; });

    ex = Double_executor{};
    ex.execute([] { std::cout << "bar\n"; });

    pool.wait();
    return 0;
}