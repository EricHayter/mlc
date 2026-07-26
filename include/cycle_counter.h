#pragma once

#include <cstddef>

/* RAII wrapper around a Linux perf_event hardware counter that measures the
 * number of core CPU cycles executed by the calling thread. Core cycles (as
 * opposed to the fixed-rate TSC) scale with the current clock, so the count is
 * frequency-independent.
 *
 * Usage: construct, then bracket the region of interest with start()/stop().
 * The counter follows the thread, so pin before measuring. */
class CycleCounter {
  public:
    /* Opens (but does not yet start) a cycle counter for the calling thread,
     * restricted to the given CPU. Counts user-space cycles only. Aborts if
     * the counter can't be opened -- most likely a permissions issue, which
     * needs kernel.perf_event_paranoid <= 2 (see /proc/sys/kernel). */
    explicit CycleCounter(int cpu);

    /* Closes the underlying perf_event file descriptor. */
    ~CycleCounter();

    /* Delete copy and move semantics to keep this simple but could implement
     * in the future */
    CycleCounter(const CycleCounter &) = delete;
    CycleCounter &operator=(const CycleCounter &) = delete;

    CycleCounter(CycleCounter &&) = delete;
    CycleCounter &operator=(CycleCounter &&) = delete;

    /* Resets the count to zero and begins counting. */
    void start();

    /* Stops counting and returns the cycles elapsed since the last start(). */
    std::size_t stop();

  private:
    int perf_fd_m{}; // perf_event fd, read for the running cycle count
};
