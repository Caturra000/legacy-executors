#pragma once
#include "impl/relationship_impl.hpp"
namespace bsio {
namespace execution {

struct Relationship: impl::relationship_impl::Top_class_property<Relationship> {
	// Function objects submitted through the executor 
    // do not represent continuations of the caller.
    struct Fork
        : impl::relationship_impl::Property<Fork> {};

	// Function objects submitted through the executor represent
    // continuations of the caller. Invocation of the submitted
    // function object may be deferred until the caller completes.
    struct Continuation
        : impl::relationship_impl::Property<Continuation> {};

    inline static constexpr Fork fork {};
    inline static constexpr Continuation continuation {};
};

inline constexpr Relationship relationship;

template <typename Property>
concept Relationship_property = impl::relationship_impl::Relationship_property<Property>;

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T, execution::Relationship_property P>
struct Is_applicable_property<T, P>: std::true_type {};

template <typename T>
struct Is_applicable_property<T, execution::Relationship>: std::true_type {};

} // namespace bsio
