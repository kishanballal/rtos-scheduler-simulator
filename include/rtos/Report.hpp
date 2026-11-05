#pragma once
// Report.hpp — text formatting for the CLI. Computing numbers (Statistics) and
// printing them are separate jobs, so changing the table layout never risks
// breaking the maths.

#include <string>
#include <vector>

#include "rtos/Schedulability.hpp"
#include "rtos/Statistics.hpp"

namespace rtos {

std::string formatTaskTable(const std::vector<Task>& tasks);
std::string formatSchedulability(const SchedulabilityReport& report);
std::string formatStats(const StatsReport& report);
std::string formatComparison(const std::vector<StatsReport>& reports);

} // namespace rtos
