project(meshoptimizer)
cmake_minimum_required(VERSION 3.0)

option(BUILD_DEMO "Build demo" OFF)
option(BUILD_TOOLS "Build tools" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(SOURCES
    src/meshoptimizer.h
    src/allocator.cpp
    src/clusterizer.cpp
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

add_library(meshoptimizer ${SOURCES})
target_include_directories(meshoptimizer INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
	target_compile_options(meshoptimizer PRIVATE /W4 /WX)
else()
	target_compile_options(meshoptimizer PRIVATE -Wall -Wextra -Werror)
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(
        meshoptimizer
        PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    if(WIN32)
        target_compile_definitions(
            meshoptimizer
            INTERFACE
            "MESHOPTIMIZER_API=__declspec(dllimport)"
            PRIVATE
            "MESHOPTIMIZER_API=__declspec(dllexport)"
        )
    else()
        target_compile_definitions(
            meshoptimizer
            PUBLIC
            "MESHOPTIMIZER_API=__attribute__((visibility(\"default\")))"
        )
    endif()
endif()

if(BUILD_DEMO)
    add_executable(demo demo/main.cpp demo/miniz.cpp demo/tests.cpp tools/meshloader.cpp)
    target_link_libraries(demo meshoptimizer)
endif()

if(BUILD_TOOLS)
    add_executable(gltfpack tools/gltfpack.cpp tools/meshloader.cpp)
    target_link_libraries(gltfpack meshoptimizer)
endif()
