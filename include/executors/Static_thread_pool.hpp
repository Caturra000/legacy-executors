#pragma once
#include <cassert>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <condition_variable>
#include "Directionality.hpp"
#include "Blocking.hpp"
#include "Relationship.hpp"
#include "Mapping.hpp"
#include "impl/Functions.hpp"
namespace bsio {

// 1.6.3
class Static_thread_pool {
public:
    // Start to execute
    explicit Static_thread_pool(size_t threads);

    // Stop and wait for completion
    ~Static_thread_pool();

    Static_thread_pool(const Static_thread_pool &) = delete;
    Static_thread_pool& operator=(const Static_thread_pool &) = delete;
    Static_thread_pool(Static_thread_pool &&) = delete;
    Static_thread_pool& operator=(Static_thread_pool &&) = delete;


// Executors
public:
    template <execution::Directionality_property Directionality,
              execution::Blocking_property Blocking,
              execution::Relationship_property Relationship,
              typename Allocator>
    class Executor_impl;

    using Executor_type = Executor_impl<
        execution::Directionality::Oneway,
        execution::Blocking::Possibly,
        execution::Relationship::Fork,
        std::allocator<void>>;

// Core functions
public:

    // A lightweight, type-safe and copyable executor for thread pool
    Executor_type executor();

    // TODO allocator concept
    void execute(execution::Blocking_property auto,
                 execution::Relationship_property auto,
                 const auto &alloc,
                 std::invocable auto func);

    auto twoway_execute(execution::Blocking_property auto,
                        execution::Relationship_property auto,
                        const auto &alloc,
                        std::invocable auto func)
        -> std::future<typename impl::Function_traits<decltype(func)>::Return_type>;

    void attach();

    // Force stop
    void stop();

    // Wait for completion
    void wait();

// Submitted functions
private:
    using Function_signature = impl::Function_signature;

    // Function with callable object/resource and sibling relationship
    // In shared data mode, functions are LIFO for better locality
    // In private data mode, functions are FIFO for continuation
    // See also: @class This_thread_private_data
    using Function_node = impl::Function_node;

    // Type of list_head, without data field
    using Function_intrusive_list = impl::Function_intrusive_list;

    // Type of queue node, with managed and movable resource
    using Function_node_handle = impl::Function_node_handle;

    // Access functions
    using Function_node_access = impl::Function_node_access;

// Thread local
private:

    struct This_thread_private_data;

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    std::vector<std::thread> _threads;
    bool _stopped {false};
    // Running counter, see attach() for details
    size_t _running {1};
    // TODO: multiple shared list_head[N]
    Function_intrusive_list _list_head;
    Function_node_access _queue_access [[no_unique_address]];
};



template <execution::Directionality_property Directionality,
          execution::Blocking_property Blocking,
          execution::Relationship_property Relationship,
          typename Allocator>
class Static_thread_pool::Executor_impl {
    friend class Static_thread_pool;
public:
    Executor_impl(Static_thread_pool *pool, const Allocator &alloc)
        : _pool(pool), _alloc(alloc) {}
    ~Executor_impl() = default;

    auto operator<=>(const Executor_impl &) const = default;

// 1.6.3.3 Operations
// TODO mutual exclusion
public:
    // For
    // execution::Blocking::Never,
    // execution::Blocking::Always, and
    // execution::Blocking::Possibly
    constexpr auto require(execution::Blocking_property auto blocking) const { return Executor_impl<Directionality, decltype(blocking), Relationship, Allocator>{_pool, _alloc}; }
    static constexpr bool query(execution::Blocking_property auto blocking) { return std::is_same_v<Blocking, decltype(blocking)>; }

    // TODO
    // use constexpr operator== to query
    // may be a better customization point

    // For
    // execution::Relationship::Fork
    // execution::Relationship::Continuation
    constexpr auto require(execution::Relationship_property auto relationship) const { return Executor_impl<Directionality, Blocking, decltype(relationship), Allocator>{_pool, _alloc}; }
    static constexpr bool query(execution::Relationship_property auto relationship) { return std::is_same_v<Relationship, decltype(relationship)>; }

    // Thread only
    constexpr auto require(execution::Mapping::Thread) const { return Executor_impl<Directionality, Blocking, Relationship, Allocator>{_pool, _alloc}; }
    static constexpr bool query(execution::Mapping_property auto mapping) { return std::is_same_v<execution::Mapping::Thread, decltype(mapping)>; }

