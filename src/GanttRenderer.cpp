#include "rtos/GanttRenderer.hpp"

#include <algorithm>
#include <sstream>

namespace rtos {

std::string GanttRenderer::render(const SimulationResult& result,
                                  const std::vector<Task>& tasks) const {
    const std::size_t width = std::min(result.timeline.size(), maxWidth_);
    std::size_t label = 4;
    for (const Task& t : tasks) label = std::max(label, t.name().size());
    label += 2;

    // Axis: a number every 5 ticks, if it fits before the next label.
    std::string axis(width, ' ');
    for (std::size_t i = 0; i < width; i += 5) {
        const std::string n = std::to_string(i);
        for (std::size_t k = 0; k < n.size() && i + k < width; ++k) axis[i + k] = n[k];
    }
    std::string ruler(width, '-');
    for (std::size_t i = 0; i < width; i += 5) ruler[i] = '|';

    std::ostringstream os;
    os << std::string("time").append(label - 4, ' ') << axis << '\n'
       << std::string(label, ' ') << ruler << '\n';

    for (const Task& t : tasks) {
        std::string row(width, '.');
        for (std::size_t i = 0; i < width; ++i)
            if (result.timeline[i] == t.id()) row[i] = '#';
        os << t.name() << std::string(label - t.name().size(), ' ') << row << '\n';
    }

    std::string idle(width, ' ');
    for (std::size_t i = 0; i < width; ++i)
        if (result.timeline[i] == kIdle) idle[i] = '_';
    os << "idle" << std::string(label - 4, ' ') << idle << '\n';

    if (width < result.timeline.size())
        os << "(showing first " << width << " of " << result.timeline.size() << " ticks)\n";
    return os.str();
}

} // namespace rtos
