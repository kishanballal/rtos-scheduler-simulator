// main.cpp — CLI entry point. Kept thin on purpose: parse arguments, call the
// library, print results. All logic lives in rtos_core so tests can reach it.

#include <algorithm>
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
#include "rtos/TimelineExporter.hpp"
#include "rtos/Version.hpp"

namespace {

struct Options {
    std::string file;
    std::string algo = "all";
    rtos::Tick quantum = 2;
    rtos::Tick horizon = 0;   // 0 = automatic
    bool inheritance = false;
    rtos::Tick jitter = 0;
    unsigned seed = 1;
    std::string exportPath;
};

const char* kUsage =
    "usage: rtos_sim <tasks.csv> [options]\n"
    "  --algo fcfs|rr|priority|rm|edf|all   scheduler (default all = comparison mode)\n"
    "  --quantum N        Round Robin time slice (default 2)\n"
    "  --horizon N        ticks to simulate (default: automatic)\n"
    "  --pi               enable Priority Inheritance on mutexes\n"
    "  --jitter N         sporadic releases: period + random 0..N\n"
    "  --seed S           random seed for --jitter (default 1)\n"
    "  --export FILE.csv  write the timeline(s) for scripts/plot_timeline.py\n";

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
        else if (arg == "--pi") opt.inheritance = true;
        else if (arg == "--jitter") opt.jitter = std::stoll(value());
        else if (arg == "--seed") opt.seed = static_cast<unsigned>(std::stoul(value()));
        else if (arg == "--export") opt.exportPath = value();
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
        sim.setReleaseJitter(opt.jitter, opt.seed);
        const Tick horizon = opt.horizon > 0 ? opt.horizon : sim.defaultHorizon();

        std::cout << "RTOS Scheduler Simulator v" << version() << "\n\n"
                  << formatTaskTable(sim.tasks()) << '\n'
                  << formatSchedulability(SchedulabilityAnalyzer::analyze(sim.tasks())) << '\n'
                  << "Horizon: " << horizon << " ticks"
                  << (opt.jitter > 0 ? " (sporadic releases)" : "") << "\n\n";

        // Each run: (scheduler key, priority inheritance on?)
        std::vector<std::pair<std::string, bool>> runs;
        if (opt.algo == "all") {
            for (const auto& key : allSchedulerKeys()) runs.emplace_back(key, opt.inheritance);
            const bool usesMutexes = std::any_of(sim.tasks().begin(), sim.tasks().end(),
                [](const Task& t) { return t.criticalSection().has_value(); });
            if (usesMutexes && !opt.inheritance) runs.emplace_back("priority", true);
        } else {
            runs.emplace_back(opt.algo, opt.inheritance);
        }

        std::vector<SimulationResult> results;
        std::vector<StatsReport> reports;
        for (const auto& [key, inherit] : runs) {
            sim.setScheduler(makeScheduler(key, opt.quantum));   // runtime swap
            sim.setPriorityInheritance(inherit);
            results.push_back(sim.run(horizon));
            const SimulationResult& result = results.back();
            reports.push_back(Statistics::compute(result, sim.tasks()));
            std::cout << "=== " << result.schedulerName << " ===\n"
                      << GanttRenderer{}.render(result, sim.tasks()) << '\n'
                      << formatStats(reports.back());
            if (result.blockings > 0) std::cout << "mutex blockings " << result.blockings << '\n';
            std::cout << '\n';
        }
        if (reports.size() > 1) std::cout << formatComparison(reports);

        if (!opt.exportPath.empty()) {
            TimelineExporter::toCsv(opt.exportPath, results, sim);
            std::cout << "\nTimeline written to " << opt.exportPath << '\n';
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n' << kUsage;
        return 1;
    }
}
