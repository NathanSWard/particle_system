cmake_minimum_required(VERSION 3.15)
project(pfx)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(DEPS_DIR ${PROJECT_SOURCE_DIR}/deps)
include_directories(${DEPS_DIR}/entt/single_include ${DEPS_DIR}/SDL2/include ${PROJECT_SOURCE_DIR}/pfx)

add_executable(particle_system src/example/main.cpp)

link_directories(${DEPS_DIR}/SDL2/lib)
set(SDL2_LIBS ${DEPS_DIR}/SDL2/lib)
target_link_libraries(particle_system ${SDL2_LIBS}/SDL2main.lib ${SDL2_LIBS}/SDL2.lib ${SDL2_LIBS}/SDL2_image.lib ${SDL2_LIBS}/SDL2_ttf.lib)

file(GLOB SDL2_DLLS ${DEPS_DIR}/SDL2/dll/*)
file(COPY ${SDL2_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})