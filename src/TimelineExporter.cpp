#include "rtos/TimelineExporter.hpp"

#include <fstream>
#include <stdexcept>

namespace rtos {

void TimelineExporter::writeHeader(std::ostream& out) {
    out << "scheduler,tick,task_id,task_name\n";
}

void TimelineExporter::writeRows(std::ostream& out, const SimulationResult& result,
                                 const Simulator& sim) {
    for (std::size_t tick = 0; tick < result.timeline.size(); ++tick) {
        const int id = result.timeline[tick];
        const Task* task = sim.findTask(id);   // O(1) hash-map lookup
        out << result.schedulerName << ',' << tick << ',' << id << ','
            << (task != nullptr ? task->name() : "idle") << '\n';
    }
}

void TimelineExporter::toCsv(const std::string& path, const std::vector<SimulationResult>& results,
                             const Simulator& sim) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write '" + path + "'");
    writeHeader(out);
    for (const auto& r : results) writeRows(out, r, sim);
}

} // namespace rtos
