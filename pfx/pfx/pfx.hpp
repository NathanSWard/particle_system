#ifndef PFX_HPP
#define PFX_HPP
#pragma once

#include <chrono>
#include <cmath>
#include <entt/entt.hpp>
#include <type_traits>

#include "debug.hpp"
#include "ring_buffer.hpp"

namespace pfx {

template<class...> struct particle_components {};
template<class T> struct arg {};
using time_type = std::chrono::duration<float>;

namespace internal {

struct particle_t {};
struct inactive_t {};
struct destroy_t {};
struct particle_lifetime {
    float remaining = 0.f; // [0, 1]
};
struct total_lifetime {
    template<class Rep, class Period>
    explicit constexpr total_lifetime(std::chrono::duration<Rep, Period> const& dur) noexcept
            : dur(dur) {}

    time_type dur;
};
struct emitter_lifetime {
    template<class Rep, class Period>
    explicit constexpr emitter_lifetime(std::chrono::duration<Rep, Period> const& dur) noexcept
            : dur(dur) {}

    time_type dur{0};
};
struct emission_rate {
    int rate = 0;
    int emitted = 0;
};
template<std::size_t N>
struct particle_pool {
    ring_buffer<entt::entity, N> pool;
};

} // namespace internal

using particle_tag = internal::particle_t;
using inactive_tag = internal::inactive_t;

/*
struct particle_system_policy_template {
    inline static constexpr std::size_t max_particles = 0;
    inline static constexpr time_type duration {0};
    inline static constexpr std::uint32_t emission_rate = 0;
    inline static constexpr auto initial_lifetime();
    inline static constexpr auto initial_lifetime(entt::entity, entt::registry&);
    inline static constexpr auto initial(arg<T>);
    inline static constexpr auto initial(arg<T>, entt::entity, entt::registry&);
    inline static constexpr void over_lifetime(T&, float);
    inline static constexpr void over_time(T&, time_type);
    inline static constexpr void stop_action();
};
*/

namespace detail {

template<class...> struct sink {};

template<class T>
struct always_false { static constexpr bool value = false; };

template<class T, class... Ts>
struct find_in_pack;

template<class T>
struct find_in_pack<T> : std::false_type {};

template<class T, class U, class... Rest>
struct find_in_pack<T, U, Rest...> {
    static constexpr bool value = [] {
        if constexpr (std::is_same_v<T, U>)
            return true;
        else
            return find_in_pack<T, Rest...>::value;
    }();
};

// unique pack
template<typename T, typename...>
struct unique_pack {
    using type = T;
};

template<typename... Ts, typename U, typename... Us>
struct unique_pack<sink<Ts...>, U, Us...>
        : std::conditional_t<(std::is_same_v<U, Ts> || ...)
                , unique_pack<sink<Ts...>, Us...>
                , unique_pack<sink<Ts..., U>, Us...>> {};

template<typename... Ts>
using unique_pack_t = typename unique_pack<sink<>, Ts...>::type;

// is constexpr
constexpr void _pfx_is_constexpr_helper(...) {}
#define PFX_IS_CONSTEXPR(...) noexcept(_pfx_is_constexpr_helper(__VA_ARGS__))

// has max particles
template<class System, class = int>
struct has_max_particles : std::false_type {};

template<class System>
struct has_max_particles<System, decltype(System::max_particles, 0)> : std::true_type {
    static_assert(std::is_convertible_v<decltype(System::max_particles), std::size_t>,
            "SystemPolicy::max_particles must be convertible to std::size_t");
};

// has rate over time
template<class, class = int>
struct has_emission_rate : std::false_type {};

template<class System>
struct has_emission_rate<System, decltype(System::emission_rate, 0)> : std::true_type {
    static_assert(std::is_convertible_v<decltype(System::emission_rate), int> );
};

template<class System>
static constexpr bool has_emission_rate_v = has_emission_rate<System>::value;

// has emitter duration
template<class, class = int>
struct has_emitter_duration : std::false_type {};

template<class System>
struct has_emitter_duration<System, decltype(System::duration, 0)> : std::true_type {
    static_assert(std::is_constructible_v<internal::emitter_lifetime, decltype(System::duration)>,
            "SystemPolicy::emitter_lifetime should be of type std::chrono::duration");
};

template<class System>
static constexpr bool has_emitter_duration_v = has_emitter_duration<System>::value;

template<class, class, class = int>
struct system_initial : std::false_type { static constexpr auto params = -1; };

template<class System, class T>
struct system_initial<System, T, decltype(System::initial(arg<T>{}, entt::entity {},
                                                          std::declval<entt::registry&>()), 0)> : std::true_type {
    static constexpr auto params = 3;
};

template<class System, class T>
struct system_initial<System, T, decltype(System::initial(arg<T>{}), 0)> : std::true_type {
    static constexpr auto params = 1;
};

template<class System, class T>
inline static constexpr auto system_initial_params = system_initial<System, T>::params;
template<class System, class T>
inline static constexpr auto system_initial_v = system_initial<System, T>::value;

template<class System, class = int>
struct system_initial_lifetime : std::false_type {
    static constexpr auto params = -1;
};

template<class System>
struct system_initial_lifetime<System, decltype(System::initial_lifetime(), 0)> : std::true_type {
    static_assert(std::is_constructible_v<internal::total_lifetime, decltype(System::initial_lifetime())>,
                  "SystemPolicy's initial_lifetime should return a std::chrono::duration");
    static constexpr auto params = 0;
};

template<class System>
struct system_initial_lifetime<System, decltype(System::initial_lifetime(entt::entity{},
                                                std::declval<entt::registry&>()), 0)> : std::true_type {
    static_assert(std::is_constructible_v<internal::total_lifetime, decltype(System::initial_lifetime(entt::entity{},
                  std::declval<entt::registry&>()))>,
                  "SystemPolicy's initial_lifetime should return a std::chrono::duration");
    static constexpr auto params = 2;
};

template<class System>
inline static constexpr auto system_initial_lifetime_v = system_initial_lifetime<System>::value;

template<class System>
inline static constexpr auto system_initial_lifetime_params = system_initial_lifetime<System>::params;

// has over lifetime
template<class, class, class = int>
struct has_over_lifetime : std::false_type {};

template<class System, class T>
struct has_over_lifetime < System, T, decltype(System::over_lifetime(std::declval<T&>(), float {}), 0) >
        : std::true_type {};

template<class System, class T>
inline static constexpr bool has_over_lifetime_v = has_over_lifetime<System, T>::value;

// has over time
template<class, class, class = int>
struct has_over_time : std::false_type {};

template<class System, class T>
struct has_over_time <System, T, decltype(System::over_time(std::declval<T&>(), time_type{}), 0)>
        : std::true_type {};

template<class System, class T>
inline static constexpr bool has_over_time_v = has_over_time<System, T>::value;

// has constexpr lifetime
template<class, class = int>
struct has_constexpr_lifetime : std::false_type{};

template<class System>
struct has_constexpr_lifetime<System, decltype(System::initial_lifetime(), 0)> {
    static constexpr bool value = PFX_IS_CONSTEXPR(System::initial_lifetime());
};

template<class System>
struct has_constexpr_lifetime<System, decltype(System::initial_lifetime(entt::entity{},
                                               std::declval<entt::registry&>()), 0)> {
    static constexpr bool value =
            PFX_IS_CONSTEXPR(System::initial_lifetime(entt::entity{}, std::declval<entt::registry&>()));
};

template<class System>
inline static constexpr bool has_constexpr_lifetime_v = has_constexpr_lifetime<System>::value;

template<std::size_t I, class T, class... Ts>
struct variadic_index_impl {
    static_assert(always_false<T>::value, "variadic_index<T, Ts...> T not found in Ts...");
};

template<std::size_t I, class T, class U, class... Rest>
struct variadic_index_impl<I, T, U, Rest...> {
    static constexpr std::size_t value = [] {
        if constexpr (std::is_same_v<T, U>)
            return I;
        else
            return variadic_index_impl<I + 1, T, Rest...>::value;
    }();
};

template<class T, class... Ts>
struct variadic_index {
    static_assert(sizeof...(Ts) > 0);
    static constexpr std::size_t value = variadic_index_impl<0, T, Ts...>::value;
};

template<class T, class... Ts>
inline static constexpr std::size_t variadic_index_v = variadic_index<T, Ts...>::value;

template<class System, class T>
inline constexpr decltype(auto) invoke_initial([[maybe_unused]] entt::entity const emitter,
                                               [[maybe_unused]] entt::registry& r) {
    if constexpr (system_initial_params<System, T> == 1)
        return System::initial(arg<T>{});
    else if constexpr (system_initial_params<System, T> == 3)
        return System::initial(arg<T>{}, emitter, r);
    else
        static_assert(always_false<System>::value, "invalid SystemPolicy::initial function signature");
}

template<class System>
inline constexpr decltype(auto) invoke_initial_lifetime([[maybe_unused]] entt::entity const emitter,
                                                        [[maybe_unused]] entt::registry& r) {
    if constexpr (system_initial_lifetime_params<System> == 0)
        return System::initial_lifetime();
    else if constexpr (system_initial_lifetime_params<System> == 2)
        return System::initial_lifetime(emitter, r);
    else
        static_assert(always_false<System>::value, "invalid SystemPolicy::initial_lifetime function signature");
}

} // namespace detail

template<class ParticleComponents, class... SystemPolicies>
struct basic_particle_system_manager;

template<class... Components, class... Systems>
class basic_particle_system_manager<particle_components<Components...>, Systems...> {
public:
    static_assert(std::conjunction_v<std::is_empty<Systems>...>, "SystemPolicies must be empty classes");
    static_assert(std::conjunction_v<detail::has_max_particles<Systems>...>,
            "SystemPolicies must have max_particles static data member");

