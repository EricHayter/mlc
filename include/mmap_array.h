#pragma once

#include <cstddef>
#include <cstdio>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include <linux/mman.h> // MAP_HUGE_2MB
#include <sys/mman.h>

/* Owning, move-only handle for a mmap'd array of T. Frees the mapping with
 * munmap on destruction, so callers never have to. Hand out a std::span for
 * the actual work via span() or the implicit conversion.
 *
 * The storage is left zero-filled and destructors are never run, so T must be
 * trivial -- this is a raw buffer, not a general container. */
template <typename T> class MmapArray {
    static_assert(std::is_trivially_default_constructible_v<T>,
                  "MmapArray leaves storage zero-filled; T must be trivially "
                  "default constructible");
    static_assert(std::is_trivially_destructible_v<T>,
                  "MmapArray never runs destructors; T must be trivially "
                  "destructible");

  public:
    /* Maps space for `count` Ts. When `use_huge_pages` is set the mapping is
     * backed by explicit 2 MiB HugeTLB pages (the pool must be reserved
     * beforehand, see /proc/meminfo). Throws std::bad_alloc on failure. The
     * memory is zero-filled. */
    MmapArray(std::size_t count, bool use_huge_pages)
        : count_m(count), bytes_m(count * sizeof(T)) {
        int flags = MAP_PRIVATE | MAP_ANONYMOUS;
        if (use_huge_pages) {
            /* HugeTLB mappings must be a whole number of huge pages. */
            bytes_m = round_up(bytes_m, huge_page_size);
            flags |= MAP_HUGETLB | MAP_HUGE_2MB;
        }

        void *p = mmap(nullptr, bytes_m, PROT_READ | PROT_WRITE, flags, -1, 0);
        if (p == MAP_FAILED) {
            /* Most likely cause for the huge-page path is an empty pool. */
            std::perror("MmapArray: mmap");
            throw std::bad_alloc{};
        }
        ptr_m = static_cast<T *>(p);
    }

    ~MmapArray() { reset(); }

    MmapArray(MmapArray &&other) noexcept
        : ptr_m(std::exchange(other.ptr_m, nullptr)),
          count_m(std::exchange(other.count_m, 0)),
          bytes_m(std::exchange(other.bytes_m, 0)) {}

    MmapArray &operator=(MmapArray &&other) noexcept {
        if (this != &other) {
            reset();
            ptr_m = std::exchange(other.ptr_m, nullptr);
            count_m = std::exchange(other.count_m, 0);
            bytes_m = std::exchange(other.bytes_m, 0);
        }
        return *this;
    }

    MmapArray(const MmapArray &) = delete;
    MmapArray &operator=(const MmapArray &) = delete;

    std::span<T> span() noexcept { return {ptr_m, count_m}; }
    std::span<const T> span() const noexcept { return {ptr_m, count_m}; }

    /* Pass an MmapArray straight into anything taking a std::span<T>. */
    operator std::span<T>() noexcept { return {ptr_m, count_m}; }
    operator std::span<const T>() noexcept { return {ptr_m, count_m}; }

    T *data() noexcept { return ptr_m; }
    const T *data() const noexcept { return ptr_m; }
    std::size_t size() const noexcept { return count_m; }

  private:
    static constexpr std::size_t huge_page_size = std::size_t{2} << 20; // 2 MiB

    static std::size_t round_up(std::size_t n, std::size_t multiple) noexcept {
        return (n + multiple - 1) / multiple * multiple;
    }

    void reset() noexcept {
        if (ptr_m) {
            munmap(ptr_m, bytes_m);
            ptr_m = nullptr;
            count_m = 0;
            bytes_m = 0;
        }
    }

    T *ptr_m = nullptr;
    std::size_t count_m = 0;
    std::size_t bytes_m = 0; // actual mapped length; needed by munmap
};
