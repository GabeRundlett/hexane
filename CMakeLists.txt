cmake_minimum_required(VERSION 3.16)
project(hexane LANGUAGES CXX)

find_package(daxa REQUIRED)
find_package(glfw3 REQUIRED)
add_executable(hexane src/main.cpp)

target_link_libraries(hexane PRIVATE daxa::daxa)
target_link_libraries(hexane PRIVATE glfw)

set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:hexane>/../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(hexane PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")
