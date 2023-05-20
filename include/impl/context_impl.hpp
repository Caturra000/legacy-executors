#pragma once
#include <type_traits>
#include "Query_only_property.hpp"

namespace bsio {
namespace execution {
namespace impl {
namespace context_impl {

struct Top_class_tag {};

template <typename Derived>
using Top_class_property = Query_only_property<Top_class_tag, Derived>;

} // namespace context_impl
} // namespace impl
} // namespace execution
} // namespace bsio
