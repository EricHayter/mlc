#pragma once

#include <cstddef>
#include <span>

struct alignas(64) Node {
    Node* next;
};

class MmapArray;  // owning buffer handle, defined in mmap_array.h

std::size_t benchmark(int cpu_id, std::size_t buffer_size_kb, bool use_huge_pages);

void pin_to_cpu(int cpu_id);

MmapArray generate_buffer(std::size_t buffer_size_kb, bool use_huge_pages);

void pointer_chase(std::span<const Node> nodes, std::size_t num_jumps);
