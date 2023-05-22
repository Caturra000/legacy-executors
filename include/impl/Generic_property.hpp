#pragma once
#include <type_traits>

namespace bsio {
namespace execution {
namespace impl {

// Default to boolean value
template <typename Tag, typename Derived>
struct Generic_property: Tag {
    // These constants are used to determine whether the property
    // can be used with the require and prefer customization points,
    // respectively.

    static constexpr bool is_requirable = true;
    static constexpr bool is_preferable = true; 

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

    static constexpr bool value() { return true; }

    constexpr bool operator==(const Generic_property&) const { return true; }
    constexpr bool operator!=(const Generic_property&) const { return false; }
};

} // namespace impl
} // namespace execution
} // namespace bsio
