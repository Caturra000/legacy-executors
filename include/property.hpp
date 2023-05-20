#pragma once
#include "cpo_detail/Require_fn.hpp"
#include "cpo_detail/Prefer_fn.hpp"
#include "cpo_detail/Query_fn.hpp"
namespace bsio {

// P1393R0
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1393r0.html
// P0443R9
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0443r9.html

/// Customization Point Objects
// Executor customization points are functions which adapt an executor's properties.
// Executor customization points enable uniform use of executors in generic contexts.

inline constexpr cpo_detail::Require_fn
    require {};

inline constexpr cpo_detail::Prefer_fn
    prefer {};

inline constexpr cpo_detail::Query_fn
    query {};


template <typename T, typename P>
struct Is_applicable_property: std::false_type {};

template <typename T, typename P>
inline constexpr bool
    is_applicable_property_v = Is_applicable_property<T, P>::value;

} // namespace bsio
