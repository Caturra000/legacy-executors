#pragma once
#include "impl/context_impl.hpp"
namespace bsio {
namespace execution {

// 1.3.3.1 Associated execution context property
// The context_t property can be used only with query
struct Context: impl::context_impl::Top_class_property<Context> {
};

inline constexpr Context context;

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T>
struct Is_applicable_property<T, execution::Context>: std::true_type {};


} // namespace bsio