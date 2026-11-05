#include "rtos/Task.hpp"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace rtos {

Task::Task(int id, std::string name, Tick arrival, Tick burst, int priority,
           Tick period, Tick deadline)
    : id_(id), name_(std::move(name)), arrival_(arrival), burst_(burst),
      priority_(priority), period_(period), deadline_(deadline) {
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
    return os << ")";
}

} // namespace rtos
