#include "benchmark.h"

#include "cycle_counter.h"
#include "mmap_array.h"

#include <sched.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <vector>

void pin_to_cpu(int cpu_id) {
    if (cpu_id < 0) {
        std::cerr << std::format("Invalid CPU id {}\n", cpu_id);
        std::abort();
    }

    cpu_set_t cpu_set{};
    CPU_ZERO(&cpu_set);
    /* CPU_SET's glibc macro trips -Wsign-conversion internally; silence it
     * just for this call now that cpu_id is known to be non-negative. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    CPU_SET(cpu_id, &cpu_set);
#pragma GCC diagnostic pop
    if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == -1) {
        std::cerr << std::format("Failed to pin CPU #{}: {}\n", cpu_id,
                                 std::strerror(errno));
        std::abort();
    }
}

MmapArray<Node> generate_buffer(std::size_t buffer_size_kb,
                                bool use_huge_pages) {
    /* allocate enough nodes such that we use approximately buffer_size_kb kb */
    const std::size_t num_nodes = buffer_size_kb * 1024 / sizeof(Node);

    /* MmapArray owns the mapping and munmaps it on destruction. */
    MmapArray<Node> buffer(num_nodes, use_huge_pages);
    std::span<Node> nodes = buffer.span();

    /* to avoid the hardware prefetcher from picking up on any patterns in
     * our data accesses we randomly shuffle the pointers for each of the
     * nodes */
    std::vector<std::size_t> visit_order(num_nodes);
    std::ranges::iota(visit_order, std::size_t{0});
    std::random_device rd;
    std::mt19937 g(rd());
    std::ranges::shuffle(visit_order, g);

    /* generate the cycle of all of the nodes */
    Node *node = &nodes[visit_order[0]];
    for (std::size_t next_node : visit_order | std::views::drop(1)) {
        node->next = &nodes[next_node];
        node = node->next;
    }
    node->next = &nodes[visit_order[0]];

    return buffer;
}

std::pair<double, double> pointer_chase(int cpu, std::span<const Node> nodes,
                                        std::size_t num_jumps) {
    pin_to_cpu(cpu);

    CycleCounter cycle_counter(cpu);

    const Node *node = nodes.data();

    const auto start = std::chrono::steady_clock::now();
    cycle_counter.start();

    for (std::size_t i = 0; i < num_jumps; i++) {
        node = node->next;
    }

    const std::size_t cycle_count = cycle_counter.stop();
    const auto stop = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::nano> duration = stop - start;

    /* Shouldn't happen; this is just to prevent the compiler from optimizing
     * out node and the jumps */
    if (node == nullptr) {
        std::cerr << "Pointer chase failed due to missing cycle in buffer\n";
        std::abort();
    }

    const auto jumps = static_cast<double>(num_jumps);
    return {duration.count() / jumps, static_cast<double>(cycle_count) / jumps};
}
