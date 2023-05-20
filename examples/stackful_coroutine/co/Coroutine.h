#pragma once
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include <array>
#include "State.h"
#include "Context.h"

namespace co {

class Environment;

class Coroutine: public std::enable_shared_from_this<Coroutine> {
    friend class Environment;
    friend class Context;

public:
    static Coroutine& current();

    static bool test();

    const State runtime() const;
    bool exit() const;
    bool running() const;

    static void yield();

    const State resume();

    Coroutine(const Coroutine&) = delete;
    Coroutine(Coroutine&&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;
    Coroutine& operator=(Coroutine&&) = delete;

public:
    template <typename Entry, typename ...Args>
    Coroutine(Environment *master, Entry &&entry, Args &&...arguments)
        : _entry([=]() mutable { entry(std::move(arguments)...); }),
          _context(nullptr),
          _master(master) {}

private:
    static void routineWrapper(Coroutine *coroutine);

private:
    State _runtime {};
    std::unique_ptr<Context> _context;
    std::function<void()> _entry;
    Environment *_master;
};

class Environment {
    friend class Coroutine;
public:
    static Environment& instance();

    template <typename Entry, typename ...Args>
    std::shared_ptr<Coroutine> createCoroutine(Entry &&entry, Args &&...arguments);

    Coroutine* current();

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

private:
    void push(std::shared_ptr<Coroutine> coroutine);
    void pop();
    Environment();

private:
    std::vector<std::shared_ptr<Coroutine>> _cStack;
    std::shared_ptr<Coroutine> _main;

private:
    std::unique_ptr<Context> reuse();
    bool reusable() { return _recycleTop > 0; }

    void recycle(std::unique_ptr<Context> trash);
    bool recyclable() { return _recycleTop != _recycleStack.size(); }


private:
    std::array<std::unique_ptr<Context>, 0xff> _recycleStack;
    size_t _recycleTop {};
};


template <typename Entry, typename ...Args>
inline std::shared_ptr<Coroutine> Environment::createCoroutine(Entry &&entry, Args &&...arguments) {
    return std::make_shared<Coroutine>(
        this, std::forward<Entry>(entry), std::forward<Args>(arguments)...);
}

inline Environment& Environment::instance() {
    static thread_local Environment env;
    return env;
}

inline Coroutine* Environment::current() {
    return _cStack.back().get();
}

inline void Environment::push(std::shared_ptr<Coroutine> coroutine) {
    _cStack.emplace_back(std::move(coroutine));
}

inline void Environment::pop() {
    _cStack.pop_back();
}

inline Environment::Environment() {
    _main = std::make_shared<Coroutine>(this, [](){});
    _main->_context = std::make_unique<Context>();
    push(_main);
}

inline std::unique_ptr<Context> Environment::reuse() {
    auto up = std::move(_recycleStack[--_recycleTop]);
    return up;
}

inline void Environment::recycle(std::unique_ptr<Context> trash) {
    _recycleStack[_recycleTop++] = std::move(trash);
}

inline Coroutine& Coroutine::current() {
    return *Environment::instance().current();
}

inline bool Coroutine::test() {
    return current()._context && current()._context->test();
}

inline const State Coroutine::runtime() const {
    return _runtime;
}

inline bool Coroutine::exit() const {
    return _runtime & State::EXIT;
}

inline bool Coroutine::running() const {
    return _runtime & State::RUNNING;
}

inline const State Coroutine::resume() {
    if(_runtime & State::EXIT) {
        return _runtime;
    }
    if(!(_runtime & State::RUNNING)) {
        _context = _master->reusable() ?
            _master->reuse() : std::make_unique<Context>();
        _context->prepare(Coroutine::routineWrapper, this);
        _runtime |= State::RUNNING;
    }
    auto previous = _master->current();
    _master->push(shared_from_this());
    _context->switchFrom(previous->_context.get());
    return _runtime;
}

inline void Coroutine::yield() {
    auto &coroutine = current();
    auto &currentContext = coroutine._context;

    coroutine._master->pop();

    auto &previousContext = current()._context;
    if(currentContext) {
        previousContext->switchFrom(currentContext.get());
    } else {
        previousContext->switchOnly();
    }
}

inline void Coroutine::routineWrapper(Coroutine *coroutine) {
    auto &routine = coroutine->_entry;
    auto &runtime = coroutine->_runtime;
    auto *master = coroutine->_master;
    if(routine) routine();
    runtime ^= (State::EXIT | State::RUNNING);

    if(master->recyclable()) {
        master->recycle(std::move(coroutine->_context));
    }

    yield();
}

} // co