    // For
    // execution::Directionality::Oneway
    // execution::Directionality::Twoway
    constexpr auto require(execution::Directionality_property auto directionality) const { return Executor_impl<decltype(directionality), Blocking, Relationship, Allocator>{_pool, _alloc}; }
    static constexpr bool query(execution::Directionality_property auto directionality) { return std::is_same_v<Directionality, decltype(directionality)>; }

    // For execution context
    Static_thread_pool* query(execution::Context) { return _pool; }
    const Static_thread_pool* query(execution::Context) const { return _pool; }

    constexpr auto prefer(auto any_property) const { return require(any_property); }

    // TODO bulk / allocator / tracking


public:
    void execute(std::invocable auto &&functor)
        requires std::same_as<Directionality, execution::Directionality::Oneway>;

    // Return: std::future<T>
    auto twoway_execute(std::invocable auto &&functor)
        -> std::future<typename impl::Function_traits<decltype(functor)>::Return_type>
        requires std::same_as<Directionality, execution::Directionality::Twoway>;

private:
    Static_thread_pool *_pool;
    Allocator _alloc [[no_unique_address]];
};



struct Static_thread_pool::This_thread_private_data {
    This_thread_private_data(Static_thread_pool *owner);

    ~This_thread_private_data();

    static This_thread_private_data*& instance();

    // FIFO-push
    void private_queue_push(auto functor);

    // Note: unlocked
    void private_queue_detach();

    bool private_queue_empty() const;

    Static_thread_pool *_owner;

    Function_node_handle _head;
    // Sentinel-tail pointer
    Function_node_handle *_tail_ptr {&_head};
    // Remove if-statement and make life easier!
    Function_node_handle *_prev_tail_ptr {nullptr};
    
