#
# Copyright (c) 2024-2025 Henri Michelon
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
#
###### Using GLM for maths
message(NOTICE "Fetching GLM from https://github.com/g-truc/glm ...")
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_compile_definitions(GLM_GTC_constants)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
FetchContent_Declare(
    fetch_glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(fetch_glm)
set(GLM_DIR ${CMAKE_BINARY_DIR}/_deps/fetch_glm-src/glm)
# compile GLM as a module
message(NOTICE "Building glm C++ module...")
add_library(glm-modules STATIC)
target_sources(glm-modules
  PUBLIC
    FILE_SET moduleStd
    TYPE CXX_MODULES
    BASE_DIRS ${GLM_DIR}
    FILES
      ${GLM_DIR}/glm.cppm)
target_link_libraries(glm-modules glm::glm)
