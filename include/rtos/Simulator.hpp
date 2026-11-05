#pragma once
// Simulator.hpp — the discrete-time engine. It decides WHEN things happen
// (releases, ticks, completions); the Scheduler decides WHO runs.

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "rtos/Clock.hpp"
#include "rtos/Job.hpp"
#include "rtos/Scheduler.hpp"
#include "rtos/Task.hpp"

namespace rtos {

/// Everything one run produced. Job::task pointers refer to the Simulator's
/// tasks, so keep the Simulator alive while you use the result.
struct SimulationResult {
    std::string schedulerName;
    Tick length = 0;                 // ticks actually simulated
    std::vector<int> timeline;       // task id that ran in each tick, or kIdle
    std::vector<Job> jobs;           // final state of every released job
    int contextSwitches = 0;         // CPU handed from one job to a different one
};

class Simulator {
public:
    /// @throws std::invalid_argument on duplicate task ids.
    explicit Simulator(std::vector<Task> tasks);

    /// Swap the algorithm at runtime (Strategy pattern). Takes ownership.
    void setScheduler(std::unique_ptr<Scheduler> scheduler);

    /// Runs ticks [0, horizon). Stops early if nothing is left to do.
    SimulationResult run(Tick horizon);

    /// One hyperperiod after the last arrival for periodic sets; for purely
    /// aperiodic sets, long enough for every job to finish.
    Tick defaultHorizon() const;

    const std::vector<Task>& tasks() const noexcept { return tasks_; }

    /// O(1) average lookup through the hash map. Returns nullptr if absent.
    const Task* findTask(int id) const;

private:
    std::vector<Task> tasks_;                    // never resized after construction,
    std::unordered_map<int, std::size_t> byId_;  // so pointers into it stay valid
    std::unique_ptr<Scheduler> scheduler_;
    Clock clock_;
};

} // namespace rtos
