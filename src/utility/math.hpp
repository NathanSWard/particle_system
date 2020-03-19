#ifndef PFX_MATH_HPP
#define PFX_MATH_HPP
#pragma once

#include <limits>
#include <cmath>

namespace math {

inline float lerp(float const a, float const b, float t) {
    return a + (b - a) * t;
}
inline float magnitude(float const x, float const y) noexcept {
    return std::hypotf(x, y);
}
inline constexpr float sq(float const x) noexcept {
    return x * x;
}
inline constexpr float cub(float const x) noexcept {
    return x * x * x;
}
inline float distance(float const x1, float const y1, float const x2, float const y2) noexcept {
    return std::hypotf(sq(x2 - x1), sq(y2 - y1));
}
inline bool feq(float a, float b) noexcept {
    float const diff = fabsf(a - b);
    a = fabsf(a);
    b = fabsf(b);
    float const largest = b > a ? b : a;
    return diff < (largest * std::numeric_limits<float>::epsilon());
}
inline bool fgeq(float const a, float const b) noexcept {
    return a > b ? true : feq(a, b);
}
inline bool fleq(float const a, float const b) noexcept {
    return a < b ? true : feq(a, b);
}

} // namespace math

#endif //PFX_MATH_HPP