    inline static void init(entt::registry& r) {
        using namespace internal;
        init_impl<particle_t, particle_lifetime, total_lifetime, emitter_lifetime, emission_rate,
                  Systems..., Components...>(r, detail::unique_pack_t<particle_pool<Systems::max_particles>...>{});
        (void)r.group<particle_t, Components...>(entt::exclude<inactive_t>);
        (void)r.group<particle_lifetime, total_lifetime>(entt::exclude<inactive_t>);
        (void)r.group<particle_lifetime>(entt::exclude<inactive_t>);
        (init_groups<Systems>(r), ...);
    }

    // static duration
    // static emission rate
    template<class SystemPolicy, std::enable_if_t<detail::has_emitter_duration_v<SystemPolicy>
                                                  && detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        r.assign<emitter_lifetime>(emitter, SystemPolicy::duration);
        return emitter;
    }

    // dynamic duration
    // static emission rate
    template<class SystemPolicy, class Rep, class Period, std::enable_if_t<!detail::has_emitter_duration_v<SystemPolicy>
            && detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r, std::chrono::duration<Rep, Period> const& duration) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        r.assign<emitter_lifetime>(emitter, duration);
        return emitter;
    }

    // infinite duration
    // static emission rate
    template<class SystemPolicy, std::enable_if_t<!detail::has_emitter_duration_v<SystemPolicy>
                                                  && detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        return emitter;
    }

    // static duration
    // dynamic emission rate
    template<class SystemPolicy, std::enable_if_t<detail::has_emitter_duration_v<SystemPolicy>
                                                  && !detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r, int const rate) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        r.assign<emitter_lifetime>(emitter, SystemPolicy::duration);
        r.assign<emission_rate>(emitter, rate);
        return emitter;
    }

    // dynamic duration
    // dynamic emission rate
    template<class SystemPolicy, class Rep, class Period, std::enable_if_t<!detail::has_emitter_duration_v<SystemPolicy>
            && !detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r, std::chrono::duration<Rep, Period> const& duration,
                                              int const rate) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        r.assign<emitter_lifetime>(emitter, duration);
        r.assign<emission_rate>(emitter, rate);
        return emitter;
    }

    // infinite duration
    // dynamic emission rate
    template<class SystemPolicy, std::enable_if_t<!detail::has_emitter_duration_v<SystemPolicy>
                                                  && !detail::has_emission_rate_v<SystemPolicy>, int> = 0>
    inline static entt::entity create_emitter(entt::registry& r, int const rate) {
        using namespace internal;
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);

        auto const emitter = r.create();
        r.assign<SystemPolicy>(emitter);
        r.assign<particle_pool<SystemPolicy::max_particles>>(emitter);
        r.assign<emission_rate>(emitter, rate);
        return emitter;
    }

    template<class SystemPolicy>
    inline static bool update_emitter_emission_rate(entt::registry& r, entt::entity const emitter, int const rate) {
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);
        static_assert(!detail::has_emission_rate_v<SystemPolicy>);
        if (r.valid(emitter)) {
            PFX_ASSERT(r.has<SystemPolicy>(emitter));
            r.assign_or_replace<internal::emission_rate>(emitter,
                    internal::emission_rate{rate, int(float(rate) * second_timer_.count())});
            return true;
        }
        return false;
    }

    template<class SystemPolicy>
    inline static bool update_emitter_lifetime(entt::registry& r, entt::entity const emitter, time_type const life) {
        static_assert(std::disjunction_v<std::is_same<SystemPolicy, Systems>...>);
        if (r.valid(emitter)) {
            PFX_ASSERT(r.has<SystemPolicy>(emitter));
            r.assign_or_replace<internal::emitter_lifetime>(emitter, life);
            return true;
        }
        return false;
    }

    inline static bool valid_emitter(entt::registry& r, entt::entity const emitter) {
        if (r.valid(emitter))
            return ((r.has<Systems>(emitter) || ...));
        return false;
    }

    inline static void update(entt::registry& r, time_type const dt) {
        emit(r, dt);
        update_lifetime(r, dt);
        update_over_lifetime(r);
        update_over_time(r, dt);
    }

    template<class System, class Fn, class... Cs>
    inline static void over_lifetime(entt::registry& r, particle_components<Cs...>, Fn&& fn) {
        using namespace internal;
        static_assert(std::is_invocable_r_v<void, Fn, float, Cs...>);
        static_assert(std::disjunction_v<std::is_same<System, Systems>...>);
        static_assert(std::conjunction_v<detail::find_in_pack<Cs, Components...>...>);

        auto particles = r.view<System, particle_t, particle_lifetime, Cs...>(entt::exclude<inactive_t>);
        particles.less([fn = std::forward<Fn>(fn)](auto const& life, auto&&... cs){
            fn(life.remaining, std::forward<decltype(cs)>(cs)...);
        });
    }

    template<class System, class Fn>
    inline static void over_lifetime(entt::registry& r, Fn&& fn) {
        using namespace internal;
        static_assert(std::is_invocable_r_v<void, Fn, float>);
        static_assert(std::disjunction_v<std::is_same<System, Systems>...>);

        auto particles = r.view<System, particle_t, particle_lifetime>(entt::exclude<inactive_t>);
        particles.less([fn = std::forward<Fn>(fn)](auto const& life){
            fn(life.remaining);
        });
    }

