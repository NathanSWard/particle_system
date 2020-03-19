#ifndef PFX_ALGORITHM_HPP
#define PFX_ALGORITHM_HPP
#pragma once

#include <algorithm>
#include <entt/entt.hpp>
#include <functional>

namespace pfx {

namespace detail {

template<class... Ts>
struct sink {};

template<class... Ts>
struct remove_empty_impl;

template<class... Ts, class T, class... Rest>
struct remove_empty_impl<sink<Ts...>, T, Rest...> {
    using type = std::conditional_t<std::is_empty_v<T>,
            typename remove_empty_impl<sink<Ts...>, Rest...>::type,
            typename remove_empty_impl<sink<Ts..., T>, Rest...>::type>;
};

template<class... Ts>
struct remove_empty_impl<sink<Ts...>> {
    using type = sink<Ts...>;
};

template<class... Ts>
struct remove_empty {
    using type = typename remove_empty_impl<sink<>, Ts...>::type;
};

template<class... Ts>
using remove_empty_t = typename remove_empty<Ts...>::type;

template<class View, class Fn, class... Components>
inline void entt_for_each_impl(View&& v, Fn&& fn, sink<Components...>) {
    std::for_each(v.begin(), v.end(), [&](entt::entity const e) {
        std::invoke(fn, std::forward<View>(v).template get<Components>(e)...);
    });
}

template<class ExPol, class View, class Fn, class... Components>
inline void entt_for_each_impl(ExPol&& ep, View&& v, Fn&& fn, sink<Components...>) {
    std::for_each(std::forward<ExPol>(ep), v.begin(), v.end(), [&](entt::entity const e) {
        std::invoke(fn, std::forward<View>(v).template get<Components>(e)...);
    });
}

} // namespace detail

template<template<class...> class View, class Fn, class E, class Ex, class... Components>
inline void entt_for_each(View<E, Ex, Components...> const& v, Fn&& fn) {
    detail::entt_for_each_impl(v, std::forward<Fn>(fn), detail::remove_empty_t<Components...>{});
}

template<template<class...> class View, class Fn, class E, class Ex, class... Components>
inline void entt_for_each(View<E, Ex, Components...>& v, Fn&& fn) {
    detail::entt_for_each_impl(v, std::forward<Fn>(fn), detail::remove_empty_t<Components...>{});
}

template<class ExPol, template<class...> class View, class Fn, class E, class Ex, class... Components>
inline void entt_for_each(ExPol&& ep, View<E, Ex, Components...> const& v, Fn&& fn) {
    detail::entt_for_each_impl(std::forward<ExPol>(ep), v, std::forward<Fn>(fn), detail::remove_empty_t<Components...>{});
}

template<class ExPol, template<class...> class View, class Fn, class E, class Ex, class... Components>
inline void entt_for_each(ExPol&& ep, View<E, Ex, Components...>& v, Fn&& fn) {
    detail::entt_for_each_impl(std::forward<ExPol>(ep), v, std::forward<Fn>(fn), detail::remove_empty_t<Components...>{});
}

} // namespace pfx

#endif //PFX_ALGORITHM_HPP
