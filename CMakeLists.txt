cmake_minimum_required(VERSION 3.16)

include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
project(hexane LANGUAGES CXX)

find_package(daxa CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
add_executable(hexane src/main.cpp)

set_property(TARGET hexane PROPERTY CXX_STANDARD 20)

target_link_libraries(hexane PRIVATE daxa::daxa)
target_link_libraries(hexane PRIVATE glfw)

target_compile_definitions(hexane PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(hexane PRIVATE include)