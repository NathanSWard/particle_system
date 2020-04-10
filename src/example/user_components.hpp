#ifndef PFX_USER_COMPONENTS_HPP
#define PFX_USER_COMPONENTS_HPP

#include <entt/entt.hpp>
#include "particle_components.hpp"

struct cursor_tag{};

entt::entity initialize_cursor(entt::registry& r) {
    auto const e = r.create();
    r.assign<cursor_tag>(e);
    r.assign<position>(e, 0.f, 0.f);
    return e;
}

#endif //PFX_USER_COMPONENTS_HPP
