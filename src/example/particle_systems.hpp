#ifndef PFX_EXAMPLE_PARTICLE_SYSTEMS_HPP
#define PFX_EXAMPLE_PARTICLE_SYSTEMS_HPP

#include "constants.hpp"
#include "particle_components.hpp"
#include <pfx/pfx.hpp>

#include <cmath>

using my_components = pfx::particle_components<position, velocity, color>;

struct snow_system {
    inline static constexpr std::size_t max_particles = 600;
    inline static constexpr int emission_rate = 50;

    inline static constexpr auto initial_lifetime() noexcept {
        return std::chrono::seconds{11};
    }

    inline static auto initial(pfx::arg<position>) {
        return position{frng<0, window_values::width::logical>::rand(), 0.f};
    }
    inline static auto initial(pfx::arg<velocity>) {
        return velocity{0.f, frng<15, 20>::rand()};
    }
    inline static auto initial(pfx::arg<color>) {
        static constexpr auto min = +[]{return .75f;};
        static constexpr auto max = +[]{return 1.f;};
        return color{1.f, 1.f, 1., frng<min, max>::rand()};
    }
};

struct explosion {
    inline static constexpr std::size_t max_particles = 200;
    inline static constexpr int emission_rate = 200;
    inline static constexpr auto duration = std::chrono::seconds{1};

    inline static constexpr auto initial_lifetime() noexcept {
        return std::chrono::seconds{3};
    }

    inline static constexpr auto initial(pfx::arg<position>) noexcept {
        return position{window_values::width::logical/2, window_values::height::logical/2};
    }
    inline static auto initial(pfx::arg<velocity>) noexcept {
        return velocity{frng<-10, 10>::rand(), frng<-10, 10>::rand()};
    }
    inline static constexpr auto initial(pfx::arg<color>) noexcept {
        return color::red();
    }
    inline static constexpr void over_lifetime(color& c, float const perc) noexcept {
        c = color::lerp(color::red(), color::yellow(), perc);
    }

    inline static constexpr void over_time(velocity& v, std::chrono::duration<float> const dt) {
        v.dx *= 1.001f;
        v.dy *= 1.001f;
    }
};

#endif //PFX_EXAMPLE_PARTICLE_SYSTEMS_HPP
