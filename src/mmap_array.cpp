#include "mmap_array.h"

#include <cstdio>
#include <new>
#include <utility>

#include <linux/mman.h>  // MAP_HUGE_2MB
#include <sys/mman.h>

namespace {

constexpr std::size_t huge_page_size = std::size_t{2} << 20;  // 2 MiB

std::size_t round_up(std::size_t n, std::size_t multiple) noexcept {
    return (n + multiple - 1) / multiple * multiple;
}

}  // namespace

MmapArray::MmapArray(std::size_t count, bool use_huge_pages)
    : count_m(count), bytes_m(count * sizeof(Node)) {
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (use_huge_pages) {
        /* HugeTLB mappings must be a whole number of huge pages. */
        bytes_m = round_up(bytes_m, huge_page_size);
        flags |= MAP_HUGETLB | MAP_HUGE_2MB;
    }

    void* p = mmap(nullptr, bytes_m, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (p == MAP_FAILED) {
        /* Most likely cause for the huge-page path is an empty pool. */
        std::perror("MmapArray: mmap");
        throw std::bad_alloc{};
    }
    ptr_m = static_cast<Node*>(p);
    /* MAP_ANONYMOUS memory is zero-filled and Node is trivially
     * constructible, so there are no constructors to run. */
}

MmapArray::~MmapArray() {
    reset();
}

void MmapArray::reset() noexcept {
    if (ptr_m) {
        munmap(ptr_m, bytes_m);
        ptr_m = nullptr;
        count_m = 0;
        bytes_m = 0;
    }
}

MmapArray::MmapArray(MmapArray&& other) noexcept
    : ptr_m(std::exchange(other.ptr_m, nullptr)),
      count_m(std::exchange(other.count_m, 0)),
      bytes_m(std::exchange(other.bytes_m, 0)) {}

MmapArray& MmapArray::operator=(MmapArray&& other) noexcept {
    if (this != &other) {
        reset();
        ptr_m = std::exchange(other.ptr_m, nullptr);
        count_m = std::exchange(other.count_m, 0);
        bytes_m = std::exchange(other.bytes_m, 0);
    }
    return *this;
}
