#ifndef __FLUENT_V3_FUTURE_CONTROL_BLOCK_H__
#define __FLUENT_V3_FUTURE_CONTROL_BLOCK_H__
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
namespace fluent {

template <typename T>
// #ifdef __GNUC__
// using SharedPtr = std::__shared_ptr<T, __gnu_cxx::_S_single>;
// #else
using SharedPtr = std::shared_ptr<T>;
// #endif

enum class State {
    // newcomer, mengxin
    // it may have _then
    NEW,
    // value has been set
    // it may have _then
    READY,
    // then() has been posted as a request in loop
    POSTED,
    // request is done
    // value may be moved
    DONE,
    // called by future.get()
    // value is moved
    // this state cannot be reused
    // (sub)task will be break, but you should not .get() while using .then()
    DEAD,
    // (sub)task will be break
    CANCEL,
};

template <typename T>
struct ControlBlock {
    std::atomic<State>       _state;
    T                        _value;
    std::function<void(T&&)> _then;
    std::mutex               _mutex;

    ControlBlock(): _state(State::NEW) {}
};

} // fluent
#endif