private:
    template<class... Ts, class... Pools>
    inline static void init_impl(entt::registry& r, detail::sink<Pools...>) {
        (r.prepare<Ts>(), ...);
        (r.prepare<Pools>(), ...);
    }

    template<class System>
    inline static void init_groups(entt::registry& r) {
        using namespace internal;
        if constexpr (detail::has_constexpr_lifetime_v<System>)
            r.group<System>(entt::get<particle_t, particle_lifetime, Components...>, entt::exclude<inactive_t>);
        else
            r.group<System>(entt::get<particle_t, particle_lifetime, total_lifetime, Components...>, entt::exclude<inactive_t>);
    }

    template<class SystemPolicy>
    inline static void assign_initial_lifetime([[maybe_unused]] entt::entity const particle,
                                                [[maybe_unused]] entt::entity const emitter,
                                                [[maybe_unused]] entt::registry& r) {
        using namespace internal;
        if constexpr (detail::system_initial_lifetime_v<SystemPolicy>) {
            if constexpr (detail::has_constexpr_lifetime_v<SystemPolicy>)
                r.assign<particle_lifetime>(particle);
            else {
                r.assign<particle_lifetime>(particle);
                r.assign<total_lifetime>(particle, detail::invoke_initial_lifetime<SystemPolicy>(emitter, r));
            }
        }
    }

    template<class SystemPolicy>
    inline static void replace_initial_lifetime([[maybe_unused]] entt::entity const particle,
                                                 [[maybe_unused]] entt::entity const emitter,
                                                 [[maybe_unused]] entt::registry& r) {
        using namespace internal;
        if constexpr (detail::system_initial_lifetime_v<SystemPolicy>) {
            if constexpr (detail::has_constexpr_lifetime_v<SystemPolicy>) {
                r.replace<particle_lifetime>(particle, [](auto& l) { l.remaining = 0.f; });
            }
            else {
                auto const total_life = total_lifetime{detail::invoke_initial_lifetime<SystemPolicy>(emitter, r)};
                r.replace<particle_lifetime>(particle, [](auto& l) { l.remaining = 0.f; });
                r.replace<total_lifetime>(particle, [&](auto& l) { l.dur = total_life.dur; });
            }
        }
    }

    template<class SystemPolicy, class Component>
    inline static void assign_initial_component([[maybe_unused]] entt::entity const particle,
                                                 [[maybe_unused]] entt::entity const emitter, entt::registry& r) {
        if constexpr (detail::system_initial_v<SystemPolicy, Component>)
            r.assign<Component>(particle, detail::invoke_initial<SystemPolicy, Component>(emitter, r));
    }

    template<class SystemPolicy, class Component>
    inline static void replace_initial_component([[maybe_unused]] entt::entity const particle,
                                                  [[maybe_unused]] entt::entity const emitter,
                                                  [[maybe_unused]] entt::registry& r) {
        if constexpr (detail::system_initial_v<SystemPolicy, Component>) {
            r.replace<Component>(particle, [&](auto& comp) {
                comp = detail::invoke_initial<SystemPolicy, Component>(emitter, r);
            });
        }
    }

    template<class SystemPolicy>
    inline static void assign_initial(entt::entity const particle, entt::entity const emitter, entt::registry& r) {
        r.assign<internal::particle_t>(particle);
        r.assign<SystemPolicy>(particle);
        assign_initial_lifetime<SystemPolicy>(particle, emitter, r);
        (assign_initial_component<SystemPolicy, Components>(particle, emitter, r), ...);
    }

    template<class SystemPolicy>
    inline static void replace_initial(entt::entity const particle, entt::entity const emitter, entt::registry& r) {
        r.remove_if_exists<internal::inactive_t>(particle);
        replace_initial_lifetime<SystemPolicy>(particle, emitter, r);
        (replace_initial_component<SystemPolicy, Components>(particle, emitter, r), ...);
    }

    template<class SystemPolicy, std::size_t N>
    inline static void emit_rate_impl(entt::entity const id, entt::registry& r, internal::particle_pool<N>& pool,
                                       int const particle_count) {
        if (pool.pool.full()) {
            for (auto i = 0; i < particle_count; ++i) {
                auto const particle = pool.pool.next();
                replace_initial<SystemPolicy>(particle, id, r);
            }
        }
        else {
            auto const remaining = pool.pool.remaining();
            if (remaining >= particle_count) { // TODO: maybe check if pre-existing particles are inactive?
                for (auto i = 0; i < particle_count; ++i) {
                    auto const particle = r.create();
                    assign_initial<SystemPolicy>(particle, id, r);
                    pool.pool.push_back(particle);
                }
            }
            else {
                for (auto i = 0; i < remaining; ++i) {
                    auto const particle = r.create();
                    assign_initial<SystemPolicy>(particle, id, r);
                    pool.pool.push_back(particle);
                }
                for (auto i = 0; i < particle_count - remaining; ++i) {
                    auto const particle = pool.pool.next();
                    replace_initial<SystemPolicy>(particle, id, r);
                }
            }
        }
    }

    template<class SystemPolicy>
    inline static void emit_static_rate(entt::registry& r) {
        static constexpr auto system_index = detail::variadic_index_v<SystemPolicy, Systems...>;
        auto& emitted = emission_count_[system_index];

        int const particle_count = int((float)SystemPolicy::emission_rate * second_timer_.count()) - emitted;
        if (particle_count > 0) {
            emitted += particle_count;
            auto emitters = r.view<SystemPolicy, internal::particle_pool<SystemPolicy::max_particles>>();
            emitters.less([&](auto const id, auto& pool) {
                emit_rate_impl<SystemPolicy>(id, r, pool, particle_count);
            });
        }
    }

    template<class SystemPolicy>
    inline static void emit_dynamic_rate(entt::registry& r) {
        auto emitters = r.view<SystemPolicy, internal::particle_pool<SystemPolicy::max_particles>, internal::emission_rate>();
        emitters.less([&](auto const id, auto& pool, auto& rate) {
            int const particle_count = int((float)rate.rate * second_timer_.count()) - rate.emitted;
            if (particle_count > 0) {
                rate.emitted += particle_count;
                emit_rate_impl<SystemPolicy>(id, r, pool, particle_count);
            }
        });
    }

    template<class SystemPolicy>
    inline static void emit_impl(entt::registry& r) {
        if constexpr (detail::has_emission_rate_v<SystemPolicy>)
            emit_static_rate<SystemPolicy>(r);
        else
            emit_dynamic_rate<SystemPolicy>(r);
    }

    inline static void emit(entt::registry& r, time_type const dt) {
        second_timer_ += dt;
        (emit_impl<Systems>(r), ...);
        if (second_timer_ >= std::chrono::seconds {1}) {
            second_timer_ = std::chrono::seconds {0};
            emission_count_.fill(0);
            r.view<internal::emission_rate>().each([](auto&& rate) { rate.emitted = 0; });
        }
    }

    template<class System, class C>
    inline static void update_over_time_impl2([[maybe_unused]] entt::registry& r,
                                              [[maybe_unused]] time_type const dt) {
        if constexpr (detail::has_over_time_v<System, C>) {
            auto view = r.view<System, particle_tag, C>(entt::exclude<inactive_tag>);
            view.less([&dt](auto& c) {
                System::over_time(c, dt);
            });
        }
    }

    template<class System>
    inline static void update_over_time_impl(entt::registry& r, time_type const dt) {
        (update_over_time_impl2<System, Components>(r, dt), ...);
    }

    template<class System, class C>
    inline static void update_over_lifetime_impl2([[maybe_unused]] entt::registry& r) {
        if constexpr (detail::has_over_lifetime_v<System, C>) {
            auto group = r.view<System, particle_tag, internal::particle_lifetime, C>(entt::exclude<inactive_tag>);
            group.less([](auto const& life, auto& c){
                System::over_lifetime(c, life.remaining);
            });
        }
    }

    template<class System>
    inline static void update_over_lifetime_impl(entt::registry& r) {
        (update_over_lifetime_impl2<System, Components>(r), ...);
    }

    template<class System>
    inline static void update_lifetime_impl(entt::registry& r, time_type const dt) {
        using namespace internal;

        auto emitters = r.view<System, emitter_lifetime, internal::particle_pool<System::max_particles>>();
        emitters.less([&](auto const e, auto& life, auto& pool){
            life.dur -= dt;
            if (life.dur <= std::chrono::seconds{0}) {
                for (auto&& particle : pool.pool)
                    r.assign<destroy_t>(particle);
                r.destroy(e);
            }
        });

        if constexpr (detail::has_constexpr_lifetime_v<System>) {
            auto const change = dt / System::initial_lifetime();
            auto particles = r.group<System>(entt::get<particle_lifetime>, entt::exclude<inactive_t>);
            particles.less([&](auto const e, auto& life){
                life.remaining += change;
                if (std::isgreaterequal(life.remaining, 1.f))
                    r.assign<inactive_t>(e);
            });
        }
        else {
            auto particles = r.group<System>(entt::get<particle_lifetime, total_lifetime>, entt::exclude<inactive_t>);
            particles.less([&](auto const e, auto& life, auto const& total){
               life.remaining += (dt / total.dur);
               if (std::isgreaterequal(life.remaining, 1.f))
                   r.assign<inactive_t>(e);
            });
        }
    }

    inline static void update_lifetime(entt::registry& r, time_type const dt) {
        (update_lifetime_impl<Systems>(r, dt), ...);
        auto particles = r.view<internal::inactive_t, internal::destroy_t>();
        particles.less([&r](auto const e){
            r.destroy(e);
        });
    }

    inline static void update_over_time(entt::registry& r, time_type const dt) {
        (update_over_time_impl<Systems>(r, dt), ...);
    }

    inline static void update_over_lifetime(entt::registry& r) {
        (update_over_lifetime_impl<Systems>(r), ...);
    }

    inline static time_type second_timer_ {0};
    inline static std::array<int, sizeof...(Systems)> emission_count_ {};
        // TODO: can be optimized to only include system's w/ emission rate
};

} // namespace pfx

#endif // PFX_HPP