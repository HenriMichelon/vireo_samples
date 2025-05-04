#
# Copyright (c) 2025-present Henri Michelon
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
#
##### Compile Slang sources files into DXIL & SPIR-V

# Use the Vulkan SDK version of the slangc executable
find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "${Vulkan_INSTALL_DIR}/bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc not found. Please ensure the Vulkan SDK is installed and slangc is available.")
endif()

set(SHADER_COMMANDS)
set(SHADER_PRODUCTS)

function(add_shader EXTENSION PROFILE ENTRY_POINT SHADER_SOURCE SHADER_BINARIES SHADER_INCLUDE_DIR)
    set(LOCAL_COMMANDS)
    set(LOCAL_PRODUCTS)
    if (DIRECTX_BACKEND)
        # Compute shader to DXIL
        list(APPEND LOCAL_COMMANDS COMMAND)
        list(APPEND LOCAL_COMMANDS "${SLANGC_EXECUTABLE}")
        list(APPEND LOCAL_COMMANDS "-profile")
        list(APPEND LOCAL_COMMANDS "${PROFILE}")
        list(APPEND LOCAL_COMMANDS "-entry")
        list(APPEND LOCAL_COMMANDS "${ENTRY_POINT}")
        list(APPEND LOCAL_COMMANDS "-I")
        list(APPEND LOCAL_COMMANDS "${SHADER_INCLUDE_DIR}")
        list(APPEND LOCAL_COMMANDS "-o")
        list(APPEND LOCAL_COMMANDS "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.dxil")
        list(APPEND LOCAL_COMMANDS "${SHADER_SOURCE}")
        list(APPEND LOCAL_PRODUCTS "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.dxil")
    endif ()
    # Compute shader to SPIR-V
    list(APPEND LOCAL_COMMANDS COMMAND)
    list(APPEND LOCAL_COMMANDS "${SLANGC_EXECUTABLE}")
    list(APPEND LOCAL_COMMANDS "-profile")
    list(APPEND LOCAL_COMMANDS "${PROFILE}")
    list(APPEND LOCAL_COMMANDS "-entry")
    list(APPEND LOCAL_COMMANDS "${ENTRY_POINT}")
    list(APPEND LOCAL_COMMANDS "-I")
    list(APPEND LOCAL_COMMANDS "${SHADER_INCLUDE_DIR}")
    list(APPEND LOCAL_COMMANDS "-o")
    list(APPEND LOCAL_COMMANDS "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.spv")
    list(APPEND LOCAL_COMMANDS "${SHADER_SOURCE}")
    list(APPEND LOCAL_PRODUCTS "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.spv")

    set(GLOBAL_SHADER_COMMANDS "${SHADER_COMMANDS}")
    set(SHADER_COMMANDS "${GLOBAL_SHADER_COMMANDS};${LOCAL_COMMANDS}" PARENT_SCOPE)
    set(GLOBAL_SHADER_PRODUCTS "${SHADER_PRODUCTS}")
    set(SHADER_PRODUCTS "${GLOBAL_SHADER_PRODUCTS};${LOCAL_PRODUCTS}" PARENT_SCOPE)

    message("${LOCAL_PRODUCTS}")
endfunction()

function(add_shaders TARGET_NAME BUILD_DIR SHADER_INCLUDE_DIR)
    set(SHADER_SOURCE_FILES ${ARGN}) # the rest of arguments to this function will be assigned as shader source files
    set(SHADER_BINARIES ${BUILD_DIR})

    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot create a shaders target without any source files")
    endif()

    file(MAKE_DIRECTORY ${SHADER_BINARIES})

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE STEM SHADER_NAME)
        if(SHADER_SOURCE MATCHES ".comp.slang$")
            add_shader("comp" "cs_6_6" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (SHADER_SOURCE MATCHES ".hull.slang$")
            add_shader("hull" "hs_6_0" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (SHADER_SOURCE MATCHES ".domain.slang$")
            add_shader("domain" "ds_6_0" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (SHADER_SOURCE MATCHES ".geom.slang$")
            add_shader("domain" "gs_6_0" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (SHADER_SOURCE MATCHES ".vert.slang$")
            add_shader("vert" "vs_6_0" "vertexMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (SHADER_SOURCE MATCHES ".frag.slang$")
            add_shader("frag" "ps_6_0" "fragmentMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        elseif (NOT SHADER_SOURCE MATCHES ".inc.slang$")
            add_shader("vert" "vs_6_0" "vertexMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
            add_shader("frag" "ps_6_0" "fragmentMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR})
        endif ()
    endforeach()

    add_custom_target(${TARGET_NAME} ALL
            ${SHADER_COMMANDS}
            COMMENT "Compiling Shaders [${TARGET_NAME}]"
            SOURCES ${SHADER_SOURCE_FILES}
            BYPRODUCTS ${SHADER_PRODUCTS}
    )
endfunction()
