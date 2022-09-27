cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_FLAGS "-std=c++17")

project(cpu-raytracer)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Our own includes
include_directories("src/")

# Include glm for vector/matrix operations
include_directories("libs/glm-master/")

add_executable(cpu-raytracer
    src/main.cpp
    src/tonemapping.cpp
    src/light.cpp
    src/rays.cpp
    src/sphere.cpp
)
target_link_libraries(cpu-raytracer ${SDL2_LIBRARIES})
