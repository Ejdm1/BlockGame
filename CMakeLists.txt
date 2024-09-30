cmake_minimum_required(VERSION 3.5.0)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(BlockGame ./definition.cpp)

target_include_directories(BlockGame PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp" "${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp-extensions")

target_link_libraries(BlockGame "-framework Metal" "-framework MetalKit" "-framework AppKit" "-framework Foundation" "-framework QuartzCore")

