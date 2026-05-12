#
# Copyright (c) 2025-present Henri Michelon
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
#
##### Compile Slang sources files into DXIL & SPIR-V
find_program(SLANGC_EXECUTABLE NAMES slangc)
if(NOT SLANGC_EXECUTABLE)
    find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "${Vulkan_INSTALL_DIR}/bin")
endif()
if(NOT SLANGC_EXECUTABLE)
    find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "$ENV{VULKAN_SDK}/bin")
endif()
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc executable not found.")
endif()
message(NOTICE "slangc found in ${SLANGC_EXECUTABLE}")


function(add_shader EXTENSION PROFILE CAPABILITY ENTRY_POINT SHADER_SOURCE SHADER_BINARIES SHADER_INCLUDE_DIR SHADER_DEPS)
    set(LOCAL_PRODUCTS)
    if (DIRECTX_BACKEND)
        set(OUTPUT_DXIL "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.dxil")
        add_custom_command(
            OUTPUT  "${OUTPUT_DXIL}"
            COMMAND "${SLANGC_EXECUTABLE}"
                    -profile  "${PROFILE}"
                    -entry    "${ENTRY_POINT}"
                    -I        "${SHADER_INCLUDE_DIR}"
                    -o        "${OUTPUT_DXIL}"
                    "${SHADER_SOURCE}"
            DEPENDS "${SHADER_SOURCE}" ${SHADER_DEPS}
        )
        list(APPEND LOCAL_PRODUCTS "${OUTPUT_DXIL}")
    endif ()
    set(OUTPUT_SPV "${SHADER_BINARIES}/${SHADER_NAME}.${EXTENSION}.spv")
    add_custom_command(
        OUTPUT  "${OUTPUT_SPV}"
        COMMAND "${SLANGC_EXECUTABLE}"
                -capability     "${CAPABILITY}"
                -profile        "${PROFILE}"
                -entry          "${ENTRY_POINT}"
                -D              __SPIRV__
                -I              "${SHADER_INCLUDE_DIR}"
                -fvk-use-dx-layout
                -o              "${OUTPUT_SPV}"
                "${SHADER_SOURCE}"
        DEPENDS "${SHADER_SOURCE}" ${SHADER_DEPS}
    )
    list(APPEND LOCAL_PRODUCTS "${OUTPUT_SPV}")

    set(GLOBAL_SHADER_PRODUCTS "${SHADER_PRODUCTS}")
    set(SHADER_PRODUCTS "${GLOBAL_SHADER_PRODUCTS};${LOCAL_PRODUCTS}" PARENT_SCOPE)
endfunction()

function(add_shaders TARGET_NAME BUILD_DIR SHADER_INCLUDE_DIR)
    set(SHADER_SOURCE_FILES ${ARGN})
    set(SHADER_BINARIES ${BUILD_DIR})
    set(SHADER_PRODUCTS)

    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        return()
    endif()

    file(MAKE_DIRECTORY ${SHADER_BINARIES})

    file(GLOB_RECURSE SHADER_DEPS "${SHADER_INCLUDE_DIR}/*.inc.slang")

    set(SPIRV_CAPABILITY "SPIRV_1_5")
    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE STEM SHADER_NAME)
        if(SHADER_SOURCE MATCHES ".comp.slang$")
            add_shader("comp" "cs_6_6" "${SPIRV_CAPABILITY}" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (SHADER_SOURCE MATCHES ".hull.slang$")
            add_shader("hull" "hs_6_6" "${SPIRV_CAPABILITY}" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (SHADER_SOURCE MATCHES ".domain.slang$")
            add_shader("domain" "ds_6_6" "${SPIRV_CAPABILITY}" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (SHADER_SOURCE MATCHES ".geom.slang$")
            add_shader("geom" "gs_6_6" "${SPIRV_CAPABILITY}" "main" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (SHADER_SOURCE MATCHES ".vert.slang$")
            add_shader("vert" "vs_6_6" "${SPIRV_CAPABILITY}" "vertexMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (SHADER_SOURCE MATCHES ".frag.slang$")
            add_shader("frag" "ps_6_6" "${SPIRV_CAPABILITY}" "fragmentMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        elseif (NOT SHADER_SOURCE MATCHES ".inc.slang$")
            add_shader("vert" "vs_6_6" "${SPIRV_CAPABILITY}" "vertexMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
            add_shader("frag" "ps_6_6" "${SPIRV_CAPABILITY}" "fragmentMain" ${SHADER_SOURCE} ${SHADER_BINARIES} ${SHADER_INCLUDE_DIR} "${SHADER_DEPS}")
        endif ()
    endforeach()

    add_custom_target(${TARGET_NAME} ALL
            DEPENDS ${SHADER_PRODUCTS}
            COMMENT "Compiling Shaders [${TARGET_NAME}]"
            SOURCES ${SHADER_SOURCE_FILES}
    )
endfunction(add_shaders)