#pragma once
// GanttRenderer.hpp — draws a timeline as ASCII, one row per task:
//
//   time        0    5    10
//               |----|----|-
//   SensorRead  #...#...#...
//   Motor       .##...##....
//
// '#' = running in that tick, '.' = not running.

#include <cstddef>
#include <string>
#include <vector>

#include "rtos/Simulator.hpp"

namespace rtos {

class GanttRenderer {
public:
    explicit GanttRenderer(std::size_t maxWidth = 100) : maxWidth_(maxWidth) {}
    std::string render(const SimulationResult& result, const std::vector<Task>& tasks) const;

private:
    std::size_t maxWidth_;
};

} // namespace rtos
