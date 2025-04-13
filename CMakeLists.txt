#
# Copyright (c) 2025-present Henri Michelon
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
#
cmake_minimum_required(VERSION 3.29)

#######################################################
project(vireo_samples)
include(.env.cmake)

#######################################################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#######################################################
include(FetchContent)
include(cmake/shaders.cmake)
include(cmake/libraries.cmake)

#######################################################
add_subdirectory(${VIREO_PROJECT_DIR} external_lib_build)
set(VIREO_INCLUDE_DIR ${VIREO_PROJECT_DIR}/include)
set(VIREO_SHADERS_DIR ${VIREO_PROJECT_DIR}/src/shaders)
set(VIREO_SHADERS_BUILD_DIR ${VIREO_PROJECT_DIR}/shaders)
set(VIREO_TARGET "vireo_ral")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SHADERS_SRC_DIR ${SRC_DIR}/shaders)
set(SHADERS_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

#######################################################
# Slang shaders
file(MAKE_DIRECTORY ${SHADERS_BUILD_DIR})
file(GLOB_RECURSE SHADERS_SOURCE_FILES
        "${SHADERS_SRC_DIR}/*.slang"
)
add_custom_target(post_build_shaders ALL
        COMMENT "Copying Shaders [${VIREO_TARGET}]"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${VIREO_SHADERS_BUILD_DIR} ${SHADERS_BUILD_DIR}
        BYPRODUCTS ${SHADERS_BUILD_DIR}/dummy_vert.spv
)

#######################################################
# Common files
set(COMMON_SOURCES
        ${SRC_DIR}/os/windows/Win32Application.cpp
)
set(COMMON_MODULES
        ${SRC_DIR}/samples/Application.ixx
        ${SRC_DIR}/os/windows/Win32Application.ixx
)

#######################################################
set(MY_TARGET ${PROJECT_NAME})
add_executable(${MY_TARGET}
        ${COMMON_SOURCES}
        ${SRC_DIR}/samples/triangle/TriangleApp.cpp
)

target_sources(${MY_TARGET}
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
        ${COMMON_MODULES}
        ${SRC_DIR}/samples/triangle/TriangleApp.ixx
)

compile_options(${MY_TARGET})
target_include_directories(${MY_TARGET} PUBLIC ${INCLUDE_DIR} ${VIREO_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${VIREO_TARGET} glm::glm glm-modules std-cxx-modules)
if(WIN32)
target_link_libraries(${MY_TARGET} Xinput dinput8 dxguid dxgi d3d12 d3dcompiler)
if(MINGW)
    target_link_options(${MY_TARGET} PRIVATE "-mwindows")
endif()
set_target_properties(${MY_TARGET} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_shaders(${MY_TARGET}_shaders ${SHADERS_BUILD_DIR} ${VIREO_SHADERS_DIR} ${SHADERS_SOURCE_FILES})
add_dependencies(post_build_shaders ${MY_TARGET}_shaders)
add_dependencies(${MY_TARGET}_shaders ${VIREO_TARGET})
add_dependencies(${MY_TARGET} post_build_shaders)
