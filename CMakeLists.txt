cmake_minimum_required(VERSION 3.16)

project(neon_city_starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm CONFIG QUIET)

add_executable(neon_city
    src/main.cpp
    src/shader.cpp
)

target_include_directories(neon_city PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(neon_city PRIVATE APP_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

target_link_libraries(neon_city PRIVATE OpenGL::GL GLEW::GLEW glfw)

if(TARGET glm::glm)
    target_link_libraries(neon_city PRIVATE glm::glm)
endif()

if(MSVC)
    target_compile_options(neon_city PRIVATE /W4)
else()
    target_compile_options(neon_city PRIVATE -Wall -Wextra -Wpedantic)
endif()
