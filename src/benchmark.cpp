#include "benchmark.h"

#include "mmap_array.h"

#include <sched.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <vector>

std::size_t benchmark(int cpu_id, std::size_t buffer_size_kb, bool use_huge_pages) {
    pin_to_cpu(cpu_id);

    /* Keep the buffer alive for the whole measurement; it frees itself
     * (munmap) when it goes out of scope at the end of this function. */
    MmapArray buffer = generate_buffer(buffer_size_kb, use_huge_pages);
    constexpr std::size_t num_jumps = 1e8;
    return pointer_chase(buffer, num_jumps);
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

MmapArray generate_buffer(std::size_t buffer_size_kb, bool use_huge_pages)
{
    /* allocate enough nodes such that we use approximately buffer_size_kb kb */
    const std::size_t num_nodes = buffer_size_kb * 1024 / sizeof(Node);

    /* MmapArray owns the mapping and munmaps it on destruction. */
    MmapArray buffer(num_nodes, use_huge_pages);
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

std::size_t pointer_chase(std::span<const Node> nodes, std::size_t num_jumps)
{
    const Node* node = &nodes[0];

    /* load all of the nodes into the lower layer caches (L3, DRAM, etc...) */
    for (std::size_t i = 0; i < nodes.size(); i++) {
        node = node->next;
    }

    std::chrono::steady_clock clock;
    const auto start = clock.now();

    for (std::size_t i = 0; i < num_jumps; i++) {
        node = node->next;
    }

    const auto stop = clock.now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    /* Shouldn't happen this is just to prevent the compiler from optimizing
     * out node and the jumps */
    if (node == nullptr) {
        std::cerr << "Pointer chase failed due to missing cycle in buffer\n";
        std::abort();
    }

    return duration.count() / num_jumps;
}
