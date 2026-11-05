#pragma once
// Schedulability.hpp — offline tests that need no simulation, only maths.
//
// Utilization U = sum(C_i / T_i) over periodic tasks.
// Rate Monotonic (Liu & Layland, 1973): U <= n(2^(1/n) - 1) guarantees every
//   deadline is met. It is SUFFICIENT, not necessary: failing it means
//   "inconclusive", not "will miss" (run the simulation to find out).
// EDF: with deadlines equal to periods, U <= 1 is necessary AND sufficient.

#include <cstddef>
#include <vector>

#include "rtos/Task.hpp"

namespace rtos {

struct SchedulabilityReport {
    std::size_t periodicTasks = 0;
    double utilization = 0;
    double rmBound = 0;
    bool rmGuaranteed = false;     // U <= Liu & Layland bound
    bool edfFeasible = false;      // U <= 1
    bool implicitDeadlines = true; // every periodic task has D == T
};

class SchedulabilityAnalyzer {
public:
    static double utilization(const std::vector<Task>& tasks);
    /// n(2^(1/n) - 1); tends to ln 2 ≈ 0.693 as n grows. Returns 1 for n == 0.
    static double liuLaylandBound(std::size_t n);
    static SchedulabilityReport analyze(const std::vector<Task>& tasks);
};

} // namespace rtos
