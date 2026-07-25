#include <argparse/argparse.hpp>

#include <format>
#include <iostream>

std::size_t benchmark(std::size_t buffer_size_kb, bool use_huge_pages);

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

    for (std::size_t buffer_size_kb = 100; buffer_size_kb < 1024; buffer_size_kb += 100) {
        std::size_t latency_ns = benchmark(buffer_size_kb, true);
        std::cout << std::format("{}, {}\n", buffer_size_kb, latency_ns);
    }
}
