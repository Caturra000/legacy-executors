#pragma once
#include <type_traits>
namespace bsio {

template <typename, typename>
struct Is_applicable_property;

namespace cpo_detail {

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Query_rules0 =
        Is_applicable_property<T, Prop>::value;

    template <typename E, typename P,
                typename T = std::decay_t<E>, typename Prop = std::decay_t<P>>
    concept Query_rules1 =
        Query_rules0<E, P> &&
        requires {Prop::template static_query_v<T>;};

    template <typename E, typename P>
    concept Query_rules2 =
        Query_rules0<E, P> &&
        !Query_rules1<E, P> &&
        requires(E e, P p) {
            e.query(p);
        };

    template <typename E, typename P>
    concept Query_rules3 =
        Query_rules0<E, P> &&
        !Query_rules1<E, P> && !Query_rules2<E, P> &&
        requires(E e, P p) {
            query(e, p);
        };

    template <typename E, typename P>
        requires Query_rules0<E, P>
    inline constexpr decltype(auto) query_lookup(E e, P p) {
        if constexpr (Query_rules1<E, P>) {
            using T = std::decay_t<E>;
            using Prop = std::decay_t<P>;
            return Prop::template static_query_v<T>;
        } else if constexpr (Query_rules2<E, P>) {
            return e.query(p);
        } else if constexpr (Query_rules3<E, P>){
            return query(e, p);
        } else {
            static_assert(
                Query_rules1<E, P> ||
                Query_rules2<E, P> ||
                Query_rules3<E, P>,
                "bsio::query() is ill-formed."
            );
        }
    }

    struct Query_fn {
        template <typename E, typename P>
        constexpr decltype(auto) operator()(E e, P p) const
        requires Query_rules0<E, P>
                    && (Query_rules1<E, P> ||
                        Query_rules2<E, P> ||
                        Query_rules3<E, P>) {
            return query_lookup(e, p);
        }
    };

} // namespace cpo_detail
} // namespace bsio
