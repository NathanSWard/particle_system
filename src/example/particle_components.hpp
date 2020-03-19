#ifndef PFX_EXAMPLE_PARTICLE_COMPONENTS_HPP
#define PFX_EXAMPLE_PARTICLE_COMPONENTS_HPP
#pragma once

#include <cstdint>
#include <pfx/pfx.hpp>
#include "rng.hpp"

struct position { float x, y; };
struct velocity { float dx, dy; };
struct speed { float s; };
struct rotation { float r; };
struct size { float s; };

// rgba representation of a color in the range [0, 1]
struct color {
    float r, g, b, a = 0.f;

    [[nodiscard]] constexpr auto as_255() const noexcept {
        return std::tuple{
                std::uint8_t(r * 255.999f),
                std::uint8_t(g * 255.999f),
                std::uint8_t(b * 255.999f),
                std::uint8_t(a * 255.999f)
        };
    }

    inline static constexpr color black() noexcept { return {0.f, 0.f, 0.f, 1.f}; };
    inline static constexpr color blue() noexcept { return {0.f, 0.f, 1.f, 1.f}; };
    inline static constexpr color clear() noexcept { return {0.f, 0.f, 0.f, 0.f}; };
    inline static constexpr color gray() noexcept { return {0.5f, 0.5f, 0.5f, 1.f}; };
    inline static constexpr color green() noexcept { return {0.f, 1.f, 0.f, 1.f}; };
    inline static constexpr color magenta() noexcept { return {1.f, 0.f, 1.f, 1.f}; };
    inline static constexpr color red() noexcept { return {1.f, 0.f, 0.f, 1.f}; };
    inline static constexpr color white() noexcept { return {1.f, 1.f, 1.f, 1.f}; };
    inline static constexpr color yellow() noexcept { return {1.f, 0.92f, 0.016f, 1.f}; };

    inline static constexpr color lerp(color const& a, color const& b, float const t) noexcept {
        return {
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t
        };
    }
};

#endif //PFX_EXAMPLE_PARTICLE_COMPONENTS_HPP
