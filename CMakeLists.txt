cmake_minimum_required(VERSION 3.28.0)
project(test VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

add_executable(test src/backend/glfw_adapter.mm src/camera.cpp src/context.cpp src/metal_bullshit.cpp src/renderer.cpp src/window.cpp src/main.cpp )


target_include_directories(test PRIVATE dependencies)
add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/glm)

target_link_libraries(test PRIVATE "-framework Metal" "-framework MetalKit" "-framework AppKit" "-framework Foundation" "-framework QuartzCore")

target_link_libraries(test PRIVATE glfw glm::glm)