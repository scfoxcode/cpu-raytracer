cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_FLAGS "-std=c++17")

project(cpu-raytracer)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Our own includes
include_directories("src/")

add_executable(cpu-raytracer src/main.cpp)
target_link_libraries(cpu-raytracer ${SDL2_LIBRARIES})
