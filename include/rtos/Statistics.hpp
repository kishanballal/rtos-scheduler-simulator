#pragma once
// Statistics.hpp — turns a SimulationResult into numbers. A pure function:
// no I/O, no state, so it is trivial to unit test.
//
// Per job (completed jobs only):
//   turnaround = finish - release
//   waiting    = turnaround - burst
//   response   = first start - release
// A job misses its deadline if it finished after it, or never finished and
// its deadline fell inside the simulated window.

#include <string>
#include <vector>

#include "rtos/Simulator.hpp"

namespace rtos {

struct TaskStats {
    int taskId = 0;
    std::string name;
    int released = 0;
    int completed = 0;
    int deadlineMisses = 0;
    double avgWaiting = 0;
    double avgTurnaround = 0;
    double avgResponse = 0;
};

struct OverallStats {
    Tick length = 0;
    int completed = 0;
    int deadlineMisses = 0;
    int contextSwitches = 0;
    double cpuUtilization = 0;   // busy ticks / total ticks, 0..1
    double throughput = 0;       // completed jobs per tick
    double avgWaiting = 0;
    double avgTurnaround = 0;
    double avgResponse = 0;
};

struct StatsReport {
    std::string schedulerName;
    std::vector<TaskStats> perTask;   // same order as the task list
    OverallStats overall;
};

class Statistics {
public:
    static StatsReport compute(const SimulationResult& result, const std::vector<Task>& tasks);
};

} // namespace rtos
