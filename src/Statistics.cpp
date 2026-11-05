#include "rtos/Statistics.hpp"

#include <algorithm>
#include <unordered_map>

namespace rtos {

namespace {
struct Sums {
    double waiting = 0, turnaround = 0, response = 0;
    void add(double w, double t, double r) { waiting += w; turnaround += t; response += r; }
};
double avg(double sum, int n) { return n > 0 ? sum / n : 0.0; }
} // namespace

StatsReport Statistics::compute(const SimulationResult& result, const std::vector<Task>& tasks) {
    StatsReport report;
    report.schedulerName = result.schedulerName;

    // Hash map: task id -> row index, so each job is attributed in O(1).
    std::unordered_map<int, std::size_t> row;
    for (const Task& t : tasks) {
        row.emplace(t.id(), report.perTask.size());
        TaskStats s;
        s.taskId = t.id();
        s.name = t.name();
        report.perTask.push_back(s);
    }

    std::vector<Sums> sums(tasks.size());
    Sums all;
    OverallStats& o = report.overall;

    for (const Job& j : result.jobs) {
        const std::size_t r = row.at(j.task->id());
        TaskStats& s = report.perTask[r];
        ++s.released;

        const bool missed = j.finish ? *j.finish > j.absDeadline : j.absDeadline <= result.length;
        if (missed) { ++s.deadlineMisses; ++o.deadlineMisses; }

        if (!j.finish) continue;
        ++s.completed;
        ++o.completed;
        const auto turnaround = static_cast<double>(*j.finish - j.release);
        const double waiting = turnaround - static_cast<double>(j.task->burst());
        const auto response = static_cast<double>(*j.start - j.release);
        sums[r].add(waiting, turnaround, response);
        all.add(waiting, turnaround, response);
    }

    for (std::size_t i = 0; i < report.perTask.size(); ++i) {
        TaskStats& s = report.perTask[i];
        s.avgWaiting = avg(sums[i].waiting, s.completed);
        s.avgTurnaround = avg(sums[i].turnaround, s.completed);
        s.avgResponse = avg(sums[i].response, s.completed);
    }

    const auto busy = std::count_if(result.timeline.begin(), result.timeline.end(),
                                    [](int id) { return id != kIdle; });
    o.length = result.length;
    o.contextSwitches = result.contextSwitches;
    if (result.length > 0) {
        o.cpuUtilization = static_cast<double>(busy) / static_cast<double>(result.length);
        o.throughput = o.completed / static_cast<double>(result.length);
    }
    o.avgWaiting = avg(all.waiting, o.completed);
    o.avgTurnaround = avg(all.turnaround, o.completed);
    o.avgResponse = avg(all.response, o.completed);
    return report;
}

} // namespace rtos
