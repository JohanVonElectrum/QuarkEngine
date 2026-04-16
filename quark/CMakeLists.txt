add_library(quark SHARED
    src/core/application.c
    src/core/window.c
    src/core/tracing.c
    src/core/engine.c
    src/core/event.c
    src/renderer/backend.c
    src/renderer/vulkan/backend.c
    src/renderer/vulkan/device.c
    src/renderer/vulkan/swapchain.c
)

target_include_directories(quark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(quark PRIVATE QUARK_ENGINE)

if (WIN32)
    target_compile_definitions(quark PRIVATE QUARK_PLATFORM_WINDOWS)
    target_sources(quark PRIVATE
        src/platform/clock_win32.c
        src/platform/memory_win32.c
        src/platform/thread_win32.c
    )
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(quark PRIVATE QUARK_DEBUG)
endif ()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE SHARED CACHE STRING "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(quark PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(quark PRIVATE Vulkan::Vulkan)

set(QUARK_GLSLC_EXECUTABLE "${Vulkan_GLSLC_EXECUTABLE}")
if (NOT QUARK_GLSLC_EXECUTABLE)
    find_program(QUARK_GLSLC_EXECUTABLE NAMES glslc)
endif ()

if (NOT QUARK_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Please install the Vulkan SDK and ensure glslc is available.")
endif ()

set(QUARK_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(QUARK_SHADER_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders")

set(QUARK_VERTEX_SHADER_SOURCE "${QUARK_SHADER_SOURCE_DIR}/triangle.vert")
set(QUARK_FRAGMENT_SHADER_SOURCE "${QUARK_SHADER_SOURCE_DIR}/triangle.frag")
set(QUARK_VERTEX_SHADER_SPV "${QUARK_SHADER_OUTPUT_DIR}/triangle.vert.spv")
set(QUARK_FRAGMENT_SHADER_SPV "${QUARK_SHADER_OUTPUT_DIR}/triangle.frag.spv")

add_custom_command(
    OUTPUT ${QUARK_VERTEX_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QUARK_SHADER_OUTPUT_DIR}"
    COMMAND ${QUARK_GLSLC_EXECUTABLE} "${QUARK_VERTEX_SHADER_SOURCE}" -o "${QUARK_VERTEX_SHADER_SPV}"
    DEPENDS ${QUARK_VERTEX_SHADER_SOURCE}
    COMMENT "Compiling Vulkan vertex shader: triangle.vert"
    VERBATIM
)

add_custom_command(
    OUTPUT ${QUARK_FRAGMENT_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QUARK_SHADER_OUTPUT_DIR}"
    COMMAND ${QUARK_GLSLC_EXECUTABLE} "${QUARK_FRAGMENT_SHADER_SOURCE}" -o "${QUARK_FRAGMENT_SHADER_SPV}"
    DEPENDS ${QUARK_FRAGMENT_SHADER_SOURCE}
    COMMENT "Compiling Vulkan fragment shader: triangle.frag"
    VERBATIM
)

add_custom_target(quark_vulkan_shaders
    DEPENDS
        ${QUARK_VERTEX_SHADER_SPV}
        ${QUARK_FRAGMENT_SHADER_SPV}
)

add_dependencies(quark quark_vulkan_shaders)
target_compile_definitions(quark PRIVATE QUARK_SHADER_DIR="${QUARK_SHADER_OUTPUT_DIR}")

