## NOTE FOR CSCI3010 HOMEWORK PROGRESS
* Due to a messy merge conflict chain, I had to delete the old repository and create a new one, to prevent breaking the project's build structure.

## Work in progress generic progress particle system
* Currently only usable on windows.
  * set 'CMAKE_C_COMPILER' and 'CMAKE_CXX_COMPILER' environment variable to the location of your compiler.
  * run 'build_script' to create the executable 'bin/particle_system'
    * NOTE: if this fails, execute the following commands:
      * echo Creating build directory
      * mkdir build
      * cd build
      * cmake ../
      * cmake --build .
  * Dependencies:
    * All dependencies are includes in the deps/ directory

* pfx/
  * the generic particle system library
* src/example
  * an example implementation using SDL2

```
namespace pfx {

template<class... Components>
particle_components; // defines the components that make up a particle

template<class... Components, class... ParticleSystemPolicies>
basic_particle_system_manager<particle_components<Components...>, ParticleSystemPolicies...>;

using particle_tag = /**/;
using inactive_tag = /**/;

template<class T>
struct arg; // helper tag type used in creating particles.

}
```

## Particle System Policy Requirements

* _Note_: Make static methods constexpr when possible. Various optimization are taken when the methods can be evaluated at compile time.

`inline static constexpr std::size_t max_particles;`
  * The maximum number of particles

`inline static constexpr std::chrono::duration duration;`
  * The duration of the emitter

`inline static constexpr int emission_rate;`
  * The number of particle emitted per second.
  * If not provided, you will need to provide one at runtime via `create_emitter()`

#### The following two methods `initial_lifetime` and `initial` have two possible overloads.
  * The first should be used when a particle's initial components are _independent_ of the `entt::registry` environment.
  * The second should be used when the inital componets are _dependent_ upon the `entt::registry` environment.
    * The two additional arguments passed in are the:
       * `entt::entity` id of the associated particle emitter.
       * `entt::registry` environment where all particle information is held.
    * Usage example: A particle's initial position depends upon its emitter's position.

`inline static constexpr auto initial_lifetime() -> std::chrono::duration;`

`inline static constexpr auto initial_lifetime(entt::entity, entt::registry&) -> std::chrono::duration;`
  * Specifies the lifetime of a particle.
  * If not provided, emitter's particles will have an infinite lifetime.

`inline static constexpr auto initial(arg<T>) -> T;`

`inline static constexpr auto initial(arg<T>, entt::entity, entt::registry&) -> T;`

`inline static constexpr void over_lifetime(T&, float);`

`inline static constexpr void over_time(T&, time_type);`

`inline static constexpr void stop_action();`
