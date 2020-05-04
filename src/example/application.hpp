#ifndef PFX_EXAMPLE_APPLICATION_HPP
#define PFX_EXAMPLE_APPLICATION_HPP
#pragma once

#include <entt/entt.hpp>
#include <string_view>

#include "constants.hpp"
#include "particle_systems.hpp"
#include "user_components.hpp"
#include "sdl/sdl.hpp"

class application : sdl::Instance {
public:
    explicit application(std::string_view const name)
        : sdl::Instance()
        , win_(name, window_values::width::actual, window_values::height::actual)
        , ren_(win_) {
        SDL_RenderSetLogicalSize(ren_, window_values::width::logical, window_values::height::logical);
        manager::init(reg_);
    }

    void run() {
        spray_emitter_ = manager::create_emitter<reactive_spray_system>(reg_, 30);
        manager::deactivate_emitter<reactive_spray_system>(reg_, spray_emitter_);

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
    using manager = pfx::basic_particle_system_manager<my_components, reactive_spray_system, explosion, snow_system>;

    void input() {

        // update mouse position
        int x, y;
        SDL_GetMouseState(&x, &y);
        float const fx = float(x) / window_values::width::actual * window_values::width::logical;
        float const fy = float(y) / window_values::height::actual * window_values::height::logical;
        CURSOR_POSITION = {fx, fy};

        while (SDL_PollEvent(&e_)) {
            switch (e_.type) {
                case SDL_QUIT:
                    running_ = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (e_.button.button == SDL_BUTTON_LEFT)
                        manager::activate_emitter<reactive_spray_system>(reg_, spray_emitter_);
                    else if (e_.button.button == SDL_BUTTON_RIGHT)
                        create_explosion<manager>(reg_, CURSOR_POSITION);
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (e_.button.button == SDL_BUTTON_LEFT)
                        manager::deactivate_emitter<reactive_spray_system>(reg_, spray_emitter_);
                    break;
                default:
                    break;
            }
        }

        auto const key_array = SDL_GetKeyboardState(nullptr);

        // check if user changed emission rate
        if (key_array[SDL_SCANCODE_UP])
            manager::update_emitter_emission_rate<reactive_spray_system>(reg_, spray_emitter_,
                    [](int const rate){ return std::min(rate + 1, (int)reactive_spray_system::max_particles);});
        else if (key_array[SDL_SCANCODE_DOWN])
            manager::update_emitter_emission_rate<reactive_spray_system>(reg_, spray_emitter_,
                    [](int const rate) { return std::max(rate - 1, 0);});

        // check if user changed emission color
        if (key_array[SDL_SCANCODE_1])
            CURSOR_COLOR = color::red();
        else if (key_array[SDL_SCANCODE_2])
            CURSOR_COLOR = color::orange();
        else if (key_array[SDL_SCANCODE_3])
            CURSOR_COLOR = color::yellow();
        else if (key_array[SDL_SCANCODE_4])
            CURSOR_COLOR = color::green();
        else if (key_array[SDL_SCANCODE_5])
            CURSOR_COLOR = color::blue();
        else if (key_array[SDL_SCANCODE_6])
            CURSOR_COLOR = color::magenta();

        // check if user is altering the snow system
        if (key_array[SDL_SCANCODE_Q]) {
            if (snow_emitter_ == entt::null)
                snow_emitter_ = manager::create_emitter<snow_system>(reg_);
        }
        else if (key_array[SDL_SCANCODE_W]) {
            if (snow_emitter_ != entt::null) {
                manager::destroy_emitter<snow_system>(reg_, snow_emitter_);
                snow_emitter_ = entt::null;
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
    entt::entity spray_emitter_ = entt::null;
    entt::entity snow_emitter_ = entt::null;
    SDL_Event e_{};
    bool running_ = true;
};

#endif //PFX_EXAMPLE_APPLICATION_HPP
