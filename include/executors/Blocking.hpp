#pragma once
#include "impl/blocking_impl.hpp"
namespace bsio {
namespace execution {

struct Blocking: impl::blocking_impl::Top_class_property<Blocking> {
    // Invocation of an executor’s execution function may block
    // pending completion of one or more invocations of the
    // submitted function object.
    struct Possibly
        : impl::blocking_impl::Property<Possibly> {};

    // Invocation of an executor’s execution function shall block
    // until completion of all invocations of submitted function object.
    struct Always
        : impl::blocking_impl::Property<Always> {};

    // Invocation of an executor’s execution function shall not block
    // pending completion of the invocations of the submitted function object.
    struct Never
        : impl::blocking_impl::Property<Never> {};

    inline static constexpr Possibly possibly {};
    inline static constexpr Always always {};
    inline static constexpr Never never {};
};

inline constexpr Blocking blocking;

template <typename Property>
concept Blocking_property = impl::blocking_impl::Blocking_property<Property>;

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T, execution::Blocking_property P>
struct Is_applicable_property<T, P>: std::true_type {};

template <typename T>
struct Is_applicable_property<T, execution::Blocking>: std::true_type {};

} // namespace bsio
