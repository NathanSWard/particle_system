#ifndef PFX_DEBUG_HPP
#define PFX_DEBUG_HPP
#pragma once

#ifndef NDEBUG

#include <cassert>
#include <iostream>

template<class T, class... Msg>
inline void PFX_ASSERT(T const& t, Msg&&... msg) {
    if (!t) {
        std::cerr << "PFX_ASSERT failed: ";
        (std::cerr << ... << std::forward<Msg>(msg)) << '\n';
        assert(false);
    }
}

template<class T, class U, class... Msg>
inline void PFX_ASSERT_EQ(T const& t, U const& u, Msg&&... msg) {
    if (t != u) {
        std::cerr << "PFX_ASSERT_EQ failed: ";
        (std::cerr << ... << std::forward<Msg>(msg)) << '\n';
        assert(false);
    }
}

template<class... Msg>
inline void PFX_LOG(Msg&&... msg) {
    std::clog << "PFX_LOG: ";
    (std::clog << ... << std::forward<Msg>(msg)) << '\n';
}

template<class... Args>
inline void PFX_LOGF(char const* const fmt, Args&&... args) {
    std::printf(fmt, std::forward<Args>(args)...);
}

#else // NDEBUG

#define PFX_ASSERT(...) static_cast<void>(0)
#define PFX_ASSERT_EQ(...) static_cast<void>(0)
#define PFX_LOG(...) static_cast<void>(0)
#define PFX_LOGF(...) static_cast<void>(0)

#endif // NDEBUG

#endif //PFX_DEBUG_HPP
