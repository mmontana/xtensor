/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XUTILS_HPP
#define XUTILS_HPP

#include <cstddef>
#include <array>
#include <utility>
#include <tuple>
#include <type_traits>
#include <initializer_list>
#include <algorithm>

namespace xt
{

    template <class T>
    struct remove_class;

    template <class F, class... T>
    void for_each(F&& f, std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()));

    template <class F, class R, class... T>
    R accumulate(F&& f, R init, const std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()));

    template <class... T>
    struct or_;

    template <std::size_t I, class... Args>
    constexpr decltype(auto) argument(Args&&... args) noexcept;

    template<class R, class F, class... S>
    R apply(std::size_t index, F&& func, const std::tuple<S...>& s) noexcept(noexcept(std::declval<F>()));

    template <class U>
    struct initializer_dimension;

    /*******************************
     * remove_class implementation *
     *******************************/

    template <class T>
    struct remove_class
    {
    };

    template <class C, class R, class... Args>
    struct remove_class<R (C::*) (Args...)>
    {
        typedef R type(Args...);
    };

    template <class C, class R, class... Args>
    struct remove_class<R (C::*) (Args...) const>
    {
        typedef R type(Args...);
    };

    template <class T>
    using remove_class_t = typename remove_class<T>::type;

    /***************************
     * for_each implementation *
     ***************************/

    namespace detail
    {
        template <std::size_t I, class F, class... T>
        inline typename std::enable_if<I == sizeof...(T), void>::type
        for_each_impl(F&& /*f*/, std::tuple<T...>& /*t*/) noexcept(noexcept(std::declval<F>()))
        {
        }

        template <std::size_t I, class F, class... T>
        inline typename std::enable_if<I < sizeof...(T), void>::type
        for_each_impl(F&& f, std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()))
        {
            f(std::get<I>(t));
            for_each_impl<I + 1, F, T...>(std::forward<F>(f), t);
        }
    }

    template <class F, class... T>
    inline void for_each(F&& f, std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()))
    {
        detail::for_each_impl<0, F, T...>(std::forward<F>(f), t);
    }

    /*****************************
     * accumulate implementation *
     *****************************/

    namespace detail
    {
        template <std::size_t I, class F, class R, class... T>
        inline std::enable_if_t<I == sizeof...(T), R>
        accumulate_impl(F&& /*f*/, R init, const std::tuple<T...>& /*t*/) noexcept(noexcept(std::declval<F>()))
        {
            return init;
        }

        template <std::size_t I, class F, class R, class... T>
        inline std::enable_if_t<I < sizeof...(T), R>
        accumulate_impl(F&& f, R init, const std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()))
        {
            R res = f(init, std::get<I>(t));
            return accumulate_impl<I + 1, F, R, T...>(std::forward<F>(f), res, t);
        }
    }

    template <class F, class R, class... T>
    inline R accumulate(F&& f, R init, const std::tuple<T...>& t) noexcept(noexcept(std::declval<F>()))
    {
        return detail::accumulate_impl<0, F, R, T...>(f, init, t);
    }

    /**********************
     * or_ implementation *
     **********************/

    template <class T>
    struct or_<T> : std::integral_constant<bool, T::value>
    {
    };

    template <class T, class... Ts>
    struct or_<T, Ts...>
        : std::integral_constant<bool, T::value || or_<Ts...>::value>
    {
    };

    /***************************
     * argument implementation *
     ***************************/

    namespace detail
    {
        template <std::size_t I>
        struct getter
        {
            template <class Arg, class... Args>
            static constexpr decltype(auto) get(Arg&& /*arg*/, Args&&... args) noexcept
            {
                return getter<I - 1>::get(std::forward<Args>(args)...);
            }
        };

        template <>
        struct getter<0>
        {
            template <class Arg, class... Args>
            static constexpr Arg&& get(Arg&& arg, Args&&... /*args*/) noexcept
            {
                return std::forward<Arg>(arg);
            }
        };
    }

    template <std::size_t I, class... Args>
    constexpr decltype(auto) argument(Args&&... args) noexcept
    {
        static_assert(I < sizeof...(Args), "I should be lesser than sizeof...(Args)");
        return detail::getter<I>::get(std::forward<Args>(args)...);
    }

    /************************
     * apply implementation *
     ************************/

    namespace detail
    {
        template <class R, class F, std::size_t I, class... S>
        R apply_one(F&& func, const std::tuple<S...>& s) noexcept(noexcept(std::declval<F>()))
        {
            return func(std::get<I>(s));
        }

        template <class R, class F, std::size_t... I, class... S>
        R apply(std::size_t index, F&& func, std::index_sequence<I...> /*seq*/, const std::tuple<S...>& s) noexcept(noexcept(std::declval<F>()))
        {
            using FT = std::add_pointer_t<R(F&&, const std::tuple<S...>&)>;
            static const std::array<FT, sizeof...(I)> ar = { &apply_one<R, F, I, S...>... };
            return ar[index](std::forward<F>(func), s);
        }
    }

    template<class R, class F, class... S>
    inline R apply(std::size_t index, F&& func, const std::tuple<S...>& s) noexcept(noexcept(std::declval<F>()))
    {
        return detail::apply<R>(index, std::forward<F>(func), std::make_index_sequence<sizeof...(S)>(), s);
    }

    /******************************
     * nested_copy implementation *
     ******************************/

    template <class T, class S>
    inline void nested_copy(T&& iter, const S& s)
    {
        *iter++ = s;
    }

    template <class T, class S>
    inline void nested_copy(T&& iter, std::initializer_list<S> s)
    {
        for (auto it = s.begin(); it != s.end(); ++it)
        {
            nested_copy(std::forward<T>(iter), *it);
        }
    }

    /****************************************
     * initializer_dimension implementation *
     ****************************************/

    namespace detail
    {
        template <class U>
        struct initializer_depth_impl
        {
            static constexpr std::size_t value = 0;
        };

        template <class T>
        struct initializer_depth_impl<std::initializer_list<T>>
        {
            static constexpr std::size_t value = 1 + initializer_depth_impl<T>::value;
        };
    }

    template <class U>
    struct initializer_dimension
    {
        static constexpr std::size_t value = detail::initializer_depth_impl<U>::value;
    };

    /************************************
     * initializer_shape implementation *
     ************************************/

    namespace detail
    {
        template <std::size_t I>
        struct initializer_shape_impl
        {
            template <class T>
            static constexpr std::size_t value(T t)
            {
                return t.size() == 0 ? 0 : initializer_shape_impl<I - 1>::value(*t.begin());
            }
        };

        template <>
        struct initializer_shape_impl<0>
        {
            template <class T>
            static constexpr std::size_t value(T t)
            {
                return t.size();
            }
        };

        template <class R, class U, std::size_t... I>
        constexpr R initializer_shape(U t, std::index_sequence<I...>)
        {
             return { initializer_shape_impl<I>::value(t)... };
        }
    }

    template <class R, class T>
    constexpr R shape(T t)
    {
        return detail::initializer_shape<R, decltype(t)>(t, std::make_index_sequence<initializer_dimension<decltype(t)>::value>());
    }

    /******************************
     * check_shape implementation *
     ******************************/

    namespace detail
    {
        template <class T, class S>
        struct predshape
        {
            constexpr predshape(S first, S last): m_first(first), m_last(last)
            {}

            constexpr bool operator()(const T&) const
            {
                return m_first == m_last;
            }

            S m_first;
            S m_last;
        };

        template <class T, class S>
        struct predshape<std::initializer_list<T>, S>
        {
            constexpr predshape(S first, S last): m_first(first), m_last(last)
            {}

            constexpr bool operator()(std::initializer_list<T> t) const
            {
                return *m_first == t.size() && std::all_of(t.begin(), t.end(), predshape<T, S>(m_first + 1, m_last));
            }

            S m_first;
            S m_last;
        };
    }

    template <class T, class S>
    constexpr bool check_shape(T t, S first, S last)
    {
        return detail::predshape<decltype(t), S>(first, last)(t);
    }

    /***********************************
     * resize_container implementation *
     ***********************************/

    template <class C>
    inline bool resize_container(C& c, typename C::size_type size)
    {
        c.resize(size);
        return true;
    }

    template <class T, std::size_t N>
    inline bool resize_container(std::array<T, N>& /*a*/, typename std::array<T, N>::size_type size)
    {
        return size == N;
    }

    /**************
     * make_shape *
     **************/
    namespace detail
    {
        template <class S>
        struct shape_builder
        {
            using value_type = typename S::value_type;
            using size_type = typename S::size_type;

            inline static S make(size_type size, value_type v)
            {
                return S(size, v);
            }
        };

        template <class T, std::size_t N>
        struct shape_builder<std::array<T, N>>
        {
            using shape_type = std::array<T, N>;
            using value_type = typename shape_type::value_type;
            using size_type = typename shape_type::size_type;

            inline static shape_type make(size_type /*size*/, value_type v)
            {
                shape_type s;
                s.fill(v);
                return s;
            }
        };
    }

    template <class S>
    inline S make_shape(typename S::size_type size, typename S::value_type v)
    {
        return detail::shape_builder<S>::make(size, v);
    }
}

#endif

