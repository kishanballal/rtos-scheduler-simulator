#pragma once
// Types.hpp — shared vocabulary types for the whole simulator.

#include <cstdint>
#include <limits>

namespace rtos {

/// One unit of simulated time. Signed so that differences (e.g. lateness) are safe.
using Tick = std::int64_t;

/// Sentinel meaning "this job has no deadline".
inline constexpr Tick kNoDeadline = std::numeric_limits<Tick>::max();

/// Timeline marker for a tick where the CPU ran nothing.
inline constexpr int kIdle = -1;

} // namespace rtos
