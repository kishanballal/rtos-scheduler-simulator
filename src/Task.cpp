#include "rtos/Task.hpp"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace rtos {

Task::Task(int id, std::string name, Tick arrival, Tick burst, int priority,
           Tick period, Tick deadline, std::optional<CriticalSection> cs)
    : id_(id), name_(std::move(name)), arrival_(arrival), burst_(burst),
      priority_(priority), period_(period), deadline_(deadline), cs_(std::move(cs)) {
    if (cs_) {
        if (cs_->resource.empty()) throw std::invalid_argument("resource name must not be empty");
        if (cs_->start < 0 || cs_->length <= 0 || cs_->start + cs_->length > burst_)
            throw std::invalid_argument("critical section must lie inside the burst");
    }
    if (id_ < 0)        throw std::invalid_argument("task id must be >= 0");
    if (name_.empty())  throw std::invalid_argument("task name must not be empty");
    if (arrival_ < 0)   throw std::invalid_argument("arrival must be >= 0");
    if (burst_ <= 0)    throw std::invalid_argument("burst must be > 0");
    if (period_ < 0)    throw std::invalid_argument("period must be >= 0");
    if (deadline_ < 0)  throw std::invalid_argument("deadline must be >= 0");
}

Tick Task::relativeDeadline() const noexcept {
    if (deadline_ > 0) return deadline_;
    if (period_ > 0)   return period_;
    return kNoDeadline;
}

double Task::utilization() const noexcept {
    return isPeriodic() ? static_cast<double>(burst_) / static_cast<double>(period_) : 0.0;
}

std::ostream& operator<<(std::ostream& os, const Task& t) {
    os << "T" << t.id() << "(" << t.name() << ", arr=" << t.arrival() << ", C=" << t.burst()
       << ", prio=" << t.priority();
    if (t.isPeriodic()) os << ", T=" << t.period();
    if (t.relativeDeadline() != kNoDeadline) os << ", D=" << t.relativeDeadline();
    if (const auto& cs = t.criticalSection())
        os << ", locks " << cs->resource << " @" << cs->start << " for " << cs->length;
    return os << ")";
}

} // namespace rtos
