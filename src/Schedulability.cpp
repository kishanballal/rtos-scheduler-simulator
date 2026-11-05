#include "rtos/Schedulability.hpp"

#include <cmath>

namespace rtos {

double SchedulabilityAnalyzer::utilization(const std::vector<Task>& tasks) {
    double u = 0;
    for (const Task& t : tasks) u += t.utilization();
    return u;
}

double SchedulabilityAnalyzer::liuLaylandBound(std::size_t n) {
    if (n == 0) return 1.0;
    const auto dn = static_cast<double>(n);
    return dn * (std::pow(2.0, 1.0 / dn) - 1.0);
}

SchedulabilityReport SchedulabilityAnalyzer::analyze(const std::vector<Task>& tasks) {
    constexpr double kEps = 1e-9;   // guard against floating-point round-off at the boundary
    SchedulabilityReport r;
    for (const Task& t : tasks) {
        if (!t.isPeriodic()) continue;
        ++r.periodicTasks;
        if (t.relativeDeadline() != t.period()) r.implicitDeadlines = false;
    }
    r.utilization = utilization(tasks);
    r.rmBound = liuLaylandBound(r.periodicTasks);
    r.rmGuaranteed = r.utilization <= r.rmBound + kEps;
    r.edfFeasible = r.utilization <= 1.0 + kEps;
    return r;
}

} // namespace rtos
