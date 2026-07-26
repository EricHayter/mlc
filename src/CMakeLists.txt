add_executable(mlc
    main.cpp
    benchmark.cpp
    mmap_array.cpp
)

target_link_libraries(
    mlc
    argparse
)

