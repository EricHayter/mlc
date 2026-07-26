#include "cycle_counter.h"

#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

CycleCounter::CycleCounter(int cpu) {
    perf_event_attr perf_attr{};
    std::memset(&perf_attr, 0x00, sizeof(perf_attr));
    perf_attr.type = PERF_TYPE_HARDWARE;
    perf_attr.config = PERF_COUNT_HW_CPU_CYCLES; /* keep track of CPU core cycles */
    perf_attr.size = sizeof(perf_attr);
    perf_attr.disabled = 1; /* have the counter initially disabled */
    perf_attr.exclude_kernel = 1; /* exclude CPU cycles used in kernel */
    perf_attr.exclude_hv = 1; /* exclude CPU cycles in hyper visor */

    perf_fd_m = syscall(SYS_perf_event_open, &perf_attr,
            0 /* pid (in this case measure for current proccess) */,
            cpu, -1 /* group ID */, 0x00 /* flags */);
    if (perf_fd_m == -1) {
        std::perror("Failed to instantiate cycle counter");
        std::abort();
    }
}

CycleCounter::~CycleCounter()
{
    close(perf_fd_m);
}

void CycleCounter::start()
{
    ioctl(perf_fd_m, PERF_EVENT_IOC_RESET, 0);
    ioctl(perf_fd_m, PERF_EVENT_IOC_ENABLE, 0);
}

std::size_t CycleCounter::stop()
{
    std::uint64_t count{};
    ioctl(perf_fd_m, PERF_EVENT_IOC_DISABLE, 0);
    read(perf_fd_m, &count, sizeof(count));
    return count;
}
