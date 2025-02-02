/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2015-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_ANY_HPP
#define RANGES_V3_UTILITY_ANY_HPP

#include <memory>
#include <typeinfo>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <concepts/concepts.hpp>
#include <range/v3/utility/swap.hpp>

RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_DEPRECATED_DECLARATIONS

namespace ranges
{
    struct bad_any_cast
      : std::bad_cast
    {
        virtual const char* what() const noexcept override
        {
            return "bad any_cast";
        }
    };

    struct RANGES_DEPRECATED(
        "ranges::any will be going away in the not-too-distant future. "
        "We suggest you use std::any or boost::any instead (or simply steal "
        "this header and maintain it yourself)."
    ) any;

    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any &);

    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any const &);

    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any &&);

    template<typename T>
    T * any_cast(any *) noexcept;

    template<typename T>
    T const * any_cast(any const *) noexcept;

#if defined(RANGES_WORKAROUND_MSVC_589046) && !defined(RANGES_DOXYGEN_INVOKED)
    namespace _any_ { struct adl_hook {}; }
#endif

    struct any
#if defined(RANGES_WORKAROUND_MSVC_589046) && !defined(RANGES_DOXYGEN_INVOKED)
      : private _any_::adl_hook
#endif
    {
    private:
        template<typename T>
        friend meta::if_c<std::is_reference<T>() || (bool) Copyable<T>, T>
        any_cast(any &);

        template<typename T>
        friend meta::if_c<std::is_reference<T>() || (bool) Copyable<T>, T>
        any_cast(any const &);

        template<typename T>
        friend meta::if_c<std::is_reference<T>() || (bool) Copyable<T>, T>
        any_cast(any &&);

        template<typename T>
        friend T * any_cast(any *) noexcept;

        template<typename T>
        friend T const * any_cast(any const *) noexcept;

        struct interface
        {
            virtual ~interface()
            {}
            virtual interface *clone() const = 0;
            virtual std::type_info const & type() const noexcept = 0;
        };

        template<typename T>
        struct impl final : interface
        {
        private:
            T obj;
        public:
            impl() = default;
            impl(T o)
              : obj(std::move(o))
            {}
            T &get() { return obj; }
            T const &get() const { return obj; }
            impl *clone() const override
            {
                return new impl{obj};
            }
            std::type_info const & type() const noexcept override
            {
                return typeid(T);
            }
        };

        std::unique_ptr<interface> ptr_;
    public:
        any() noexcept = default;
        template<typename TRef, typename T = detail::decay_t<TRef>>
        CPP_ctor(any)(TRef &&t)(
            requires Copyable<T> && !Same<T, any>)
          : ptr_(new impl<T>(static_cast<TRef &&>(t)))
        {}
        any(any &&) noexcept = default;
        any(any const &that)
          : ptr_{that.ptr_ ? that.ptr_->clone() : nullptr}
        {}
        any &operator=(any &&) noexcept = default;
        any &operator=(any const &that)
        {
            ptr_.reset(that.ptr_ ? that.ptr_->clone() : nullptr);
            return *this;
        }
        template<typename TRef, typename T = detail::decay_t<TRef>>
        auto operator=(TRef &&t) -> CPP_ret(any &)(
            requires Copyable<T> && !Same<T, any>)
        {
            any{static_cast<TRef &&>(t)}.swap(*this);
            return *this;
        }
        void clear() noexcept
        {
            ptr_.reset();
        }
        bool empty() const noexcept
        {
            return !ptr_;
        }
        std::type_info const & type() const noexcept
        {
            return ptr_ ? ptr_->type() : typeid(void);
        }
        void swap(any &that) noexcept
        {
            ptr_.swap(that.ptr_);
        }

#if !defined(RANGES_WORKAROUND_MSVC_589046) || defined(RANGES_DOXYGEN_INVOKED)
        friend void swap(any &x, any &y) noexcept
        {
            x.swap(y);
        }
#endif
    };

#if defined(RANGES_WORKAROUND_MSVC_589046) && !defined(RANGES_DOXYGEN_INVOKED)
    namespace _any_
    {
        void swap(any &x, any &y) noexcept
        {
            x.swap(y);
        }
    }
#endif

    /// \throw bad_any_cast
    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any &x)
    {
        if(x.type() != typeid(detail::decay_t<T>))
            throw bad_any_cast{};
        return static_cast<any::impl<detail::decay_t<T>>*>(x.ptr_.get())->get();
    }

    /// \overload
    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any const &x)
    {
        if(x.type() != typeid(detail::decay_t<T>))
            throw bad_any_cast{};
        return static_cast<any::impl<detail::decay_t<T>> const *>(x.ptr_.get())->get();
    }

    /// \overload
    template<typename T>
    meta::if_c<std::is_reference<T>() || Copyable<T>, T>
    any_cast(any &&x)
    {
        if(x.type() != typeid(detail::decay_t<T>))
            throw bad_any_cast{};
        return static_cast<any::impl<detail::decay_t<T>>*>(x.ptr_.get())->get();
    }

    /// \overload
    template<typename T>
    T * any_cast(any *p) noexcept
    {
        if(p && p->ptr_)
            if(any::impl<T> *q = dynamic_cast<any::impl<T>*>(p->ptr_.get()))
                return &q->get();
        return nullptr;
    }

    /// \overload
    template<typename T>
    T const * any_cast(any const *p) noexcept
    {
        if(p && p->ptr_)
            if(any::impl<T> const *q = dynamic_cast<any::impl<T> const *>(p->ptr_.get()))
                return &q->get();
        return nullptr;
    }
}

RANGES_DIAGNOSTIC_POP

#endif
