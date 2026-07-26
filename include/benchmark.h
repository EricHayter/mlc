#pragma once

#include <cstddef>
#include <span>

#include "mmap_array.h"

/* Node structure to build linked lists in memory. Make sure to align to the
 * size of the cache line (64 bytes) so that a single load doesn't pull in
 * more than one Node. */
struct alignas(64) Node {
    Node* next;
};

/* Performs a benchmark on the given CPU to determine read latency parametrized
 * by the buffer size it accesses, and whether or not huge pages will be used
 * (to minimize the effect of TLB misses on averages). */
std::size_t benchmark(int cpu_id, std::size_t buffer_size_kb, bool use_huge_pages);

/* Pin currently running thread to the given CPU */
void pin_to_cpu(int cpu_id);

/* Generates an array of Nodes at least buffer_size_kb kilobytes large, and
 * links them into a linked list structure to be walked by pointer_chase.
 * Can use huge pages if required. */
MmapArray<Node> generate_buffer(std::size_t buffer_size_kb, bool use_huge_pages);

/* Given an array of nodes, performs pointer chasing for a total of num_jumps,
 * then returns the average latency of each load (in ns). */
std::size_t pointer_chase(std::span<const Node> nodes, std::size_t num_jumps);
