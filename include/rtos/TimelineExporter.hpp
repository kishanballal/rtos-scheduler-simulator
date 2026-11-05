#pragma once
// TimelineExporter.hpp — writes timelines as CSV for scripts/plot_timeline.py.
//
//   scheduler,tick,task_id,task_name
//   FCFS,0,1,SensorRead
//   FCFS,1,-1,idle

#include <ostream>
#include <string>
#include <vector>

#include "rtos/Simulator.hpp"

namespace rtos {

class TimelineExporter {
public:
    static void writeHeader(std::ostream& out);
    static void writeRows(std::ostream& out, const SimulationResult& result, const Simulator& sim);

    /// Writes every result to `path`. @throws std::runtime_error if it can't open the file.
    static void toCsv(const std::string& path, const std::vector<SimulationResult>& results,
                      const Simulator& sim);
};

} // namespace rtos
