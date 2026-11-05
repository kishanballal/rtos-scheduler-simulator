// Tests for the stretch goals: mutexes + Priority Inheritance, sporadic
// releases, heap re-keying and timeline export.
#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "rtos/MinHeap.hpp"
#include "rtos/Schedulers.hpp"
#include "rtos/Statistics.hpp"
#include "rtos/TaskLoader.hpp"
#include "rtos/TimelineExporter.hpp"

using namespace rtos;

namespace {

// Low (prio 3) locks R1 after 1 tick for 3 ticks. High (prio 1) needs R1
// immediately. Medium (prio 2) never touches R1 but is CPU-hungry.
std::vector<Task> inversionSet() {
    return {Task(1, "Low", 0, 5, 3, 0, 0, CriticalSection{"R1", 1, 3}),
            Task(2, "High", 2, 2, 1, 0, 6, CriticalSection{"R1", 0, 1}),
            Task(3, "Medium", 3, 4, 2)};
}

Tick finishOf(const SimulationResult& r, int taskId) {
    for (const Job& j : r.jobs)
        if (j.task->id() == taskId) return j.finish.value_or(-1);
    return -1;
}

} // namespace

TEST(PriorityInversion, WithoutInheritanceMediumDelaysHigh) {
    Simulator sim(inversionSet());
    sim.setScheduler(makeScheduler("priority"));
    const auto r = sim.run(50);
    // High blocks at t=2; Medium then runs t=3..6 while Low still holds R1.
    EXPECT_EQ(r.timeline, (std::vector<int>{1, 1, 1, 3, 3, 3, 3, 1, 2, 2, 1}));
    EXPECT_EQ(finishOf(r, 2), 10);
    EXPECT_EQ(r.blockings, 1);
    EXPECT_EQ(Statistics::compute(r, sim.tasks()).perTask[1].deadlineMisses, 1);
}

TEST(PriorityInversion, InheritanceLetsLowFinishCriticalSectionFirst) {
    Simulator sim(inversionSet());
    sim.setScheduler(makeScheduler("priority"));
    sim.setPriorityInheritance(true);
    const auto r = sim.run(50);
    // Low inherits priority 1 at t=2, so Medium can't preempt it at t=3.
    EXPECT_EQ(r.timeline, (std::vector<int>{1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 1}));
    EXPECT_EQ(finishOf(r, 2), 6);
    EXPECT_EQ(r.schedulerName, "Priority+PI");
    EXPECT_EQ(Statistics::compute(r, sim.tasks()).perTask[1].deadlineMisses, 0);
}

TEST(Task, CriticalSectionMustFitInBurst) {
    EXPECT_THROW(Task(1, "A", 0, 3, 1, 0, 0, CriticalSection{"R", 2, 2}), std::invalid_argument);
    EXPECT_THROW(Task(1, "A", 0, 3, 1, 0, 0, CriticalSection{"", 0, 1}), std::invalid_argument);
}

TEST(TaskLoader, ParsesOptionalResourceColumn) {
    std::istringstream csv("1,Low,0,5,3,0,0,R1:1:3\n2,Plain,0,1,1,0,0\n");
    const auto tasks = TaskLoader::fromStream(csv);
    ASSERT_TRUE(tasks[0].criticalSection().has_value());
    EXPECT_EQ(tasks[0].criticalSection()->resource, "R1");
    EXPECT_EQ(tasks[0].criticalSection()->length, 3);
    EXPECT_FALSE(tasks[1].criticalSection().has_value());

    std::istringstream bad("1,Low,0,5,3,0,0,R1-1-3\n");
    EXPECT_THROW(TaskLoader::fromStream(bad), std::runtime_error);
}

TEST(MinHeap, RebuildRestoresOrderAfterKeysChange) {
    std::vector<int> keys{5, 3, 8, 1};
    auto byKey = [&keys](int a, int b) { return keys[static_cast<std::size_t>(a)] <
                                                keys[static_cast<std::size_t>(b)]; };
    MinHeap<int, decltype(byKey)> h(byKey);
    for (int i = 0; i < 4; ++i) h.push(i);
    EXPECT_EQ(h.top(), 3);   // key 1
    keys[2] = 0;             // index 2 becomes most urgent
    h.rebuild();
    EXPECT_EQ(h.pop(), 2);
    EXPECT_EQ(h.pop(), 3);
}

TEST(Sporadic, InterArrivalIsAtLeastThePeriodAndReproducible) {
    auto releases = [](unsigned seed) {
        Simulator sim({Task(1, "S", 0, 1, 0, 4)});
        sim.setScheduler(makeScheduler("edf"));
        sim.setReleaseJitter(3, seed);
        std::vector<Tick> out;
        for (const Job& j : sim.run(200).jobs) out.push_back(j.release);
        return out;
    };
    const auto a = releases(7);
    ASSERT_GT(a.size(), 10u);
    for (std::size_t i = 1; i < a.size(); ++i) {
        EXPECT_GE(a[i] - a[i - 1], 4);
        EXPECT_LE(a[i] - a[i - 1], 7);
    }
    EXPECT_EQ(a, releases(7));   // same seed, same releases
}

TEST(TimelineExporter, WritesOneRowPerTick) {
    Simulator sim({Task(1, "A", 1, 1, 0)});
    sim.setScheduler(makeScheduler("fcfs"));
    std::ostringstream out;
    TimelineExporter::writeHeader(out);
    TimelineExporter::writeRows(out, sim.run(10), sim);
    EXPECT_EQ(out.str(), "scheduler,tick,task_id,task_name\nFCFS,0,-1,idle\nFCFS,1,1,A\n");
}
