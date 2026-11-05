#pragma once
// Scheduler.hpp — the Strategy interface. The Simulator only ever talks to this
// abstract class, so a new algorithm is a new subclass and zero Simulator edits
// (open/closed principle).

#include <string>

#include "rtos/Job.hpp"
#include "rtos/Types.hpp"

namespace rtos {

class Scheduler {
public:
    virtual ~Scheduler() = default;   // virtual: deleted through unique_ptr<Scheduler>

    virtual std::string name() const = 0;

    /// A job was released and is ready to run.
    virtual void onJobArrival(Job* job) = 0;

    /// Choose the job to run for tick `now`.
    /// @param running the job that ran last tick and is not finished, or nullptr.
    ///        It is NOT in the ready queue. If the scheduler returns a different
    ///        job (preemption), it must put `running` back into its ready queue.
    /// @return the job to run, or nullptr to idle.
    virtual Job* pickNext(Job* running, Tick now) = 0;

    /// The job returned by the last pickNext() has finished its work.
    virtual void onJobComplete(Job* /*job*/) {}

    /// A job's effectivePriority changed (Priority Inheritance). Schedulers
    /// whose order depends on it must re-sort their ready queue.
    virtual void onPriorityChanged(Job* /*job*/) {}

    /// True if any job is waiting in the ready queue.
    virtual bool hasReadyJobs() const = 0;
};

} // namespace rtos
