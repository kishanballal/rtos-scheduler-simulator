// main.cpp — CLI entry point. Kept thin on purpose: parse arguments, call the
// library, print results. All logic lives in rtos_core so tests can reach it.
//
// Usage: rtos_sim <tasks.csv> [--algo fcfs|rr|priority|rm|edf|all]
//                             [--quantum N] [--horizon N]

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "rtos/GanttRenderer.hpp"
#include "rtos/Report.hpp"
#include "rtos/Schedulability.hpp"
#include "rtos/Schedulers.hpp"
#include "rtos/Simulator.hpp"
#include "rtos/Statistics.hpp"
#include "rtos/TaskLoader.hpp"
#include "rtos/Version.hpp"

namespace {

struct Options {
    std::string file;
    std::string algo = "all";
    rtos::Tick quantum = 2;
    rtos::Tick horizon = 0;   // 0 = automatic
};

const char* kUsage =
    "usage: rtos_sim <tasks.csv> [--algo fcfs|rr|priority|rm|edf|all] [--quantum N] [--horizon N]\n";

Options parseArgs(int argc, char* argv[]) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(arg + " needs a value");
            return argv[++i];
        };
        if (arg == "--algo") opt.algo = value();
        else if (arg == "--quantum") opt.quantum = std::stoll(value());
        else if (arg == "--horizon") opt.horizon = std::stoll(value());
        else if (arg == "-h" || arg == "--help") { std::cout << kUsage; std::exit(0); }
        else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option " + arg);
        else opt.file = arg;
    }
    if (opt.file.empty()) throw std::invalid_argument("no task file given");
    return opt;
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace rtos;
    try {
        const Options opt = parseArgs(argc, argv);
        Simulator sim(TaskLoader::fromCsv(opt.file));
        const Tick horizon = opt.horizon > 0 ? opt.horizon : sim.defaultHorizon();

        std::cout << "RTOS Scheduler Simulator v" << version() << "\n\n"
                  << formatTaskTable(sim.tasks()) << '\n'
                  << formatSchedulability(SchedulabilityAnalyzer::analyze(sim.tasks())) << '\n'
                  << "Horizon: " << horizon << " ticks\n\n";

        const std::vector<std::string> keys =
            opt.algo == "all" ? allSchedulerKeys() : std::vector<std::string>{opt.algo};
        std::vector<StatsReport> reports;

        for (const std::string& key : keys) {
            sim.setScheduler(makeScheduler(key, opt.quantum));   // runtime swap
            const SimulationResult result = sim.run(horizon);
            reports.push_back(Statistics::compute(result, sim.tasks()));
            std::cout << "=== " << result.schedulerName << " ===\n"
                      << GanttRenderer{}.render(result, sim.tasks()) << '\n'
                      << formatStats(reports.back()) << '\n';
        }
        if (reports.size() > 1) std::cout << formatComparison(reports);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n' << kUsage;
        return 1;
    }
}
