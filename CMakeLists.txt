cmake_minimum_required(VERSION 3.28.0)
project(test VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

FetchContent_Declare(FastNoise2 GIT_REPOSITORY https://github.com/Auburn/FastNoise2.git)
FetchContent_MakeAvailable(FastNoise2)

set(CMAKE_CXX_STANDARD 20)

add_executable(test src/backend/glfw_adapter.mm
                    src/camera.cpp src/context.cpp
                    src/metal_stuff.cpp src/renderer.cpp
                    src/window.cpp 
                    src/main.cpp
                    src/chunk.cpp
                    dependencies/imgui/imgui_draw.cpp
                    dependencies/imgui/imgui_internal.h
                    dependencies/imgui/imgui_tables.cpp
                    dependencies/imgui/imgui_widgets.cpp
                    dependencies/imgui/imgui.cpp
                    dependencies/imgui/imgui.h
                    dependencies/imgui/imstb_rectpack.h
                    dependencies/imgui/imstb_textedit.h
                    dependencies/imgui/imstb_truetype.h
                    dependencies/imgui/backends/imgui_impl_metal.h
                    dependencies/imgui/backends/imgui_impl_metal.mm
                    dependencies/imgui/backends/imgui_impl_glfw.h
                    dependencies/imgui/backends/imgui_impl_glfw.cpp
                    )

target_include_directories(test PRIVATE dependencies)
target_include_directories(test PRIVATE src)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/glm)

target_link_libraries(test PRIVATE "-framework Metal"
                                   "-framework MetalKit" 
                                   "-framework AppKit" 
                                   "-framework Foundation" 
                                   "-framework QuartzCore"
                                   "-framework GameController")
target_link_libraries(test PRIVATE glfw glm::glm)
target_link_libraries(test PRIVATE FastNoise2)
target_compile_features(test PRIVATE cxx_std_20)
