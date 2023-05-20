#pragma once
#include <type_traits>
namespace bsio {

template <typename, typename>
struct Is_applicable_property;

namespace cpo_detail {

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Require_rules0 =
        Is_applicable_property<T, Prop>::value && Prop::is_requirable;

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Require_rules1 =
        Require_rules0<E, P> &&
        requires {Prop::template static_query_v<T>;} &&
        Prop::template static_query_v<T> == Prop::value();

    template <typename E, typename P>
    concept Require_rules2 =
        Require_rules0<E, P> &&
        !Require_rules1<E, P> &&
        requires(E e, P p) {
            e.require(p);
        };

    template <typename E, typename P>
    concept Require_rules3 =
        Require_rules0<E, P> &&
        !Require_rules1<E, P> && !Require_rules2<E, P> &&
        requires(E e, P p) {
            require(e, p);
        };

    template <typename E, typename P>
        requires Require_rules0<E, P>
    inline constexpr decltype(auto) require_lookup(E e, P p) {
        if constexpr (Require_rules1<E, P>) {
            return e;
        } else if constexpr (Require_rules2<E, P>) {
            return e.require(p);
        } else if constexpr (Require_rules3<E, P>){
            return require(e, p);
        } else {
            static_assert(
                Require_rules1<E, P> ||
                Require_rules2<E, P> ||
                Require_rules3<E, P>,
                "bsio::require() is ill-formed."
            );
        }
    }

    struct Require_fn {
        template <typename E, typename P>
        constexpr decltype(auto) operator()(E e, P p) const
        requires Require_rules0<E, P>
                    && (Require_rules1<E, P> ||
                        Require_rules2<E, P> ||
                        Require_rules3<E, P>) {
            return require_lookup(e, p);
        }
    };

} // namespace cpo_detail
} // namespace bsio
