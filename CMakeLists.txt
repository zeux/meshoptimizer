cmake_minimum_required(VERSION 3.5...3.30)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # Enables override of options from parent CMakeLists.txt
endif()

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW) # Enables use of MSVC_RUNTIME_LIBRARY
endif()

if(POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW) # Enables clean /W4 override for MSVC
endif()

project(meshoptimizer VERSION 1.0 LANGUAGES CXX)

option(MESHOPT_BUILD_DEMO "Build demo" OFF)
option(MESHOPT_BUILD_GLTFPACK "Build gltfpack" OFF)
option(MESHOPT_BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(MESHOPT_STABLE_EXPORTS "Only export stable APIs from shared library" OFF)
option(MESHOPT_WERROR "Treat warnings as errors" OFF)
option(MESHOPT_INSTALL "Install library" ON)

# Optional gltfpack components for texture compression support
set(MESHOPT_GLTFPACK_BASISU_PATH "" CACHE STRING "") # Basis Universal, https://github.com/BinomialLLC/basis_universal
set(MESHOPT_GLTFPACK_LIBWEBP_PATH "" CACHE STRING "") # libwebp, https://github.com/webmproject/libwebp

set(SOURCES
    src/meshoptimizer.h
    src/allocator.cpp
    src/clusterizer.cpp
    src/indexanalyzer.cpp
    src/indexcodec.cpp
    src/indexgenerator.cpp
    src/overdrawoptimizer.cpp
    src/partition.cpp
    src/quantization.cpp
    src/rasterizer.cpp
    src/simplifier.cpp
    src/spatialorder.cpp
    src/stripifier.cpp
    src/vcacheoptimizer.cpp
    src/vertexcodec.cpp
    src/vertexfilter.cpp
    src/vfetchoptimizer.cpp
)

set(GLTF_SOURCES
    gltf/animation.cpp
    gltf/encodebasis.cpp
    gltf/encodewebp.cpp
    gltf/fileio.cpp
    gltf/gltfpack.cpp
    gltf/image.cpp
    gltf/json.cpp
    gltf/material.cpp
    gltf/mesh.cpp
    gltf/node.cpp
    gltf/parseobj.cpp
    gltf/parselib.cpp
    gltf/parsegltf.cpp
    gltf/stream.cpp
    gltf/write.cpp
)

if(WIN32)
    list(APPEND GLTF_SOURCES gltf/gltfpack.manifest)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wno-missing-field-initializers)
endif()

