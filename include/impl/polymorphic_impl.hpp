#pragma once
#include <atomic>
#include "Functions.hpp"
namespace bsio {
namespace execution {
namespace polymorphic_impl {

struct Polymorphic_executor_base {
    using Function = bsio::impl::Function;
    using Base = Polymorphic_executor_base;
    virtual ~Polymorphic_executor_base() {}
    virtual Base* clone() const = 0;
    virtual void destroy() = 0;
    virtual const void* target() const = 0;
    virtual void* target() = 0;
    virtual const std::type_info& target_type() const = 0;
    virtual bool equals(const Polymorphic_executor_base *ex) const = 0;

    virtual void execute(Function f) = 0;
    virtual Base* require(const std::type_info&, const void *p) const = 0;
    virtual Base* prefer(const std::type_info&, const void *p) const = 0;
    virtual void* query(const std::type_info&, const void *p) const = 0;
};

template <typename Executor_impl, typename ...Supportable_properties>
class Polymorphic_executor: public Polymorphic_executor_base {
public:
    explicit Polymorphic_executor(const Executor_impl &ex): _ex_impl(ex) {}
    explicit Polymorphic_executor(Executor_impl &&ex): _ex_impl(std::move(ex)) {}
public:
    Polymorphic_executor_base* clone() const override {
        auto ptr = const_cast<Polymorphic_executor*>(this);
        _refcount.fetch_add(1, std::memory_order_relaxed);
        return ptr;
    }

    void destroy() override {
        if(0 == _refcount.fetch_add(-1, std::memory_order_acq_rel)) {
            delete this;
        }
    }

    const void* target() const override {
        return &_ex_impl;
    }

    void* target() override {
        return &_ex_impl;
    }

    const std::type_info& target_type() const override {
        return typeid(_ex_impl);
    }

    bool equals(const Polymorphic_executor_base *ex) const override {
        if(ex == this) {
            return true;
        }
        if(target_type() == ex->target_type()) {
            return _ex_impl == *static_cast<const Executor_impl *>(ex->target());
        }
        return false;
    }

    void execute(Function f) override {
        _ex_impl.execute(std::move(f));
    }

    Polymorphic_executor_base* require(const std::type_info &t, const void *p) const override {
        return require_impl(t, p);
    }

    Polymorphic_executor_base* prefer(const std::type_info &t, const void *p) const override {
        return prefer_impl(t, p);
    }

    // TODO
    void* query(const std::type_info&, const void *p) const override {
        return nullptr;
    }

private:
    template <size_t I = 0>
    Polymorphic_executor_base* require_impl(const std::type_info &t, const void *p) const {
        constexpr size_t size = sizeof...(Supportable_properties);
        if constexpr (size == 0 || I >= size) {
            return nullptr;
        } else {
            using As_tuple = std::tuple<Supportable_properties...>;
            using I_type = std::tuple_element_t<I, As_tuple>;
            if constexpr (requires(Executor_impl e, I_type i) {
                bsio::require(e, i);
            }) {
                if(t == typeid(I_type)) {
                    using New_executor_impl = decltype(bsio::require(_ex_impl, *static_cast<const I_type *>(p)));
                    return new Polymorphic_executor<New_executor_impl, Supportable_properties...>(
                        bsio::require(_ex_impl, *static_cast<const I_type *>(p)));
                }
            }
            return require_impl<I+1>(t, p);
        }
    }

    template <size_t I = 0>
    Polymorphic_executor_base* prefer_impl(const std::type_info &t, const void *p) const {
        constexpr size_t size = sizeof...(Supportable_properties);
        if constexpr (size == 0 || I >= size) {
            return this->clone();
        } else {
            using As_tuple = std::tuple<Supportable_properties...>;
            using I_type = std::tuple_element_t<I, As_tuple>;
            if constexpr (requires(Executor_impl e, I_type i) {
                bsio::prefer(e, i);
            }) {
                if(t == typeid(I_type)) {
                    using New_executor_impl = decltype(bsio::prefer(_ex_impl, *static_cast<const I_type *>(p)));
                    return new Polymorphic_executor<New_executor_impl, Supportable_properties...>(
                        bsio::prefer(_ex_impl, *static_cast<const I_type *>(p)));
                }
            }
            return prefer_impl<I+1>(t, p);
        }
    }

private:
    Executor_impl _ex_impl;
    mutable std::atomic<int> _refcount;
};
    
} // namespace polymorphic_impl
} // namespace execution
} // namespace bsio
