#pragma once
// Clock.hpp — the single source of truth for simulated time.
// Like the FreeRTOS tick counter (xTickCount) it only ever moves forward by one.

#include "rtos/Types.hpp"

namespace rtos {

class Clock {
public:
    Tick now() const noexcept { return now_; }
    void tick() noexcept { ++now_; }
    void reset() noexcept { now_ = 0; }

private:
    Tick now_ = 0;
};

} // namespace rtos
