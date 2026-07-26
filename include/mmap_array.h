#pragma once

#include <cstddef>
#include <span>

#include "benchmark.h"

/* Owning, move-only handle for a mmap'd array of Nodes. Frees the mapping
 * with munmap on destruction, so callers never have to. Hand out a
 * std::span for the actual work via span() or the implicit conversion. */
class MmapArray {
public:
    /* Maps space for `count` Nodes. When `use_huge_pages` is set the
     * mapping is backed by explicit 2 MiB HugeTLB pages (the pool must be
     * reserved beforehand, see /proc/meminfo). Throws std::bad_alloc on
     * failure. The memory is zero-filled, so every Node::next starts null. */
    MmapArray(std::size_t count, bool use_huge_pages);
    ~MmapArray();

    MmapArray(MmapArray&& other) noexcept;
    MmapArray& operator=(MmapArray&& other) noexcept;

    MmapArray(const MmapArray&) = delete;
    MmapArray& operator=(const MmapArray&) = delete;

    std::span<Node> span() noexcept { return {ptr_m, count_m}; }
    std::span<const Node> span() const noexcept { return {ptr_m, count_m}; }

    /* Pass an MmapArray straight into anything taking a std::span<Node>. */
    operator std::span<Node>() noexcept { return {ptr_m, count_m}; }

    Node* data() noexcept { return ptr_m; }
    const Node* data() const noexcept { return ptr_m; }
    std::size_t size() const noexcept { return count_m; }

private:
    void reset() noexcept;

    Node* ptr_m = nullptr;
    std::size_t count_m = 0;
    std::size_t bytes_m = 0;  // actual mapped length; needed by munmap
};
