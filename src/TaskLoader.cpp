#include "rtos/TaskLoader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace rtos {

namespace {

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, sep)) fields.push_back(trim(field));
    return fields;
}

Tick toTick(const std::string& s) {
    std::size_t pos = 0;
    const long long v = std::stoll(s, &pos);   // throws on non-numeric input
    if (pos != s.size()) throw std::invalid_argument("not a whole number: '" + s + "'");
    return static_cast<Tick>(v);
}

int toInt(const std::string& s) {
    const Tick v = toTick(s);
    if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max())
        throw std::out_of_range("value out of int range: '" + s + "'");
    return static_cast<int>(v);
}

} // namespace

std::vector<Task> TaskLoader::fromStream(std::istream& in) {
    std::vector<Task> tasks;
    std::string raw;
    std::size_t lineNo = 0;
    bool seenFirstRow = false;

    while (std::getline(in, raw)) {
        ++lineNo;
        const std::string line = trim(raw);
        if (line.empty() || line.front() == '#') continue;

        // The first row is a header if it doesn't start with a digit.
        if (!seenFirstRow) {
            seenFirstRow = true;
            if (!std::isdigit(static_cast<unsigned char>(line.front()))) continue;
        }

        const auto f = split(line, ',');
        const std::string where = "line " + std::to_string(lineNo) + ": ";
        if (f.size() != 7)
            throw std::runtime_error(where + "expected 7 fields, got " + std::to_string(f.size()));
        try {
            tasks.emplace_back(toInt(f[0]), f[1], toTick(f[2]), toTick(f[3]), toInt(f[4]),
                               toTick(f[5]), toTick(f[6]));
        } catch (const std::exception& e) {
            throw std::runtime_error(where + e.what());
        }
    }

    // Stable sort by arrival: O(n log n), keeps file order for equal arrivals.
    std::stable_sort(tasks.begin(), tasks.end(),
                     [](const Task& a, const Task& b) { return a.arrival() < b.arrival(); });
    return tasks;
}

std::vector<Task> TaskLoader::fromCsv(const std::string& path) {
    std::ifstream file(path);   // RAII: closed automatically when it goes out of scope
    if (!file) throw std::runtime_error("cannot open '" + path + "'");
    return fromStream(file);
}

} // namespace rtos
