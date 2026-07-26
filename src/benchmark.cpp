#include "benchmark.h"

#include "mmap_array.h"
#include "cycle_counter.h"

#include <sched.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <vector>

std::pair<std::size_t, std::size_t> benchmark(int cpu, std::size_t buffer_size_kb, bool use_huge_pages) {
    MmapArray<Node> buffer = generate_buffer(buffer_size_kb, use_huge_pages);
    constexpr std::size_t num_jumps = 1e8;
    return pointer_chase(cpu, buffer, num_jumps);
}

void pin_to_cpu(int cpu_id)
{
    cpu_set_t cpu_set{};
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == -1) {
        std::cerr << std::format("Failed to pin CPU #{}: {}\n", 0, strerror(errno));
        std::abort();
    }
}

MmapArray<Node> generate_buffer(std::size_t buffer_size_kb, bool use_huge_pages)
{
    /* allocate enough nodes such that we use approximately buffer_size_kb kb */
    const std::size_t num_nodes = buffer_size_kb * 1024 / sizeof(Node);

    /* MmapArray owns the mapping and munmaps it on destruction. */
    MmapArray<Node> buffer(num_nodes, use_huge_pages);
    std::span<Node> nodes = buffer.span();

    /* to avoid the hardware prefetcher from picking up on any patterns in
     * our data accesses we randomly shuffle the pointers for each of the
     * nodes */
    std::vector<int> visit_order(num_nodes);
    std::iota(std::begin(visit_order), std::end(visit_order), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(std::begin(visit_order), std::end(visit_order), g);

    /* generate the cycle of all of the nodes */
    Node* node = &nodes[visit_order[0]];
    for (int next_node: visit_order | std::views::drop(1)) {
        node->next = &nodes[next_node];
        node = node->next;
    }
    node->next = &nodes[visit_order[0]];

    return buffer;
}

std::pair<std::size_t, std::size_t> pointer_chase(int cpu, std::span<const Node> nodes, std::size_t num_jumps)
{
    pin_to_cpu(cpu);

    CycleCounter cycle_counter(cpu);

    const Node* node = &nodes[0];

    std::chrono::steady_clock clock;
    const auto start = clock.now();
    cycle_counter.start();

    for (std::size_t i = 0; i < num_jumps; i++) {
        node = node->next;
    }

    const std::size_t cycle_count = cycle_counter.stop();
    const auto stop = clock.now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    /* Shouldn't happen; this is just to prevent the compiler from optimizing
     * out node and the jumps */
    if (node == nullptr) {
        std::cerr << "Pointer chase failed due to missing cycle in buffer\n";
        std::abort();
    }

    return { duration.count() / num_jumps, cycle_count / num_jumps };
}
