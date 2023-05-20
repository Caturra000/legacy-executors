#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

template <typename T>
class Pipe;

// Retrieve data from this pipe
template <typename T>
class In;

// Transfer data to this pipe
template <typename T>
class Out;

template <typename T>
class Queue {
    template <typename> friend class Pipe;
private:
    void push(T data) {
        auto ptr = std::make_unique<T>(std::move(data));
        push(std::move(ptr));
    }

    void push(std::unique_ptr<T> ptr) {
        {
            std::lock_guard lock {_mutex};
            _queue.emplace(std::move(ptr));
        }
        _condition.notify_one();
    }

    std::unique_ptr<T> pop() {
        std::unique_lock lock {_mutex};
        _condition.wait(lock, [this] {return !_queue.empty() || _stop;});
        if(_stop) return {};
        auto ptr = std::move(_queue.front());
        _queue.pop();
        return ptr;
    }

    void stop() {
        std::unique_lock lock {_mutex};
        _stop = true;
        _condition.notify_all();
    }

private:
    std::queue<std::unique_ptr<T>> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;
    bool _stop {false};
};

template <typename T>
class Pipe {
public:
    using Value_type = T;
    explicit Pipe(std::shared_ptr<Queue<T>> queue)
        : _queue_ref(std::move(queue))
    {
        if(!_queue_ref) [[unlikely]] {
            throw std::runtime_error("Bad pipe.");
        }
    }

protected:
    void push(T data) const { _queue_ref->push(std::move(data)); }
    void push(std::unique_ptr<T> ptr) const { _queue_ref->push(std::move(ptr)); }
    std::unique_ptr<T> pop() const { return _queue_ref->pop(); }
    void stop() const { _queue_ref->stop(); }

private:
    std::shared_ptr<Queue<T>> _queue_ref;
};

// Retrieve data from this pipe
template <typename T>
class In: public Pipe<T> {
public:
    explicit In(std::shared_ptr<Queue<T>> queue): Pipe<T>(std::move(queue)) {}
    using Pipe<T>::pop;
};

// Transfer data to this pipe
template <typename T>
class Out: public Pipe<T> {
public:
    explicit Out(std::shared_ptr<Queue<T>> queue): Pipe<T>(std::move(queue)) {}
    using Pipe<T>::push;
    using Pipe<T>::stop;
};
