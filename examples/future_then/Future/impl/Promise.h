#ifndef __FLUENT_FUTURE_PROMISE_H__
#define __FLUENT_FUTURE_PROMISE_H__
#include <memory>
#include "then_context.hpp"
#include "ControlBlock.h"
namespace fluent {

template <typename T>
class Future;

// NOTE:
// Promise is not copyable, but we need default const& operation
// For the implementation, the moved capture lambda cannot be stored in std::function
template <typename T>
class Promise {
public:
    explicit Promise(Then_execution_context::Executor_type executor)
        : _executor(executor),
// #ifdef __GNUC__
//           _shared(std::__make_shared<ControlBlock<T>, std::_S_single>()) {}
// #else
          _shared(std::make_shared<ControlBlock<T>>()) {}
// #endif

    explicit Promise(Then_execution_context *context): Promise(context->executor()) {}

    template <typename T_>
    void setValue(T_ &&value, bool need_lock) {
        std::optional<std::unique_lock<std::mutex>> opt_lock;
        if(need_lock) {
            opt_lock = std::unique_lock<std::mutex>{_shared->_mutex};
        }
        if(_shared->_state == State::NEW) {
            _shared->_value = std::forward<T_>(value);
            _shared->_state = State::READY;
            if(_shared->_then) {
                _shared->_state = State::POSTED;
                _executor.execute([shared = _shared] {
                    shared->_then(static_cast<T&&>(shared->_value));
                    shared->_state = State::DONE;
                });
            }
        } else if(_shared->_state == State::CANCEL) {
            // ignore
            return;
        } else {
            throw std::runtime_error("promise can only set once.");
        }
    }

    void cancel() {
        _shared->_state = State::CANCEL;
    }

    Future<T> get() {
        return Future<T>(_executor, _shared);
    };

private:
    Then_execution_context::Executor_type _executor;
    SharedPtr<ControlBlock<T>> _shared;
};

} // fluent
#endif