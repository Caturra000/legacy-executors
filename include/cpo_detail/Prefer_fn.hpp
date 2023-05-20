#pragma once
#include <type_traits>
namespace bsio {

template <typename, typename>
struct Is_applicable_property;

namespace cpo_detail {

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Prefer_rules0 =
        Is_applicable_property<T, Prop>::value && Prop::is_preferable;

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Prefer_rules1 =
        Prefer_rules0<E, P> &&
        requires {Prop::template static_query_v<T>;} &&
        Prop::template static_query_v<T> == Prop::value();

    template <typename E, typename P>
    concept Prefer_rules2 =
        Prefer_rules0<E, P> &&
        !Prefer_rules1<E, P> &&
        requires(E e, P p) {
            e.prefer(p);
        };

    template <typename E, typename P>
    concept Prefer_rules3 =
        Prefer_rules0<E, P> &&
        !Prefer_rules1<E, P> && !Prefer_rules2<E, P> &&
        requires(E e, P p) {
            prefer(e, p);
        };

    template <typename E, typename P>
        requires Prefer_rules0<E, P>
    inline constexpr decltype(auto) prefer_lookup(E e, P p) {
        if constexpr (Prefer_rules1<E, P>) {
            return e;
        } else if constexpr (Prefer_rules2<E, P>) {
            return e.prefer(p);
        } else if constexpr (Prefer_rules3<E, P>){
            return prefer(e, p);
        } else {
            static_assert(
                Prefer_rules1<E, P> ||
                Prefer_rules2<E, P> ||
                Prefer_rules3<E, P>,
                "bsio::prefer() is ill-formed."
            );
        }
    }

    struct Prefer_fn {
        template <typename E, typename P>
        constexpr decltype(auto) operator()(E e, P p) const
        requires Prefer_rules0<E, P>
                    && (Prefer_rules1<E, P> ||
                        Prefer_rules2<E, P> ||
                        Prefer_rules3<E, P>) {
            return prefer_lookup(e, p);
        }
    };

} // namespace cpo_detail
} // namespace bsio
