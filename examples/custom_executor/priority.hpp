#pragma once
#include <queue>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include "execution.hpp"
#include "property.hpp"

// Define a priority property
struct Priority {
    static constexpr bool is_requirable = true;
    static constexpr bool is_preferable = true;
    int value() const { return _value; }

    int _value {1};

    friend bool operator<(Priority lhs, Priority rhs) {
        return lhs._value < rhs._value;
    }
};

inline constexpr Priority low_priority{0};
inline constexpr Priority normal_priority{1};
inline constexpr Priority high_priority{2};

template <typename T>
struct bsio::Is_applicable_property<T, Priority>: std::true_type {};

struct Priority_execution_context {
public:
    class Executor_type {
    public:
        Executor_type(Priority_execution_context *context, int priority_value = normal_priority.value())
            : _context(context), _priority_value{priority_value} {}

        auto query(bsio::execution::Context) { return _context; }
        auto query(Priority) const { return _priority_value; }
        auto require(Priority priority) const {
            Executor_type e{_context, priority.value()};
            return e;
        }

        template <typename F>
        void execute(F f) {
            _context->enqueue(_priority_value, std::move(f));
        }

    private:
        Priority_execution_context *_context;
        int _priority_value;
    };

private:
    using Item_base = std::tuple<int, std::function<void()>>;
    struct Item: Item_base {
        using Item_base::Item_base;
        bool operator < (const Item &rhs) const {
            return get<0>(*this) < get<0>(rhs);
        }
    };

public:
    template <typename F>
    void enqueue(int priority, F f) {
        Item item {priority, std::move(f)};
        {
            std::lock_guard lock {_mutex};
            _queue.emplace(std::move(item));
        }
        _condition.notify_one();
    }

    Executor_type executor() { return {this}; }

    void run() {
        for(std::unique_lock lock{_mutex};;) {
            _condition.wait(lock, [this] {
                return !_queue.empty() || _stop;
            });
            if(_stop) return;
            auto i = std::move(_queue.top());
            _queue.pop();
            lock.unlock();
            std::get<1>(i)();
            lock.lock();
        }
    }

    void stop() {
        std::unique_lock lock {_mutex};
        _stop = true;
        _condition.notify_all();
    }

private:
    std::priority_queue<Item> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;
    bool _stop {false};
};
