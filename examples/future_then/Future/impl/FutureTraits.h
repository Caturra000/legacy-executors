#ifndef __FLUENT_FUTURE_TRAITS_H__
#define __FLUENT_FUTURE_TRAITS_H__
#include <type_traits>
#include "FunctionTraits.h"
namespace fluent {

// forward
template <typename T>
class Future;

// example: FutureInner<Future<int>>::type -> int
template <typename T>
struct FutureInner {
    using Type = T;
};

template <typename T>
struct FutureInner<Future<T>> {
    using Type = typename FutureInner<T>::Type;
};

template <typename Fut, typename Func,
// some useful info
typename T = typename FutureInner<Fut>::Type,
typename Args = typename FunctionTraits<Func>::ArgsTuple>
struct IsThenValid: public std::conditional_t<
    std::is_same<std::tuple<T>, Args>::value
    || std::is_same<std::tuple<T&>, Args>::value
    || std::is_same<std::tuple<T&&>, Args>::value,
    std::true_type,
    std::false_type> {};

// T: decay type, for comparing
// Functor: then functor type
template <typename T, typename Functor>
struct IsCopyThen: public std::conditional_t<
    std::is_same<std::tuple<std::remove_reference_t<T>>, typename FunctionTraits<Functor>::ArgsTuple>::value
, std::true_type, std::false_type> {};

template <typename T, typename Functor>
struct IsReferenceThen: public std::conditional_t<
    std::is_same<std::tuple<std::remove_reference_t<T>&>, typename FunctionTraits<Functor>::ArgsTuple>::value
, std::true_type, std::false_type> {};

template <typename T, typename Functor>
struct IsMovealbeThen: public std::conditional_t<
    std::is_same<std::tuple<std::remove_reference_t<T>&&>, typename FunctionTraits<Functor>::ArgsTuple>::value
, std::true_type, std::false_type> {};


// Functor: then functor type
template <typename T, typename Functor>
struct ThenArgumentTraits {
    using Type =
            std::conditional_t<IsCopyThen<T, Functor>::value, // are you T?
                std::remove_reference_t<T>, // yes, copy
                std::conditional_t<IsReferenceThen<T, Functor>::value, // no, it is reference // are you lvalue?
                    std::remove_reference_t<T>&, // yes, it is &
                    std::remove_reference_t<T>&& // no, it is &&. IsMovealbeThen::value must be true
                >
            >;
};


// T: passed by ThenArgumentTraits::Type
template <typename T>
struct ThenArgumentTraitsConvert {
    using Type = T&;
};

template <typename T>
struct ThenArgumentTraitsConvert<T&> {
    using Type = T&;
};

template <typename T>
struct ThenArgumentTraitsConvert<T&&> {
    using Type = T&&;
};

} // fluent
#endif