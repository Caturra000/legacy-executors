#pragma once
#include <vector>
#include <deque>
#include <memory>
#include "execution.hpp"
#include "property.hpp"

class Actor;
using Actor_address = Actor*;

class Handler {
public:
    virtual ~Handler() {}
    virtual const std::type_info& message_id() = 0;
};

template <typename Message>
class Message_handler: public Handler {
public:
    virtual void handle_message(Message message, Actor_address from) = 0;
};

// A message handler in this example
template <typename Actor, typename Message>
class Member_function_message_handler: public Message_handler<Message> {
public:
    using Member_function = void (Actor:: *)(Message, Actor_address);
    Member_function_message_handler(Member_function function, Actor *who)
        : _function{function}, _who{who} {}

// overrides
public:
    const std::type_info& message_id() override {
        return typeid(Message);
    }

    void handle_message(Message message, Actor_address from) override {
        (_who->*_function)(std::move(message), from);
    }

// helpers
public:
    bool is_this_function(Member_function function) const {
        return _function == function;
    }

private:
    Member_function _function;
    // Actor who calls the member function
    Actor *_who;
};

// Base class for all actors
class Actor {
public:
    virtual ~Actor() {}

public:
    Actor_address address() { return this; }

    template <typename Message>
    friend void send(Message message, Actor_address from, Actor_address to) {
        auto ex = bsio::require(to->_executor, bsio::execution::blocking.never);
        ex.execute([=, msg = std::move(message)]() mutable {
            to->call_handler(std::move(msg), from);
        });
    }

// For derived actors
protected:
    // pool: Where the executor comes from
    // TODO: Polymorphic executor
    explicit Actor(bsio::Static_thread_pool &pool);

    template <typename Actor_impl, typename Message>
    void register_handler(void (Actor_impl::*function)(Message, Actor_address));

    template <typename Actor_impl, typename Message>
    void deregister_handler(void (Actor_impl::*function)(Message, Actor_address));

    template <typename Message>
    void defer_send(Message message, Actor_address to);

private:
    template <typename Message>
    void call_handler(Message message, Actor_address from);

private:
    bsio::Static_thread_pool::Executor_type _executor;
    std::vector<std::shared_ptr<Handler>> _handlers;
};

// For synchronization
template <typename Message>
class Receiver:
    private bsio::Static_thread_pool,
    public Actor
{
public:
    Receiver();

public:
    // Wait for message
    void wait(size_t count = 1);

private:
    void message_handler(Message message, Actor_address ignored);

private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::deque<Message> _message_queue;
};


inline Actor::Actor(bsio::Static_thread_pool &pool)
    : _executor(pool.executor()) {}

template <typename Actor_impl, typename Message>
inline void Actor::register_handler(void (Actor_impl::*function)(Message, Actor_address)) {
    _handlers.emplace_back(
        std::make_shared<Member_function_message_handler<Actor_impl, Message>>(
            function, static_cast<Actor_impl*>(this)));
}

template <typename Actor_impl, typename Message>
inline void Actor::deregister_handler(void (Actor_impl::*function)(Message, Actor_address)) {
    const std::type_info &id = typeid(Message_handler<Message>);
    for(auto iter = std::begin(_handlers); iter != std::end(_handlers); ++iter) {
        auto &&handler = *iter;
        if(handler->message_id() != id) continue;
        auto mh = static_cast<Member_function_message_handler<Actor_impl, Message>*>(handler.get());
        if(!mh->is_this_function(function)) continue;
        _handlers.erase(iter);
        return;
    }
}

template <typename Message>
inline void Actor::defer_send(Message message, Actor_address to) {
    auto ex = [to] {
        auto ex1 = bsio::require(to->_executor, bsio::execution::blocking.never);
        auto ex2 = bsio::prefer(ex1, bsio::execution::relationship.continuation);
        return ex2;
    } ();
    ex.execute([=, msg = std::move(message), from = this]() mutable {
        to->call_handler(std::move(msg), from);
    });
}

template <typename Message>
inline void Actor::call_handler(Message message, Actor_address from) {
    const std::type_info &this_message_id = typeid(Message);
    for(auto &&handler : _handlers) {
        if(handler->message_id() != this_message_id) continue;
        auto mh = static_cast<Message_handler<Message>*>(handler.get());
        mh->handle_message(message, from);
    }
}

template <typename Message>
inline Receiver<Message>::Receiver()
    : bsio::Static_thread_pool(1),
      Actor(*static_cast<bsio::Static_thread_pool*>(this)) {
    register_handler(&Receiver::message_handler);
}

template <typename Message>
inline void Receiver<Message>::wait(size_t count) {
    while(count--) {
        std::unique_lock lock {_mutex};
        _condition.wait(lock, [this] {
            return !_message_queue.empty();
        });
        _message_queue.pop_front();
    }
}

template <typename Message>
inline void Receiver<Message>::message_handler(Message message, Actor_address ignored) {
    std::ignore = ignored;
    std::lock_guard lock {_mutex};
    _message_queue.push_back(std::move(message));
    _condition.notify_one();
}
