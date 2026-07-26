#pragma once

#include <cstddef>
#include <span>
#include <utility>

#include "mmap_array.h"

/* Node structure to build linked lists in memory. Make sure to align to the
 * size of the cache line (64 bytes) so that a single load doesn't pull in
 * more than one Node. */
struct alignas(64) Node {
    Node *next;
};

/* Pin currently running thread to the given CPU */
void pin_to_cpu(int cpu_id);

/* Generates an array of Nodes at least buffer_size_kb kilobytes large, and
 * links them into a linked list structure to be walked by pointer_chase.
 * Can use huge pages if required. */
MmapArray<Node> generate_buffer(std::size_t buffer_size_kb,
                                bool use_huge_pages);

/* Pins to the given CPU, then performs pointer chasing over nodes for a total
 * of num_jumps. Returns the average per-load cost as a { nanoseconds, core
 * cycles } pair. */
std::pair<double, double> pointer_chase(int cpu, std::span<const Node> nodes,
                                        std::size_t num_jumps);
