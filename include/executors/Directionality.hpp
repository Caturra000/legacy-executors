#pragma once
#include "impl/polymorphic_impl.hpp"
#include "impl/directionality_impl.hpp"
namespace bsio {
namespace execution {

// 1.3.4.1 Directionality properties
struct Directionality: impl::directionality_impl::Top_class_property<Directionality> {
    struct Oneway
        : impl::directionality_impl::Property<Oneway>
    {
        template <typename ...Supportable_properties>
        class Polymorphic_executor_type;
    };

    // TODO Polymorphic_executor_type
    struct Twoway
        : impl::directionality_impl::Property<Twoway> {};
    
    // TODO
    struct Then
        : impl::directionality_impl::Property<Then> {};
    
    // TODO struct Co_await;

    inline static constexpr Oneway oneway {};
    inline static constexpr Twoway twoway {};
    inline static constexpr Then then {};
};

inline constexpr Directionality directionality {};

template <typename Property>
concept Directionality_property = impl::directionality_impl::Directionality_property<Property>;

template <typename ...Supportable_properties>
class Directionality::Oneway::Polymorphic_executor_type {
public:
    Polymorphic_executor_type(): _pimpl(nullptr) {}

    Polymorphic_executor_type(std::nullptr_t): _pimpl(nullptr) {}

    ~Polymorphic_executor_type() { _pimpl ? _pimpl->destroy() : (void)0; }

    Polymorphic_executor_type(const Polymorphic_executor_type &e)
        : _pimpl(e._pimpl ? e._pimpl->clone() : nullptr) {}

    Polymorphic_executor_type(Polymorphic_executor_type &&e)
        : _pimpl(e._pimpl) { e._pimpl = nullptr; }

    template <typename Executor,
        typename Impl = polymorphic_impl::Polymorphic_executor<
            std::decay_t<Executor>, Supportable_properties...>,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<Executor>, Polymorphic_executor_type>>>
    Polymorphic_executor_type(Executor &&e)
        : _pimpl(new Impl(std::forward<Executor>(e))) {}

    Polymorphic_executor_type& operator=(std::nullptr_t) {
        this->~Polymorphic_executor_type();
        return *this;
    }

    Polymorphic_executor_type& operator=(const Polymorphic_executor_type &e) {
        if(&e == this) return *this;
        this->~Polymorphic_executor_type();
        _pimpl = e._pimpl->clone();
        return *this;
    }

    Polymorphic_executor_type& operator=(Polymorphic_executor_type &&e) {
        if(&e == this) return *this;
        this->~Polymorphic_executor_type();
        _pimpl = e._pimpl;
        e._pimpl = nullptr;
        return *this;
    }

    template <typename Executor>
    Polymorphic_executor_type& operator=(Executor &&e) {
        return this->operator=(Polymorphic_executor_type(std::forward<Executor>(e)));
    }

    Polymorphic_executor_type require(auto property) const {
        return _pimpl->require(typeid(property), std::addressof(property));
    }

    Polymorphic_executor_type prefer(auto property) const {
        return _pimpl->prefer(typeid(property), std::addressof(property));
    }

public:
    void execute(std::invocable auto &&f) { _pimpl->execute(std::forward<decltype(f)>(f)); }

private:
    Polymorphic_executor_type(polymorphic_impl::Polymorphic_executor_base *pimpl): _pimpl(pimpl) {}

private:
    // Note: `_pimpl` is a reference-counted pointer, don't delete(_pimpl) in destructor
    polymorphic_impl::Polymorphic_executor_base *_pimpl;
};

} // namespace execution

template <typename T, typename P>
struct Is_applicable_property;

template <typename T, execution::Directionality_property P>
struct Is_applicable_property<T, P>: std::true_type {};

template <typename T>
struct Is_applicable_property<T, execution::Directionality>: std::true_type {};

} // namespace bsio
