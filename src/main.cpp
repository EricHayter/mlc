#include <argparse/argparse.hpp>

#include <format>
#include <iostream>

#include "benchmark.h"

int main(int argc, const char** argv) {
    argparse::ArgumentParser program("mlc");

    program.add_argument("-H", "--huge")
        .help("Use huge pages")
        .flag();

    program.add_argument("--id")
        .required()
        .help("CPU id to use")
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
        std::cerr << program;
        return 1;
    }

    const int cpu_id = program.get<int>("--id");

    for (std::size_t buffer_size_kb = 100; buffer_size_kb < 1000; buffer_size_kb += 100) {
        std::size_t latency_ns = benchmark(cpu_id, buffer_size_kb, true);
        std::cout << std::format("{}, {}\n", buffer_size_kb, latency_ns);
    }
}
