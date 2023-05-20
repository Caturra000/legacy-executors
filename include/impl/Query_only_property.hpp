#pragma once
#include <type_traits>

namespace bsio {
namespace execution {
namespace impl {

template <typename Tag, typename Derived>
struct Query_only_property: Tag {
    // These constants are used to determine whether the property
    // can be used with the require and prefer customization points,
    // respectively.

    static constexpr bool is_requirable = false;
    static constexpr bool is_preferable = false; 

    // If both static_query_v and value() are present, they shall
    // return the same type and this type shall satisfy the
    // EqualityComparable requirements
    //
    // These are used to determine whether invoking require would
    // result in an identity transformation. 

    template <typename Executor,
        typename = decltype(Executor::query(Derived()))>
    static constexpr decltype(auto)
        static_query_v = Executor::query(Derived());

    // TODO return enumeration
    static constexpr bool value() { return true; }

    constexpr bool operator==(const Query_only_property&) const { return true; }
    constexpr bool operator!=(const Query_only_property&) const { return false; }
};

} // namespace impl
} // namespace execution
} // namespace bsio
