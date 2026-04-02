add_executable(testbed
    src/main.c
)

target_include_directories(testbed PRIVATE
    ${CMAKE_SOURCE_DIR}/quark/include
)

target_link_libraries(testbed PRIVATE quark)
