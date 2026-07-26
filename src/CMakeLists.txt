add_executable(mlc
    main.cpp
    benchmark.cpp
    cycle_counter.cpp
)

target_link_libraries(
    mlc
    argparse
)

