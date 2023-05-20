#pragma once
#include "Coroutine.h"

namespace co {
namespace this_coroutine {

inline void yield() {
    return ::co::Coroutine::yield();
}

} // this_coroutine

inline bool test() {
    return Coroutine::test();
}

inline Environment& open() {
    return Environment::instance();
}

} // co