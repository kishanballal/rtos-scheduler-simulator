// Tests for Statistics, Schedulability and the Gantt renderer.
#include <gtest/gtest.h>

#include <cmath>

#include "rtos/GanttRenderer.hpp"
#include "rtos/Schedulability.hpp"
#include "rtos/Schedulers.hpp"
#include "rtos/Statistics.hpp"

using namespace rtos;

TEST(Statistics, FcfsNumbersMatchHandCalculation) {
    Simulator sim({Task(1, "T1", 0, 3, 3), Task(2, "T2", 1, 2, 1), Task(3, "T3", 2, 1, 2)});
    sim.setScheduler(makeScheduler("fcfs"));
    const auto r = sim.run(20);
    const auto s = Statistics::compute(r, sim.tasks());

    // T1: 0->3 (wait 0)   T2: 1->5 (wait 2, resp 2)   T3: 2->6 (wait 3, resp 3)
    EXPECT_DOUBLE_EQ(s.perTask[1].avgWaiting, 2.0);
    EXPECT_DOUBLE_EQ(s.perTask[2].avgTurnaround, 4.0);
    EXPECT_DOUBLE_EQ(s.perTask[2].avgResponse, 3.0);
    EXPECT_DOUBLE_EQ(s.overall.avgWaiting, 5.0 / 3.0);
    EXPECT_DOUBLE_EQ(s.overall.cpuUtilization, 1.0);
    EXPECT_DOUBLE_EQ(s.overall.throughput, 3.0 / 6.0);
    EXPECT_EQ(s.overall.contextSwitches, 2);
}

TEST(Statistics, UnfinishedJobPastDeadlineCountsAsMiss) {
    Simulator sim({Task(1, "A", 0, 5, 0, 0, 3)});   // needs 5 ticks, deadline 3
    sim.setScheduler(makeScheduler("edf"));
    const auto r = sim.run(4);                       // stop before it finishes
    EXPECT_EQ(Statistics::compute(r, sim.tasks()).overall.deadlineMisses, 1);
}

TEST(Schedulability, LiuLaylandBoundValues) {
    EXPECT_DOUBLE_EQ(SchedulabilityAnalyzer::liuLaylandBound(1), 1.0);
    EXPECT_NEAR(SchedulabilityAnalyzer::liuLaylandBound(2), 0.8284, 1e-4);
    EXPECT_NEAR(SchedulabilityAnalyzer::liuLaylandBound(1000), std::log(2.0), 1e-3);
}

TEST(Schedulability, RmGuaranteedBelowBound) {
    const auto r = SchedulabilityAnalyzer::analyze({Task(1, "A", 0, 1, 0, 4), Task(2, "B", 0, 2, 0, 6)});
    EXPECT_NEAR(r.utilization, 0.5833, 1e-4);
    EXPECT_TRUE(r.rmGuaranteed);
    EXPECT_TRUE(r.edfFeasible);
}

TEST(Schedulability, AboveBoundOnlyEdfPasses) {
    const auto r = SchedulabilityAnalyzer::analyze({Task(1, "A", 0, 2, 0, 5), Task(2, "B", 0, 4, 0, 7)});
    EXPECT_FALSE(r.rmGuaranteed);
    EXPECT_TRUE(r.edfFeasible);
}

TEST(Schedulability, OverloadFailsBoth) {
    const auto r = SchedulabilityAnalyzer::analyze({Task(1, "A", 0, 3, 0, 4), Task(2, "B", 0, 3, 0, 6)});
    EXPECT_FALSE(r.edfFeasible);
}

TEST(GanttRenderer, MarksRunningTicks) {
    Simulator sim({Task(1, "A", 0, 2, 0), Task(2, "B", 0, 1, 1)});
    sim.setScheduler(makeScheduler("fcfs"));
    const std::string chart = GanttRenderer{}.render(sim.run(10), sim.tasks());
    EXPECT_NE(chart.find("A     ##."), std::string::npos);
    EXPECT_NE(chart.find("B     ..#"), std::string::npos);
}
