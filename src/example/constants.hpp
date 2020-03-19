#ifndef PFX_EXAMPLE_CONSTANTS_HPP
#define PFX_EXAMPLE_CONSTANTS_HPP
#pragma once

namespace window_values {
namespace width {
    inline static constexpr auto actual = 1024;
    inline static constexpr auto logical = actual / 4;
} // namespace width
namespace height {
    inline static constexpr auto actual = 768;
    inline static constexpr auto logical = actual / 4;
} // namespace height
} // namespace window_values

#endif //PFX_EXAMPLE_CONSTANTS_HPP
