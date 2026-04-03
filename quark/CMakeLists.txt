add_library(quark SHARED
    src/core/application.c
    src/core/window.c
    src/core/logger.c
    src/core/engine.c
)

target_include_directories(quark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(quark PRIVATE QUARK_ENGINE)

if (WIN32)
    target_compile_definitions(quark PRIVATE QUARK_PLATFORM_WINDOWS)
    target_sources(quark PRIVATE
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
