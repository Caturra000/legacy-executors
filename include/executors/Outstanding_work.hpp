#pragma once
#include "impl/outstanding_work_impl.hpp"
namespace bsio {
namespace execution {

struct Outstanding_work: impl::outstanding_work_impl::Top_class_property<Outstanding_work> {
    // The existence of the executor object represents an indication
    // of likely future submission of a function object. The executor
    // or its associated execution context may choose to maintain
    // execution resources in anticipation of this submission.
    struct Tracked
        : impl::outstanding_work_impl::Property<Tracked> {};

    // The existence of the executor object does not indicate
    // any likely future submission of a function object.
    struct Untracked
        : impl::outstanding_work_impl::Property<Untracked> {};

    inline static constexpr Tracked tracked {};
    inline static constexpr Untracked untracked {};
};

inline constexpr Outstanding_work outstanding_work;

template <typename Property>
concept Outstanding_work_property = impl::outstanding_work_impl::Outstanding_work_impl_property<Property>;

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T, execution::Outstanding_work_property P>
struct Is_applicable_property<T, P>: std::true_type {};

template <typename T>
struct Is_applicable_property<T, execution::Outstanding_work>: std::true_type {};

} // namespace bsio
