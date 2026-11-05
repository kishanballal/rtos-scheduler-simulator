#include "rtos/Report.hpp"

#include <iomanip>
#include <sstream>

namespace rtos {

namespace {
std::string orDash(Tick v) { return v == kNoDeadline ? "-" : std::to_string(v); }
} // namespace

std::string formatTaskTable(const std::vector<Task>& tasks) {
    std::ostringstream os;
    os << "Tasks\n" << std::left
       << std::setw(5) << "id" << std::setw(16) << "name" << std::setw(9) << "arrival"
       << std::setw(7) << "burst" << std::setw(6) << "prio" << std::setw(8) << "period"
       << "deadline\n";
    for (const Task& t : tasks) {
        os << std::setw(5) << t.id() << std::setw(16) << t.name() << std::setw(9) << t.arrival()
           << std::setw(7) << t.burst() << std::setw(6) << t.priority()
           << std::setw(8) << (t.isPeriodic() ? std::to_string(t.period()) : "-")
           << orDash(t.relativeDeadline()) << '\n';
    }
    return os.str();
}

std::string formatSchedulability(const SchedulabilityReport& r) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << "Schedulability (" << r.periodicTasks
       << " periodic tasks)\n  U = " << r.utilization << '\n';
    if (r.periodicTasks == 0) return os.str() + "  (no periodic tasks: tests not applicable)\n";
    os << "  RM : U <= " << r.rmBound << " (Liu & Layland)? "
       << (r.rmGuaranteed ? "yes, guaranteed schedulable" : "no, inconclusive (simulate to check)")
       << '\n'
       << "  EDF: U <= 1? " << (r.edfFeasible ? "yes" : "no, overloaded")
       << (r.implicitDeadlines ? "" : " (deadlines < periods: test is necessary only)") << '\n';
    return os.str();
}

std::string formatStats(const StatsReport& r) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << std::left
       << std::setw(16) << "task" << std::setw(6) << "jobs" << std::setw(6) << "done"
       << std::setw(8) << "wait" << std::setw(8) << "tat" << std::setw(8) << "resp" << "missed\n";
    for (const TaskStats& s : r.perTask) {
        os << std::setw(16) << s.name << std::setw(6) << s.released << std::setw(6) << s.completed
           << std::setw(8) << s.avgWaiting << std::setw(8) << s.avgTurnaround
           << std::setw(8) << s.avgResponse << s.deadlineMisses << '\n';
    }
    const OverallStats& o = r.overall;
    os << "\nCPU utilization " << o.cpuUtilization * 100 << "%  |  context switches "
       << o.contextSwitches << "  |  throughput " << std::setprecision(3) << o.throughput
       << " jobs/tick  |  deadline misses " << o.deadlineMisses << '\n';
    return os.str();
}

std::string formatComparison(const std::vector<StatsReport>& reports) {
    std::ostringstream os;
    os << "Comparison\n" << std::fixed << std::setprecision(2) << std::left
       << std::setw(13) << "algo" << std::setw(8) << "wait" << std::setw(8) << "tat"
       << std::setw(8) << "resp" << std::setw(8) << "misses" << std::setw(10) << "switches"
       << std::setw(8) << "util%" << "done\n";
    for (const StatsReport& r : reports) {
        const OverallStats& o = r.overall;
        os << std::setw(13) << r.schedulerName << std::setw(8) << o.avgWaiting
           << std::setw(8) << o.avgTurnaround << std::setw(8) << o.avgResponse
           << std::setw(8) << o.deadlineMisses << std::setw(10) << o.contextSwitches
           << std::setw(8) << o.cpuUtilization * 100 << o.completed << '\n';
    }
    return os.str();
}

} // namespace rtos
