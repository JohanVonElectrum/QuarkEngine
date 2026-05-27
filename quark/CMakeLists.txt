add_library(quark SHARED
    src/core/application.c
    src/core/camera.c
    src/core/window.c
    src/core/tracing.c
    src/core/engine.c
    src/core/event.c
    src/renderer/backend.c
    src/renderer/device.c
    src/renderer/swapchain.c
)

target_include_directories(quark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(quark PRIVATE QUARK_ENGINE)

if (WIN32)
    target_compile_definitions(quark PRIVATE QUARK_PLATFORM_WINDOWS)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(quark PRIVATE QUARK_DEBUG)
endif ()

include(FetchContent)

set(CSTDLIB_LOCAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cstdlib" CACHE PATH "Path to local cstdlib for development")
if(EXISTS "${CSTDLIB_LOCAL_PATH}/CMakeLists.txt")
    message(STATUS "Using LOCAL cstdlib from ${CSTDLIB_LOCAL_PATH} (development mode)")
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    add_subdirectory(${CSTDLIB_LOCAL_PATH} cstdlib)
else()
    FetchContent_Declare(
        cstdlib
        GIT_REPOSITORY https://github.com/JohanVonElectrum/cstdlib.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(cstdlib)
endif()
target_link_libraries(quark PRIVATE cstdlib::shared)

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

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cglm)
target_include_directories(quark PUBLIC ${cglm_SOURCE_DIR}/include)
target_link_libraries(quark PRIVATE cglm)

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

set(QUARK_VERTEX_SHADER_SOURCE "${QUARK_SHADER_SOURCE_DIR}/obb.vert")
set(QUARK_FRAGMENT_SHADER_SOURCE "${QUARK_SHADER_SOURCE_DIR}/obb.frag")
set(QUARK_VERTEX_SHADER_SPV "${QUARK_SHADER_OUTPUT_DIR}/obb.vert.spv")
set(QUARK_FRAGMENT_SHADER_SPV "${QUARK_SHADER_OUTPUT_DIR}/obb.frag.spv")

add_custom_command(
    OUTPUT ${QUARK_VERTEX_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QUARK_SHADER_OUTPUT_DIR}"
    COMMAND ${QUARK_GLSLC_EXECUTABLE} "${QUARK_VERTEX_SHADER_SOURCE}" -o "${QUARK_VERTEX_SHADER_SPV}"
    DEPENDS ${QUARK_VERTEX_SHADER_SOURCE}
    COMMENT "Compiling Vulkan vertex shader: obb.vert"
    VERBATIM
)

add_custom_command(
    OUTPUT ${QUARK_FRAGMENT_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QUARK_SHADER_OUTPUT_DIR}"
    COMMAND ${QUARK_GLSLC_EXECUTABLE} "${QUARK_FRAGMENT_SHADER_SOURCE}" -o "${QUARK_FRAGMENT_SHADER_SPV}"
    DEPENDS ${QUARK_FRAGMENT_SHADER_SOURCE}
    COMMENT "Compiling Vulkan fragment shader: obb.frag"
    VERBATIM
)

add_custom_target(quark_vulkan_shaders
    DEPENDS
        ${QUARK_VERTEX_SHADER_SPV}
        ${QUARK_FRAGMENT_SHADER_SPV}
)

add_dependencies(quark quark_vulkan_shaders)
target_compile_definitions(quark PRIVATE QUARK_SHADER_DIR="${QUARK_SHADER_OUTPUT_DIR}")

