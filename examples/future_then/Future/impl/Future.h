#ifndef __FLUENT_FUTURE_FUTURE_H__
#define __FLUENT_FUTURE_FUTURE_H__
#include <type_traits>
#include <mutex>
#include "execution.hpp"
#include "FunctionTraits.h"
#include "FutureTraits.h"
#include "ControlBlock.h"
#include "Promise.h"
namespace fluent {

// for SFINAE
namespace future_requires {

template <typename T, typename Functor>
struct Then:
    std::enable_if<
        IsThenValid<Future<T>, Functor>::value
    > {};

template <typename T, typename Functor>
struct Poll:
    std::enable_if<
        IsThenValid<Future<T>, Functor>::value &&
        std::is_same<typename FunctionTraits<Functor>::ReturnType, bool>::value
    > {};

template <typename T, typename Functor>
struct CancelIf:
    Poll<T, Functor> {};

template <typename T, typename Functor>
struct Wait:
    std::enable_if<
        IsThenValid<Future<T>, Functor>::value &&
        !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T&&>>::value &&
        std::is_same<typename FunctionTraits<Functor>::ReturnType, void>::value
    > {};
}

template <typename T>
class Promise;

template <typename T>
class Future {
    template <typename> friend class Promise;
// basic
public:
    Future(Then_execution_context *context, SharedPtr<ControlBlock<T>> shared);
    Future(Then_execution_context::Executor_type executor, SharedPtr<ControlBlock<T>> shared);
    Future(const Future &) = delete;
    Future(Future &&) = default;
    Future& operator=(const Future&) = delete;
    Future& operator=(Future&&) = default;

// asynchronous operations
public:
    template <typename Functor,
              typename R = typename FunctionTraits<Functor>::ReturnType,
              typename = typename future_requires::Then<T, Functor>::type>
    Future<R> then(Functor &&f);

    template <typename Functor,
              typename = typename future_requires::CancelIf<T, Functor>::type>
    Future<T> cancelIf(Functor &&f);

// Debug
public:
    bool hasResult();

    // !!UNSAFE
    T get();

    // !!UNSAFE
    SharedPtr<ControlBlock<T>> getControlBlock();

private:
    // unsafe
    // ensure: READY & has then_
    void postRequest();

    // Callback: void(T&&, Promise<R>)
    template <typename R, typename Callback>
    Future<R> futureRoutine(Callback &&callback);

private:
    Then_execution_context::Executor_type _executor;
    SharedPtr<ControlBlock<T>> _shared;
};

template <typename T>
inline Future<T>::Future(Then_execution_context *context, SharedPtr<ControlBlock<T>> shared)
    : _executor(context, {}),
      _shared(std::move(shared)) {}

template <typename T>
inline Future<T>::Future(Then_execution_context::Executor_type executor, SharedPtr<ControlBlock<T>> shared)
    : _executor(executor),
      _shared(std::move(shared)) {}

template <typename T>
template <typename Functor, typename R, typename>
inline Future<R> Future<T>::then(Functor &&f) {
    using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
    return futureRoutine<R>([f](T &&value, Promise<R> promise) mutable {
        promise.setValue(f(std::forward<ForwardType>(value)), true);
    });
}

template <typename T>
template <typename Functor, typename>
inline Future<T> Future<T>::cancelIf(Functor &&f) {
    using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
    return futureRoutine<T>([f, this](T &&value, Promise<T> promise) {
        std::unique_lock<std::mutex> lock{_shared->_mutex};
        if(f(std::forward<ForwardType>(value))) {
            promise.cancel();
        } else {
            // forward the current future to the next
            promise.setValue(std::forward<ForwardType>(value), false);
        }
    });
}

template <typename T>
inline bool Future<T>::hasResult() {
    return _shared->_state == State::READY;
}

template <typename T>
inline T Future<T>::get() {
    auto value = std::move(_shared->_value);
    _shared->_state = State::DEAD;
    return value;
}

template <typename T>
inline SharedPtr<ControlBlock<T>> Future<T>::getControlBlock() {
    return _shared;
}

template <typename T>
inline void Future<T>::postRequest() {
    _shared->_state = State::POSTED;
    _executor.execute([shared = _shared] {
        shared->_then(static_cast<T&&>(shared->_value));
        shared->_state = State::DONE;
    });
}

template <typename T>
template <typename R, typename Callback>
inline Future<R> Future<T>::futureRoutine(Callback &&callback) {
    Promise<R> promise(_executor);
    auto future = promise.get();

    std::unique_lock<std::mutex> lock{_shared->_mutex};

    State state = _shared->_state;
    if(state == State::NEW || state == State::READY) {
        // async request, will be set value and then callback

        // register request
        _shared->_then = [promise = std::move(promise), callback](T &&value) mutable {
            callback(std::forward<T>(value), std::move(promise));
        };

        // value is ready
        // post request immediately
        if(state == State::READY) {
            _shared->_state = State::POSTED;
            _executor.execute([shared = _shared] {
                // shared->_value may be moved
                // _then must be T&&
                shared->_then(static_cast<T&&>(shared->_value));
                shared->_state = State::DONE;
            });
        }
    } else if(state == State::CANCEL) {
        // return a future will never be setValue()
        promise.cancel();
    }
    return future;
}

} // fluent
#endif