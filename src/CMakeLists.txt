add_executable(mlc
    main.cpp
    benchmark.cpp
)

target_link_libraries(
    mlc
    argparse
)

