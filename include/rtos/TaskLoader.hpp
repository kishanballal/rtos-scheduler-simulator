#pragma once
// TaskLoader.hpp — turns CSV text into Tasks. Kept separate from the engine so
// adding JSON later touches only this class (single responsibility).
//
// Format (header line optional, '#' starts a comment line):
//   id,name,arrival,burst,priority,period,deadline[,resource:start:length]

#include <istream>
#include <string>
#include <vector>

#include "rtos/Task.hpp"

namespace rtos {

class TaskLoader {
public:
    /// @throws std::runtime_error with the line number on any bad row.
    static std::vector<Task> fromStream(std::istream& in);

    /// @throws std::runtime_error if the file cannot be opened or parsed.
    static std::vector<Task> fromCsv(const std::string& path);
};

} // namespace rtos
