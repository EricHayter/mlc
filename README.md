# mlc

A from-scratch implementation of Intel's Memory Latency Checker.

`mlc` measures the average latency of a single dependent memory load across a
range of buffer sizes. It allocates a buffer, links it into one big randomized
cycle of cache-line-sized nodes, then walks the cycle (pointer chasing) so that
each load depends on the previous one and the hardware prefetcher can't hide the
latency. Sweeping the buffer size from a few KiB up to 128 MiB walks the load
down through the cache hierarchy, and finally DRAM so the latency
curve reveals the size and cost of each level.

For every buffer size it reports the average per-load cost in both nanoseconds
(wall-clock) and core CPU cycles (via a Linux `perf_event` hardware counter):

```
buffer size (kb), average latency per load (ns), cycles per load
8, 1.02, 4.60
10, 1.01, 4.55
...
131072, 92.31, 415.40
```

## Building

The project uses CMake (>= 3.31, C++23) and vendors [p-ranav/argparse] as a git
submodule, so clone recursively:

```sh
git clone --recurse-submodules https://github.com/EricHayter/mlc.git
cd mlc
```

If you already cloned without `--recurse-submodules`, pull the submodule in:

```sh
git submodule update --init --recursive
```

Then configure and build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary is produced at `build/src/mlc`.

## Enabling perf monitoring

`mlc` reads core cycle counts through the kernel's `perf_event` interface, which
unprivileged processes can only use when `kernel.perf_event_paranoid` is `2` or
lower. Many distros ship a stricter default (`3`, or `4` on some hardened
kernels), in which case the counter fails to open and `mlc` aborts with
"Failed to instantiate cycle counter".

Lower it to `2` for the current boot:

```sh
sudo sysctl kernel.perf_event_paranoid=2
```

To make it persist across reboots:

```sh
echo 'kernel.perf_event_paranoid=2' | sudo tee /etc/sysctl.d/99-perf.conf
sudo sysctl --system
```

`mlc` only ever counts user-space cycles (kernel and hypervisor cycles are
excluded), so `2` is sufficient — you do not need to drop to `-1` or run as root.

## Running

`mlc` pins itself to a single CPU for the duration of the sweep so the results
aren't perturbed by migration. Pass the CPU id to pin to with `--id` (required):

```sh
./build/src/mlc --id 0
```

Output is CSV on stdout, so you can redirect it straight to a file for plotting:

```sh
./build/src/mlc --id 0 > latencies.csv
```

## Viewing results

The `web/` directory holds a small self-contained viewer that plots the latency
curve (log-scaled buffer size, with `ns` and `cycles` on separate axes) so the
cache-hierarchy plateaus are easy to read. It has no build step and no runtime
dependencies beyond a browser.

The quickest path is to pipe `mlc` straight into `scripts/mlc-view`, which
inlines the data into a standalone HTML report and opens it in your browser — no
saving or file-picking:

```sh
./build/src/mlc --id 0 | scripts/mlc-view
```

You can also point it at a saved CSV, or keep the generated report:

```sh
scripts/mlc-view latencies.csv                 # from a file
./build/src/mlc --id 0 | scripts/mlc-view -o report.html   # write a shareable file
```

Prefer to skip the wrapper? Open `web/index.html` directly and drop a CSV onto
the page, choose one with the file picker, paste it in, or hit **Load sample**.

## Huge pages

By default the buffer is backed by ordinary 4 KiB pages, so at larger sizes the
TLB miss rate climbs and inflates the measured latency. Passing `-H` / `--huge`
backs the buffer with explicit 2 MiB HugeTLB pages instead, which keeps the TLB
footprint small and isolates cache/DRAM latency from TLB effects.

Explicit huge pages must be reserved in the kernel's pool *before* running —
`mlc` maps them with `MAP_HUGETLB` and will fail if the pool is empty. The
largest buffer is 128 MiB, so reserve at least 64 pages (64 × 2 MiB = 128 MiB):

```sh
sudo sysctl vm.nr_hugepages=64
```

Confirm the reservation took effect (the request can be partially denied if
memory is fragmented):

```sh
grep Huge /proc/meminfo
```

Then run with huge pages enabled:

```sh
./build/src/mlc --id 0 --huge
```

[p-ranav/argparse]: https://github.com/p-ranav/argparse
