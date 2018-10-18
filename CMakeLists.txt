project(meshoptimizer)
cmake_minimum_required(VERSION 3.0)

option(BUILD_DEMO "Build demo" OFF)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif(MSVC)

set(SOURCES
    src/meshoptimizer.h
    src/indexcodec.cpp
    src/indexgenerator.cpp
    src/overdrawanalyzer.cpp
    src/overdrawoptimizer.cpp
    src/simplifier.cpp
    src/stripifier.cpp
    src/vcacheanalyzer.cpp
    src/vcacheoptimizer.cpp
    src/vertexcodec.cpp
    src/vfetchanalyzer.cpp
    src/vfetchoptimizer.cpp
)

add_library(meshoptimizer STATIC ${SOURCES})
target_include_directories(meshoptimizer INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(BUILD_DEMO)
    add_executable(demo demo/main.cpp demo/miniz.cpp demo/objparser.cpp)
    target_link_libraries(demo meshoptimizer)
endif()
