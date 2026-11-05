#pragma once
// Task.hpp — the static description of a task (what the user writes in the CSV).
// Immutable after construction: the constructor validates every field, so any
// Task object that exists is guaranteed to be well-formed (class invariant).

#include <iosfwd>
#include <optional>
#include <string>

#include "rtos/Types.hpp"

namespace rtos {

/// A section of a job's work that must hold a shared resource (mutex).
/// The job locks `resource` after `start` ticks of work and holds it for
/// `length` ticks of work. Example: "R1:1:3" = lock after 1 tick, hold for 3.
struct CriticalSection {
    std::string resource;
    Tick start = 0;
    Tick length = 0;
};

class Task {
public:
    /// @param period    0 means aperiodic (released once at `arrival`).
    /// @param deadline  relative deadline; 0 means "use the period" (or none if aperiodic).
    /// @param cs        optional critical section on a shared resource.
    /// @throws std::invalid_argument if any field is out of range.
    Task(int id, std::string name, Tick arrival, Tick burst, int priority,
         Tick period = 0, Tick deadline = 0, std::optional<CriticalSection> cs = std::nullopt);

    const std::optional<CriticalSection>& criticalSection() const noexcept { return cs_; }

    int id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    Tick arrival() const noexcept { return arrival_; }
    Tick burst() const noexcept { return burst_; }
    int priority() const noexcept { return priority_; }   // lower number = more urgent
    Tick period() const noexcept { return period_; }
    bool isPeriodic() const noexcept { return period_ > 0; }

    /// Explicit deadline if given, else the period, else kNoDeadline.
    Tick relativeDeadline() const noexcept;

    /// C / T for periodic tasks, 0 for aperiodic ones.
    double utilization() const noexcept;

private:
    int id_;
    std::string name_;
    Tick arrival_;
    Tick burst_;
    int priority_;
    Tick period_;
    Tick deadline_;
    std::optional<CriticalSection> cs_;
};

/// Operator overloading where it is natural: lets us write `std::cout << task`.
std::ostream& operator<<(std::ostream& os, const Task& task);

} // namespace rtos