    This_thread_private_data *_prev_thread_data;
};



template <execution::Directionality_property Directionality,
          execution::Blocking_property Blocking,
          execution::Relationship_property Relationship,
          typename Allocator>
inline void Static_thread_pool::Executor_impl<Directionality, Blocking, Relationship, Allocator>
::execute(std::invocable auto &&functor)
        requires std::same_as<Directionality, execution::Directionality::Oneway> {
    return _pool->execute(Blocking{}, Relationship{}, Allocator{}, std::forward<decltype(functor)>(functor));
}

template <execution::Directionality_property Directionality,
          execution::Blocking_property Blocking,
          execution::Relationship_property Relationship,
          typename Allocator>
inline auto Static_thread_pool::Executor_impl<Directionality, Blocking, Relationship, Allocator>
::twoway_execute(std::invocable auto &&functor)
        -> std::future<typename impl::Function_traits<decltype(functor)>::Return_type>
        requires std::same_as<Directionality, execution::Directionality::Twoway> {
    return _pool->twoway_execute(Blocking{}, Relationship{}, Allocator{}, std::forward<decltype(functor)>(functor));
}

inline Static_thread_pool::Static_thread_pool(size_t threads) {
    while(threads--) {
        _threads.emplace_back(&Static_thread_pool::attach, this);
    }
}

inline Static_thread_pool::~Static_thread_pool() {
    stop();
    wait();
}

inline Static_thread_pool::Executor_type Static_thread_pool::executor() {
    return {this, {}};
}

inline void Static_thread_pool::execute(execution::Blocking_property auto blocking,
                                        execution::Relationship_property auto relationship,
                                        const auto &alloc,
                                        std::invocable auto func)
{
    using Blocking = decltype(blocking);
    using Relationship = decltype(relationship);
    constexpr bool is_blocking_possibly = std::is_same_v<Blocking, execution::Blocking::Possibly>;
    constexpr bool is_blocking_always = std::is_same_v<Blocking, execution::Blocking::Always>;
    constexpr bool is_relationship_continuation = std::is_same_v<Relationship, execution::Relationship::Continuation>;
    [](...){}(alloc); // unused


    if constexpr (is_blocking_possibly || is_blocking_always) {
        if(auto *private_data = This_thread_private_data::instance()) {
            if(private_data->_owner == this) {
                std::invoke(func);
                return;
            }
        }
    }

    if constexpr (is_blocking_always) {
        // Workaround for promise: “std::function target must be copy-constructible”
        // May be fixed by using std::move_only_function after C++23
        auto promise = std::make_shared<std::promise<void>>();
        std::future<void> future = promise->get_future();
        auto executor = [this] {
            auto ex1 = this->executor();
            auto ex2 = ex1.require(execution::blocking.never);
            auto ex3 = ex2.prefer(execution::relationship.continuation);
            return ex3;
        } ();
        executor.execute([func = std::move(func), _ = std::move(promise)]() mutable {
            std::invoke(func);
            // Signal to future automatically
        });
        future.wait();
        return;
    }

    if constexpr (is_relationship_continuation) {
        if(auto *private_data = This_thread_private_data::instance()) {
            if(private_data->_owner == this) {
                // defer
                private_data->private_queue_push(std::move(func));
                return;
            }
        }
    }

    auto new_node = std::make_unique<Function_node>(std::move(func));

    bool wake_more = [&] {
        std::lock_guard lock{_mutex};

        constexpr bool eager_mode = false /*|| is_heavy_task(func)*/;
        bool wake_more = eager_mode && _list_head._next && _list_head._next->_next;

        _queue_access.push(_list_head, std::move(new_node));

        return wake_more;
    } ();

    // If there are too many pending nodes,
    // we can try to notify all
    if(wake_more) {
        _cv.notify_all();
    } else {
        _cv.notify_one();
    }
}

inline auto Static_thread_pool::twoway_execute(
        execution::Blocking_property auto blocking,
        execution::Relationship_property auto relationship,
        const auto &alloc,
        std::invocable auto func)
-> std::future<typename impl::Function_traits<decltype(func)>::Return_type> {
    // Workaround again
    using Ret = typename impl::Function_traits<decltype(func)>::Return_type;
    auto promise = std::make_shared<std::promise<Ret>>();
    auto wrapped_func = [f = std::move(func), promise]() mutable {
        if constexpr (std::is_same_v<void, Ret>) {
            std::invoke(f);
        } else {
            promise->set_value(std::invoke(f));
        }
        
    };
    this->execute(blocking, relationship, alloc, std::move(wrapped_func));
    return promise->get_future();
}

inline void Static_thread_pool::attach() {
    This_thread_private_data private_data {this};
    for(std::unique_lock lock{_mutex}; ; private_data.private_queue_detach()) {
        // _stopped flag: force stop, if anyone send this message
        // _running counter: threads will not sleep when users are ALL wait-ing()
        _cv.wait(lock, [&] { return _stopped || 0 == _running || !_queue_access.empty(_list_head); });
        if(_stopped) return;
        // If users are all wait()-ing but tasks are queueing,
        // we should first complete all the tasks
        if(!_running && _queue_access.empty(_list_head)) return;
        assert(!_queue_access.empty(_list_head));

        // A block scope for resource management
        {
            auto node = _queue_access.consume_one(_list_head);
            // `node` has been detached from shared queue
            lock.unlock();
            std::invoke(node->_func);
            // Optimization: release the resource eagerly
            // This avoid RAII after lock() is acquired
        }

        lock.lock();
    }
}

inline void Static_thread_pool::stop() {
    std::lock_guard lock{_mutex};
    _stopped = true;
    _cv.notify_all();
}

inline void Static_thread_pool::wait() {
    std::unique_lock lock{_mutex};
    auto threads = std::move(_threads);
    if(!threads.empty()) {
        // TODO outstading work
        --_running;
        _cv.notify_all();
        lock.unlock();
        for(auto &&thread : threads) {
            thread.join();
        }
    }
}

inline Static_thread_pool::This_thread_private_data::This_thread_private_data(Static_thread_pool *owner)
    : _owner(owner), _prev_thread_data(instance()) {
    instance() = this;
}

inline Static_thread_pool::This_thread_private_data::~This_thread_private_data() {
    instance() = _prev_thread_data;
}

inline Static_thread_pool::This_thread_private_data*& Static_thread_pool::This_thread_private_data::instance() {
    static thread_local This_thread_private_data *obj {nullptr};
    return obj;
}

inline void Static_thread_pool::This_thread_private_data::private_queue_push(auto functor) {
    _prev_tail_ptr = _tail_ptr;
    *_tail_ptr = std::make_unique<Function_node>(std::move(functor));
    _tail_ptr = &((*_tail_ptr)->_next);
    
}

inline void Static_thread_pool::This_thread_private_data::private_queue_detach() {
    if(!private_queue_empty()) {
        auto queue_access = _owner->_queue_access;
        auto &non_empty_tail = *_prev_tail_ptr;
        if(_head == non_empty_tail) {
            queue_access.push(_owner->_list_head, std::move(_head));
        } else {
            queue_access.push(_owner->_list_head, std::move(_head), non_empty_tail);
        }

        // reset to empty private list
        _tail_ptr = &_head;
        _prev_tail_ptr = nullptr;
    }
}

inline bool Static_thread_pool::This_thread_private_data::private_queue_empty() const {
    return _tail_ptr == &_head;
}

} // namespace bsio