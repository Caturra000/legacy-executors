#pragma once
#include "impl/mapping_impl.hpp"
namespace bsio {
namespace execution {

struct Mapping: impl::mapping_impl::Top_class_property<Mapping> {
    struct Thread
        : impl::mapping_impl::Property<Thread> {};
    struct New_thread
        : impl::mapping_impl::Property<New_thread> {};
    struct Other
        : impl::mapping_impl::Property<Other> {};

    inline static constexpr Thread thread {};
    inline static constexpr New_thread new_thread {};
    inline static constexpr Other other {};
};

inline constexpr Mapping mapping;

template <typename Property>
concept Mapping_property = impl::mapping_impl::Mapping_property<Property>;

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T, execution::Mapping_property P>
struct Is_applicable_property<T, P>: std::true_type {};

template <typename T>
struct Is_applicable_property<T, execution::Mapping>: std::true_type {};

} // namespace bsio
