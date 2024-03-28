cmake_minimum_required(VERSION 3.28)
project(metal_cpp_test)

set(CMAKE_CXX_STANDARD 20)

find_library(FOUNDATION_LIBRARY Foundation)
find_library(METALKIT_LIBRARY MetalKit)
find_library(METAL_LIBRARY Metal)

add_executable(
    add_arrays
    src/main.cpp
    src/metal_adder.cpp
)

target_include_directories(add_arrays PRIVATE deps)
target_link_libraries(add_arrays
    ${FOUNDATION_LIBRARY}
    ${METALKIT_LIBRARY}
    ${METAL_LIBRARY}
)

# Compile the Metal Shader/Kernel
add_custom_command(
    TARGET add_arrays POST_BUILD
    COMMAND xcrun -sdk macosx metal -c ${PROJECT_SOURCE_DIR}/src/add.metal -o MetalLibrary.air &&
            xcrun -sdk macosx metallib MetalLibrary.air -o default.metallib
)
