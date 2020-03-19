#ifndef PFX_SDL_HPP
#define PFX_SDL_HPP
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string_view>

#include <pfx/debug.hpp>

namespace sdl {

struct Instance {
    explicit Instance(std::uint32_t const flags = SDL_INIT_EVERYTHING) {
        [[maybe_unused]] auto err = SDL_Init(flags);
        PFX_ASSERT_EQ(err, 0, SDL_GetError());

        err = TTF_Init();
        PFX_ASSERT_EQ(err, 0, TTF_GetError());
    }

    Instance(Instance const&) = delete;
    Instance& operator=(Instance const&) = delete;

    ~Instance() {
        TTF_Quit();
        SDL_Quit();
    }
};

class Window {
public:
    explicit Window(std::string_view const name, int const w, int const h,
                    int const x = SDL_WINDOWPOS_CENTERED, int const y = SDL_WINDOWPOS_CENTERED,
                    uint32_t const flags = 0)
            : w_(SDL_CreateWindow(name.data(), x, y, w, h, flags))
    {
        PFX_ASSERT(w_, SDL_GetError());
    }

    ~Window() { SDL_DestroyWindow(w_); }

    constexpr operator SDL_Window*() noexcept { return w_;  }
    constexpr operator SDL_Window const*() const noexcept { return w_; }

private:
    SDL_Window* const w_;
};

class Renderer {
public:
    explicit Renderer(Window& w, int const index = -1, uint32_t const flags = 0)
            : r_(SDL_CreateRenderer(w, index, flags))
    {
        PFX_ASSERT(r_, SDL_GetError());
        SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    }

    ~Renderer() { SDL_DestroyRenderer(r_); }

    constexpr operator SDL_Renderer*() noexcept { return r_; }
    constexpr operator SDL_Renderer const*() const noexcept { return r_; }

private:
    SDL_Renderer* const r_;
};

} // namespace sdl


#endif //PFX_SDL_HPP
