# RTOS Task Scheduler Simulator

A command-line simulator, in modern C++17, of how a real-time operating system
schedules tasks on a single-core CPU: FCFS, Round Robin, Preemptive Priority,
Rate Monotonic and EDF, with Gantt charts, statistics and schedulability tests.

**Status:** Phase 1 (design and build skeleton). See [docs/DESIGN.md](docs/DESIGN.md)
for requirements, the class diagram, data structures and complexity analysis.

## Build

Requires CMake ≥ 3.16 and a C++17 compiler (GCC ≥ 9, Clang ≥ 10 or MSVC 2019+).

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/rtos_sim
ctest --test-dir build --output-on-failure
```

On Linux/macOS drop the `-G` flag.

**Windows note:** WinLibs ships its own `cmake.exe` built with GnuTLS. That build
has no CA certificates, so FetchContent fails with "SSL peer certificate ... not OK".
Put Kitware's CMake (`C:\Program Files\CMake\bin`) ahead of WinLibs on `PATH` so
it uses the Windows certificate store.

## Layout

```
include/rtos/   public headers
src/            implementation + main.cpp
tests/          GoogleTest unit tests
data/           sample task sets (CSV)
docs/           design notes
```
