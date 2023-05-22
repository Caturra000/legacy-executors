#pragma once
#include <type_traits>
#include "Generic_property.hpp"
#include "Query_only_property.hpp"

namespace bsio {
namespace execution {
namespace impl {
namespace directionality_impl {

struct Top_class_tag {};

template <typename Derived>
using Top_class_property = Query_only_property<Top_class_tag, Derived>;

// Base common type for concept
struct Tag: Top_class_tag {};

template <typename Derived>
struct Property: Generic_property<Tag, Derived> {
    static constexpr bool is_preferable = false;
};

// Top class is not included
template <typename Property>
concept Directionality_property = std::is_base_of_v<Tag, Property>;

} // namespace directionality_impl
} // namespace impl
} // namespace execution
} // namespace bsio
