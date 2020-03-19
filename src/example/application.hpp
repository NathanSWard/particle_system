#ifndef PFX_EXAMPLE_APPLICATION_HPP
#define PFX_EXAMPLE_APPLICATION_HPP
#pragma once

#include <entt/entt.hpp>
#include <string_view>

#include "constants.hpp"
#include "particle_systems.hpp"
#include "sdl/sdl.hpp"

class application : sdl::Instance {
public:
    explicit application(std::string_view const name)
        : sdl::Instance()
        , win_(name, window_values::width::actual, window_values::height::actual)
        , ren_(win_) {
        SDL_RenderSetLogicalSize(ren_, window_values::width::logical, window_values::height::logical);
    }

    void run() {
        manager::init(reg_);
        manager::create_emitter<snow_system>(reg_);
        manager::create_emitter<explosion>(reg_);

        using clock_type = std::chrono::steady_clock;
        auto current_time = clock_type::now();

        while (running_) {
            input();

            auto const new_time = clock_type::now();
            auto frame_time = new_time - current_time;
            current_time = new_time;

            update(frame_time);
            render();
        }
    }

private:
    using manager = pfx::basic_particle_system_manager<my_components, snow_system, explosion>;

    void input() {
        while (SDL_PollEvent(&e_)) {
            switch (e_.type) {
                case SDL_QUIT:
                    running_ = false;
                    break;
                default:
                    break;
            }
        }
    }

    void update(std::chrono::duration<float> const dt) {
        auto group = reg_.group<position, velocity>();
        group.each([&dt](auto& pos, auto const& vel){
            pos.x += vel.dx * dt.count();
            pos.y += vel.dy * dt.count();
        });

        manager::update(reg_, dt);
    }

    void render() {
        SDL_SetRenderDrawColor(ren_, 0, 0, 0, 255);
        SDL_RenderClear(ren_);

        auto view = reg_.view<pfx::particle_tag, position, color>(entt::exclude<pfx::inactive_tag>);
        view.less([&](auto const& pos, auto const& col){
            auto const [r, g, b, a] = col.as_255();
            SDL_SetRenderDrawColor(ren_, r, g, b, a);
            SDL_RenderDrawPointF(ren_, pos.x, pos.y);
        });

        SDL_RenderPresent(ren_);
    }

private:
    entt::registry reg_{};
    sdl::Window win_;
    sdl::Renderer ren_;
    SDL_Event e_{};
    bool running_ = true;
};

#endif //PFX_EXAMPLE_APPLICATION_HPP
