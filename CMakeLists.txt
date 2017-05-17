project(meshoptimizer)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    src/indexgenerator.cpp
    src/overdrawanalyzer.cpp
    src/overdrawoptimizer.cpp
    src/posttransformoptimizer.cpp
    src/pretransformoptimizer.cpp
    src/vcacheanalyzer.cpp
    src/meshoptimizer.hpp
)

add_library(meshoptimizer STATIC ${SOURCES})

add_executable(demo demo/main.cpp demo/tiny_obj_loader.cpp)
target_link_libraries(demo meshoptimizer)
