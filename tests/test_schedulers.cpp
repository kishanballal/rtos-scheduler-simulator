// Each scheduler is checked against a timeline worked out by hand.
// Timelines list the task id that ran in each tick (-1 = idle).
#include <gtest/gtest.h>

#include <vector>

#include "rtos/Schedulers.hpp"
#include "rtos/Simulator.hpp"
#include "rtos/Statistics.hpp"

using namespace rtos;

namespace {

// T1 arrives 0 (C=3, prio 3), T2 arrives 1 (C=2, prio 1), T3 arrives 2 (C=1, prio 2).
std::vector<Task> aperiodicSet() {
    return {Task(1, "T1", 0, 3, 3), Task(2, "T2", 1, 2, 1), Task(3, "T3", 2, 1, 2)};
}

// U = 2/5 + 4/7 = 0.971: above the RM bound (0.828) but below 1.
std::vector<Task> rmFailsEdfPasses() {
    return {Task(1, "A", 0, 2, 0, 5), Task(2, "B", 0, 4, 0, 7)};
}

SimulationResult run(std::vector<Task> tasks, const std::string& algo, Tick horizon,
                     Tick quantum = 2) {
    static std::vector<Simulator> keepAlive;   // results point into the simulator's tasks
    keepAlive.reserve(64);
    keepAlive.emplace_back(std::move(tasks));
    keepAlive.back().setScheduler(makeScheduler(algo, quantum));
    return keepAlive.back().run(horizon);
}

} // namespace

TEST(Fcfs, RunsInArrivalOrderWithoutPreemption) {
    const auto r = run(aperiodicSet(), "fcfs", 20);
    EXPECT_EQ(r.timeline, (std::vector<int>{1, 1, 1, 2, 2, 3}));
    EXPECT_EQ(r.contextSwitches, 2);
}

TEST(RoundRobin, RotatesEveryQuantum) {
    const auto r = run(aperiodicSet(), "rr", 20, 2);
    EXPECT_EQ(r.timeline, (std::vector<int>{1, 1, 2, 2, 3, 1}));
}

TEST(RoundRobin, RejectsZeroQuantum) {
    EXPECT_THROW(RoundRobinScheduler(0), std::invalid_argument);
}

TEST(Priority, PreemptsForMoreUrgentArrival) {
    const auto r = run(aperiodicSet(), "priority", 20);
    EXPECT_EQ(r.timeline, (std::vector<int>{1, 2, 2, 3, 1, 1}));
    EXPECT_EQ(r.contextSwitches, 3);
}

TEST(RateMonotonic, MissesDeadlineAboveBound) {
    const auto tasks = rmFailsEdfPasses();
    const auto r = run(tasks, "rm", 35);
    // A (period 5) always wins; B's first job needs tick 7 but its deadline is 7.
    EXPECT_EQ(std::vector<int>(r.timeline.begin(), r.timeline.begin() + 8),
              (std::vector<int>{1, 1, 2, 2, 2, 1, 1, 2}));
    EXPECT_GT(Statistics::compute(r, tasks).overall.deadlineMisses, 0);
}

TEST(Edf, MeetsAllDeadlinesWhenUtilizationBelowOne) {
    const auto tasks = rmFailsEdfPasses();
    const auto r = run(tasks, "edf", 35);
    EXPECT_EQ(Statistics::compute(r, tasks).overall.deadlineMisses, 0);
}

TEST(Simulator, PeriodicTasksReleaseEveryPeriod) {
    const auto r = run({Task(1, "A", 0, 1, 0, 4)}, "edf", 12);
    EXPECT_EQ(r.jobs.size(), 3u);   // released at 0, 4, 8
    EXPECT_EQ(r.timeline, (std::vector<int>{1, -1, -1, -1, 1, -1, -1, -1, 1, -1, -1, -1}));
}

TEST(Simulator, IdlesUntilFirstArrivalAndStopsWhenDone) {
    const auto r = run({Task(1, "A", 2, 1, 0)}, "fcfs", 100);
    EXPECT_EQ(r.timeline, (std::vector<int>{-1, -1, 1}));
}

TEST(Simulator, RejectsDuplicateIdsAndMissingScheduler) {
    EXPECT_THROW(Simulator({Task(1, "A", 0, 1, 0), Task(1, "B", 0, 1, 0)}), std::invalid_argument);
    Simulator sim({Task(1, "A", 0, 1, 0)});
    EXPECT_THROW(sim.run(10), std::logic_error);
}

TEST(Simulator, SchedulerCanBeSwappedAtRuntime) {
    Simulator sim(aperiodicSet());
    sim.setScheduler(makeScheduler("fcfs"));
    const auto first = sim.run(20).timeline;
    sim.setScheduler(makeScheduler("priority"));
    EXPECT_NE(sim.run(20).timeline, first);
    EXPECT_NE(sim.findTask(2), nullptr);
    EXPECT_EQ(sim.findTask(99), nullptr);
}

TEST(Factory, UnknownKeyThrows) {
    EXPECT_THROW(makeScheduler("lottery"), std::invalid_argument);
}
