add_executable(mlc
    main.cpp
    benchmark.cpp
    cycle_counter.cpp
)

# Warnings are scoped to our target so third-party code stays quiet.
target_compile_options(mlc PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
)

target_link_libraries(
    mlc
    argparse
)