if(MESHOPT_WERROR)
    if(MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

if(MESHOPT_BUILD_SHARED_LIBS)
    add_library(meshoptimizer SHARED ${SOURCES})
else()
    add_library(meshoptimizer STATIC ${SOURCES})
endif()

target_include_directories(meshoptimizer INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")

if(MESHOPT_BUILD_SHARED_LIBS)
    set_target_properties(meshoptimizer PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(meshoptimizer PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

    # soversion may be requested via -DMESHOPT_SOVERSION=n; note that experimental APIs (marked with MESHOPTIMIZER_EXPERIMENTAL) are not ABI-stable
    if(MESHOPT_SOVERSION)
        set_target_properties(meshoptimizer PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${MESHOPT_SOVERSION})
    endif()

    if(WIN32)
        target_compile_definitions(meshoptimizer INTERFACE "MESHOPTIMIZER_API=__declspec(dllimport)")
        target_compile_definitions(meshoptimizer PRIVATE "MESHOPTIMIZER_API=__declspec(dllexport)")
    else()
        target_compile_definitions(meshoptimizer PUBLIC "MESHOPTIMIZER_API=__attribute__((visibility(\"default\")))")
    endif()

    target_compile_definitions(meshoptimizer PUBLIC MESHOPTIMIZER_ALLOC_EXPORT)

    if(MESHOPT_STABLE_EXPORTS)
		target_compile_definitions(meshoptimizer PUBLIC "MESHOPTIMIZER_EXPERIMENTAL=")
    endif()
endif()

if(MESHOPT_BUILD_DEMO)
    add_executable(demo demo/main.cpp demo/nanite.cpp demo/tests.cpp tools/objloader.cpp)
    set_target_properties(demo PROPERTIES CXX_STANDARD 11)
    set_target_properties(demo PROPERTIES OUTPUT_NAME meshoptdemo)
    target_link_libraries(demo meshoptimizer)
endif()

if(MESHOPT_BUILD_GLTFPACK)
    add_executable(gltfpack ${GLTF_SOURCES})
    set_target_properties(gltfpack PROPERTIES CXX_STANDARD 11)
    target_link_libraries(gltfpack meshoptimizer)

    if(MESHOPT_BUILD_SHARED_LIBS)
        string(CONCAT RPATH "$ORIGIN/../" ${CMAKE_INSTALL_LIBDIR})
        set_target_properties(gltfpack PROPERTIES INSTALL_RPATH ${RPATH})
    endif()

    if(NOT MESHOPT_GLTFPACK_BASISU_PATH STREQUAL "")
        get_filename_component(BASISU_PATH ${MESHOPT_GLTFPACK_BASISU_PATH} ABSOLUTE)
        if (NOT EXISTS ${BASISU_PATH})
            message(FATAL_ERROR "Basis Universal path ${BASISU_PATH} not found")
        endif()

        if (NOT SSE AND NOT MSVC AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            message(WARNING "Building Basis Universal without SSE4.1 support; performance may be suboptimal")
        endif()

        add_subdirectory(${BASISU_PATH} ${CMAKE_CURRENT_BINARY_DIR}/basisu EXCLUDE_FROM_ALL)

        target_compile_definitions(gltfpack PRIVATE WITH_BASISU)
        target_link_libraries(gltfpack basisu_encoder)
        set_source_files_properties(gltf/encodebasis.cpp PROPERTIES INCLUDE_DIRECTORIES ${BASISU_PATH}) # necessary because basisu_encoder doesn't export include directories
    endif()

    if(NOT MESHOPT_GLTFPACK_LIBWEBP_PATH STREQUAL "")
        get_filename_component(LIBWEBP_PATH ${MESHOPT_GLTFPACK_LIBWEBP_PATH} ABSOLUTE)
        if (NOT EXISTS ${LIBWEBP_PATH})
            message(FATAL_ERROR "libwebp path ${LIBWEBP_PATH} not found")
        endif()

        add_subdirectory(${LIBWEBP_PATH} ${CMAKE_CURRENT_BINARY_DIR}/libwebp EXCLUDE_FROM_ALL)

        target_compile_definitions(gltfpack PRIVATE WITH_LIBWEBP)
        target_link_libraries(gltfpack webp)

        # when gltfpack is built with Basis Universal, we use Basis image decoders for PNG/JPEG support for ease of distribution
        if(NOT MESHOPT_GLTFPACK_BASISU_PATH STREQUAL "")
            target_compile_definitions(gltfpack PRIVATE WITH_LIBWEBP_BASIS)
        else()
            target_link_libraries(gltfpack imagedec)
        endif()
    endif()

    if(NOT MESHOPT_GLTFPACK_BASISU_PATH STREQUAL "" OR NOT MESHOPT_GLTFPACK_LIBWEBP_PATH STREQUAL "")
        find_package(Threads REQUIRED)
        target_link_libraries(gltfpack Threads::Threads)
    endif()
endif()

if(MESHOPT_INSTALL)
	include(GNUInstallDirs)

	install(TARGETS meshoptimizer EXPORT meshoptimizerTargets
	    COMPONENT meshoptimizer
	    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

	if(MESHOPT_BUILD_GLTFPACK)
	    install(TARGETS gltfpack RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	endif()

	install(FILES src/meshoptimizer.h COMPONENT meshoptimizer DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
	install(EXPORT meshoptimizerTargets COMPONENT meshoptimizer DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/meshoptimizer NAMESPACE meshoptimizer::)

	if(MSVC)
		set(PDB_TARGETS meshoptimizer)
		if(MESHOPT_BUILD_GLTFPACK)
		    list(APPEND PDB_TARGETS gltfpack)
		endif()

	    foreach(TARGET ${PDB_TARGETS})
	        get_target_property(TARGET_TYPE ${TARGET} TYPE)
	        if(NOT ${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
	            install(FILES $<TARGET_PDB_FILE:${TARGET}> COMPONENT meshoptimizer DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
	        endif()
	    endforeach(TARGET)
	endif()

	include(CMakePackageConfigHelpers)

	# CMake 3.18+ supports file(CONFIGURE OUTPUT file CONTENT content @ONLY) but we only require 3.5+
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/meshoptimizerConfig.cmake
		"include(\"\${CMAKE_CURRENT_LIST_DIR}/meshoptimizerTargets.cmake\")\n")

	write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/meshoptimizerConfigVersion.cmake COMPATIBILITY ExactVersion)

	install(FILES
	    ${CMAKE_CURRENT_BINARY_DIR}/meshoptimizerConfig.cmake
	    ${CMAKE_CURRENT_BINARY_DIR}/meshoptimizerConfigVersion.cmake
	    COMPONENT meshoptimizer
	    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/meshoptimizer)
endif()
