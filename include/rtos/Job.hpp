#pragma once
// Job.hpp — one runtime release of a Task. A periodic task produces a new Job
// every period; an aperiodic task produces exactly one. Schedulers order Jobs,
// never Tasks, because two releases of the same task have different deadlines.

#include <optional>

#include "rtos/Task.hpp"
#include "rtos/Types.hpp"

namespace rtos {

struct Job {
    int jobId = 0;                 // unique, in release order
    const Task* task = nullptr;    // non-owning: the Simulator owns all Tasks
    Tick release = 0;              // absolute release time
    Tick remaining = 0;            // ticks of work still to do
    Tick absDeadline = kNoDeadline;
    std::optional<Tick> start;     // first tick it ran (for response time)
    std::optional<Tick> finish;    // tick after its last unit of work
    int effectivePriority = 0;     // base priority, raised by Priority Inheritance

    /// Ticks of work already done.
    Tick progress() const noexcept { return task->burst() - remaining; }

    /// Builds a fresh job for `task` released at `release`.
    static Job make(int jobId, const Task& task, Tick release) {
        const Tick rel = task.relativeDeadline();
        return Job{jobId, &task, release, task.burst(),
                   rel == kNoDeadline ? kNoDeadline : release + rel,
                   std::nullopt, std::nullopt, task.priority()};
    }
};

} // namespace rtos
