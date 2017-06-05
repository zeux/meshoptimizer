project(meshoptimizer)
cmake_minimum_required(VERSION 3.4)

option(BUILD_DEMO "Build demo" OFF)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif(MSVC)

set(SOURCES
    src/meshoptimizer.hpp
    src/indexgenerator.cpp
    src/overdrawanalyzer.cpp
    src/overdrawoptimizer.cpp
    src/posttransformanalyzer.cpp
    src/posttransformoptimizer.cpp
    src/pretransformanalyzer.cpp
    src/pretransformoptimizer.cpp
)

add_library(meshoptimizer STATIC ${SOURCES})

if(BUILD_DEMO)
	add_executable(demo demo/main.cpp demo/objparser.cpp)
	target_link_libraries(demo meshoptimizer)
endif()
