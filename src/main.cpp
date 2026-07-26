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

    const bool use_huge_pages = program.get<bool>("--huge");
    const int cpu_id = program.get<int>("--id");

    /* Grow the buffer geometrically (~1.25x per step) so the points are
     * evenly spaced on a log axis and span L1 through DRAM without either
     * exploding in count or racing past the interesting boundaries. The
     * top end (128 MiB) is well beyond the 3 MiB L3 to capture DRAM. */
    for (std::size_t buffer_size_kb = 8; buffer_size_kb <= 128 * 1024;
         buffer_size_kb += buffer_size_kb / 4) {
        std::size_t latency_ns = benchmark(cpu_id, buffer_size_kb, use_huge_pages);
        std::cout << std::format("{}, {}\n", buffer_size_kb, latency_ns);
    }
}
