#include <functional>
#include <any>
#include <mutex>
#include "execution.hpp"

struct LF {
public:
    LF(size_t threads): _pool(threads) {}

public:
    void on_accept(auto f) {
        _accpet = std::move(f);
    }

    void on_process(auto f) {
        _process = std::move(f);
    }

    void stop() { _pool.stop(); }
    void wait() { _pool.wait(); }

public:
    void start() {
        std::call_once(_first_check, [this] {
            if(!_accpet || !_process) {
                throw std::logic_error("You should first register callbacks");
            }
        });

        start_leader();
    }

private:
    void start_leader() {
        auto ex = bsio::require(_pool.executor(),
                    bsio::execution::blocking.never);
        // Leader thread
        ex.execute([this, ex]() mutable {
            std::any ret = _accpet();
            // Promote a follower thread
            // to become the new leader
            ex.execute([this] {start_leader();});
            _process(std::move(ret));
        });
    }

private:
    // Followers
    bsio::Static_thread_pool _pool;
    // Wait for event
    std::function<std::any()> _accpet;
    // Long-running task
    std::function<void(std::any)> _process;

    std::once_flag _first_check;
};
