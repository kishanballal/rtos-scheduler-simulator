#include "rtos/Schedulers.hpp"

#include <stdexcept>

namespace rtos {

// FCFS: whoever holds the CPU keeps it until done (non-preemptive).
Job* FcfsScheduler::pickNext(Job* running, Tick /*now*/) {
    if (running != nullptr) return running;
    if (ready_.empty()) return nullptr;
    Job* next = ready_.front();
    ready_.pop();
    return next;
}

RoundRobinScheduler::RoundRobinScheduler(Tick quantum) : quantum_(quantum) {
    if (quantum_ <= 0) throw std::invalid_argument("Round Robin quantum must be > 0");
}

std::string RoundRobinScheduler::name() const { return "RR(q=" + std::to_string(quantum_) + ")"; }

// Jobs released this tick are already queued (the simulator handles arrivals
// first), so a job whose slice expires goes behind them. Same as Linux SCHED_RR.
Job* RoundRobinScheduler::pickNext(Job* running, Tick /*now*/) {
    if (running != nullptr && used_ < quantum_) {   // slice not used up yet
        ++used_;
        return running;
    }
    if (running != nullptr) {                       // slice expired
        if (ready_.empty()) {                       // nobody waiting: keep going
            used_ = 1;
            return running;
        }
        ready_.push(running);                       // go to the back of the line
    }
    if (ready_.empty()) {
        used_ = 0;
        return nullptr;
    }
    used_ = 1;
    return ready_.pop();
}

void RoundRobinScheduler::onJobComplete(Job* /*job*/) { used_ = 0; }

std::unique_ptr<Scheduler> makeScheduler(const std::string& key, Tick quantum) {
    if (key == "fcfs")     return std::make_unique<FcfsScheduler>();
    if (key == "rr")       return std::make_unique<RoundRobinScheduler>(quantum);
    if (key == "priority") return std::make_unique<PriorityScheduler>();
    if (key == "rm")       return std::make_unique<RateMonotonicScheduler>();
    if (key == "edf")      return std::make_unique<EdfScheduler>();
    throw std::invalid_argument("unknown scheduler '" + key + "' (use fcfs, rr, priority, rm, edf, all)");
}

std::vector<std::string> allSchedulerKeys() { return {"fcfs", "rr", "priority", "rm", "edf"}; }

} // namespace rtos
