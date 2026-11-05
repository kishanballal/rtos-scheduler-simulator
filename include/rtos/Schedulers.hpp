#pragma once
// Schedulers.hpp — the five concrete strategies.
//
//   Scheduler (abstract)
//   ├── FcfsScheduler            std::queue           non-preemptive
//   ├── RoundRobinScheduler      CircularQueue (ours) time-sliced
//   └── HeapScheduler<Compare>   MinHeap (ours)       preemptive, template
//       ├── PriorityScheduler       key = static priority
//       ├── RateMonotonicScheduler  key = period (shorter = more urgent)
//       └── EdfScheduler            key = absolute deadline
//
// Every comparator breaks ties by release time, then task id, so the
// simulation is deterministic (same input, same timeline).

#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

#include "rtos/CircularQueue.hpp"
#include "rtos/MinHeap.hpp"
#include "rtos/Scheduler.hpp"

namespace rtos {

// ---- FCFS ------------------------------------------------------------------
class FcfsScheduler final : public Scheduler {
public:
    std::string name() const override { return "FCFS"; }
    void onJobArrival(Job* job) override { ready_.push(job); }
    Job* pickNext(Job* running, Tick now) override;
    bool hasReadyJobs() const override { return !ready_.empty(); }

private:
    std::queue<Job*> ready_;
};

// ---- Round Robin -----------------------------------------------------------
class RoundRobinScheduler final : public Scheduler {
public:
    explicit RoundRobinScheduler(Tick quantum);
    std::string name() const override;
    void onJobArrival(Job* job) override { ready_.push(job); }
    Job* pickNext(Job* running, Tick now) override;
    void onJobComplete(Job* job) override;
    bool hasReadyJobs() const override { return !ready_.empty(); }

private:
    CircularQueue<Job*> ready_;
    Tick quantum_;
    Tick used_ = 0;   // ticks the current job has used of its slice
};

// ---- Heap-based preemptive schedulers --------------------------------------
/// Shared logic for every "most urgent job wins, preempt if something more
/// urgent arrives" policy. Only the ordering differs, so it is a template
/// parameter instead of copy-pasted code.
template <typename Compare>
class HeapScheduler : public Scheduler {
public:
    void onJobArrival(Job* job) override { ready_.push(job); }

    Job* pickNext(Job* running, Tick /*now*/) override {
        if (ready_.empty()) return running;
        if (running == nullptr) return ready_.pop();
        if (Compare{}(ready_.top(), running)) {   // strictly more urgent: preempt
            Job* next = ready_.pop();
            ready_.push(running);
            return next;
        }
        return running;
    }

    bool hasReadyJobs() const override { return !ready_.empty(); }

    void onPriorityChanged(Job* /*job*/) override { ready_.rebuild(); }

private:
    MinHeap<Job*, Compare> ready_;
};

struct ByPriority {   // uses the EFFECTIVE priority so Priority Inheritance works
    bool operator()(const Job* a, const Job* b) const {
        return std::make_tuple(a->effectivePriority, a->release, a->task->id()) <
               std::make_tuple(b->effectivePriority, b->release, b->task->id());
    }
};

struct ByPeriod {   // Rate Monotonic: shorter period = higher priority
    static Tick key(const Job* j) { return j->task->isPeriodic() ? j->task->period() : kNoDeadline; }
    bool operator()(const Job* a, const Job* b) const {
        return std::make_tuple(key(a), a->release, a->task->id()) <
               std::make_tuple(key(b), b->release, b->task->id());
    }
};

struct ByDeadline {  // EDF: earliest absolute deadline first
    bool operator()(const Job* a, const Job* b) const {
        return std::make_tuple(a->absDeadline, a->release, a->task->id()) <
               std::make_tuple(b->absDeadline, b->release, b->task->id());
    }
};

class PriorityScheduler final : public HeapScheduler<ByPriority> {
public:
    std::string name() const override { return "Priority"; }
};

class RateMonotonicScheduler final : public HeapScheduler<ByPeriod> {
public:
    std::string name() const override { return "RM"; }
};

class EdfScheduler final : public HeapScheduler<ByDeadline> {
public:
    std::string name() const override { return "EDF"; }
};

// ---- Factory ---------------------------------------------------------------
/// Builds a scheduler from a CLI key: fcfs, rr, priority, rm, edf.
/// @throws std::invalid_argument for an unknown key.
std::unique_ptr<Scheduler> makeScheduler(const std::string& key, Tick quantum = 2);

/// Every key accepted by makeScheduler, in comparison-table order.
std::vector<std::string> allSchedulerKeys();

} // namespace rtos
