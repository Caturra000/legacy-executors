#pragma once
namespace bsio {
namespace execution {

template <typename Interface_changable_property, typename ...Properites>
using Polymorphic_executor =
    typename Interface_changable_property::template Polymorphic_executor_type<Properites...>;

} // namespace execution
} // namespace bsio
