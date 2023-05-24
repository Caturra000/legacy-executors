#pragma once
#include <type_traits>
#include "pipeline.hpp"

namespace pipeline_stream {

// End of pipeline
inline constexpr struct Done {} done {};

// Stateless pipeline unit
template <typename ...Fs>
struct P_unit {
    auto operator|(auto &&f) const {
        return P_unit<Fs..., std::decay_t<decltype(f)>>{};
    }

    auto operator|(Done) const {
        // Since we haven't store any function pointer address into template/instance
        static_assert((!std::is_pointer_v<Fs> && ...),
            "This example only support simple lambda/operator() types");
        // Need default ctor
        return pipeline(Fs{}...);
    }
};

// Factory type
inline constexpr struct {
    auto operator|(auto &&f) const {
        return P_unit<>{}|(std::forward<decltype(f)>(f));
    };
} make {};

} // namespace pipeline_stream
