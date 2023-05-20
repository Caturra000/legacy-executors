#pragma once
#include <thread>
#include <type_traits>
#include <queue>
#include "execution.hpp"
#include "property.hpp"
#include "co.hpp"

struct Co_executor {
    auto execute(auto &&f, auto &&...args) {
        auto &env = co::open();
        auto co = env.createCoroutine(std::forward<decltype(f)>(f),
                    std::forward<decltype(args)>(args)...);
        co->resume();
        return co;
    }

    std::thread::id context() const { return std::this_thread::get_id(); }

    auto query(bsio::execution::Blocking::Never) { return true; }
    auto query(bsio::execution::Mapping::Other) { return true; }
    static constexpr auto require(bsio::execution::Blocking::Never) { return Co_executor{}; }
    static constexpr auto prefer(bsio::execution::Blocking::Possibly) { return Co_executor{}; }
    static constexpr auto prefer(bsio::execution::Blocking::Always) { return Co_executor{}; }
    static constexpr auto require(bsio::execution::Mapping::Other) { return Co_executor{}; }
};

struct Looper {
public:
    Looper(Co_executor executor): _executor(executor) {}

    ~Looper() {
        _executor.execute([this] {
            while(!_pending_coroutines.empty()) {
                auto co = _pending_coroutines.front();
                _pending_coroutines.pop();
                co->resume();
            }
            std::cout << "done!" << std::endl;
        });
    }

    void yield() {
        if(!co::test()) throw std::runtime_error("not a coroutine!");
        _pending_coroutines.emplace(co::Coroutine::current().shared_from_this());
        co::this_coroutine::yield();
    }

private:
    Co_executor _executor;
    std::queue<decltype(_executor.execute([]{}))> _pending_coroutines;
};
