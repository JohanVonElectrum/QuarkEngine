add_library(quark SHARED
    src/core/application.c
)

target_include_directories(quark PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(quark PRIVATE QUARK_ENGINE)
