add_library(quark SHARED
    src/core/application.c
    src/core/window.c
    src/core/logger.c
)

target_include_directories(quark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(quark PRIVATE QUARK_ENGINE)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE   SHARED CACHE STRING "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(quark PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(quark PRIVATE Vulkan::Vulkan)
