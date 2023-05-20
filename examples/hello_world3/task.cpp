#include <iostream>
#include "bsio.hpp"

int main() {
    static size_t recursive = 10;
    bsio::Static_thread_pool pool(2);
    bsio::dispatch(pool.executor(), [] {std::cout<<"x\n";});
    bsio::post(pool.executor(), [] {std::cout<<"xx\n";});
    bsio::defer(pool.executor(), [] {std::cout<<"xxx\n";});
    pool.wait();
    if(recursive--) {
        main();
    }
    return 0;
}
