#include "rtos/Simulator.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>

#include "rtos/MinHeap.hpp"

namespace rtos {

namespace {

/// An entry in the event queue: "task X releases a job at time T".
struct Release {
    Tick time;
    const Task* task;
};

struct EarlierRelease {
    bool operator()(const Release& a, const Release& b) const {
        if (a.time != b.time) return a.time < b.time;
        return a.task->id() < b.task->id();
    }
};

} // namespace

Simulator::Simulator(std::vector<Task> tasks) : tasks_(std::move(tasks)) {
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        if (!byId_.emplace(tasks_[i].id(), i).second)
            throw std::invalid_argument("duplicate task id " + std::to_string(tasks_[i].id()));
    }
}

void Simulator::setScheduler(std::unique_ptr<Scheduler> scheduler) {
    scheduler_ = std::move(scheduler);
}

const Task* Simulator::findTask(int id) const {
    const auto it = byId_.find(id);
    return it == byId_.end() ? nullptr : &tasks_[it->second];
}

Tick Simulator::defaultHorizon() const {
    constexpr Tick kCap = 10000;
    Tick hyper = 0, maxArrival = 0, totalBurst = 0;
    for (const Task& t : tasks_) {
        maxArrival = std::max(maxArrival, t.arrival());
        totalBurst += t.burst();
        if (t.isPeriodic()) hyper = hyper == 0 ? t.period() : std::lcm(hyper, t.period());
        if (hyper > kCap) return kCap;
    }
    const Tick horizon = hyper > 0 ? maxArrival + hyper : maxArrival + totalBurst;
    return std::min(std::max<Tick>(horizon, 1), kCap);
}

SimulationResult Simulator::run(Tick horizon) {
    if (!scheduler_) throw std::logic_error("Simulator::run called with no scheduler set");
    if (horizon <= 0) throw std::invalid_argument("horizon must be > 0");

    // Event queue: future releases ordered by time. Popping the next release is
    // O(log n) instead of scanning every task on every tick.
    MinHeap<Release, EarlierRelease> events;
    for (const Task& t : tasks_) events.push({t.arrival(), &t});

    std::vector<std::unique_ptr<Job>> jobs;   // owns every job; schedulers hold Job*
    SimulationResult result;
    result.schedulerName = scheduler_->name();
    result.timeline.reserve(static_cast<std::size_t>(horizon));

    Job* running = nullptr;       // ran last tick and still has work
    const Job* lastRun = nullptr; // last job that used the CPU (for context switches)

    for (clock_.reset(); clock_.now() < horizon; clock_.tick()) {
        const Tick now = clock_.now();

        // 1. Release every job due now.
        while (!events.empty() && events.top().time <= now) {
            const Release ev = events.pop();
            jobs.push_back(std::make_unique<Job>(
                Job::make(static_cast<int>(jobs.size()), *ev.task, ev.time)));
            scheduler_->onJobArrival(jobs.back().get());
            if (ev.task->isPeriodic()) events.push({ev.time + ev.task->period(), ev.task});
        }

        // 2. Nothing running, nothing ready, nothing coming: we are done.
        if (running == nullptr && !scheduler_->hasReadyJobs() && events.empty()) break;

        // 3. Ask the strategy who runs.
        Job* next = scheduler_->pickNext(running, now);
        if (next == nullptr) {
            result.timeline.push_back(kIdle);
            running = nullptr;
            continue;
        }
        if (lastRun != nullptr && next != lastRun) ++result.contextSwitches;

        // 4. Execute one tick of work.
        if (!next->start) next->start = now;
        --next->remaining;
        result.timeline.push_back(next->task->id());
        lastRun = next;

        // 5. Completion.
        if (next->remaining == 0) {
            next->finish = now + 1;
            scheduler_->onJobComplete(next);
            running = nullptr;
        } else {
            running = next;
        }
    }

    result.length = static_cast<Tick>(result.timeline.size());
    result.jobs.reserve(jobs.size());
    for (const auto& j : jobs) result.jobs.push_back(*j);
    return result;
}

} // namespace rtos
