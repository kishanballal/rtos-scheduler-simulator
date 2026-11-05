// Tests for Task validation and CSV loading.
#include <gtest/gtest.h>

#include <sstream>

#include "rtos/TaskLoader.hpp"

using namespace rtos;

TEST(Task, DeadlineDefaultsToPeriod) {
    Task periodic(1, "A", 0, 1, 1, 10, 0);
    EXPECT_EQ(periodic.relativeDeadline(), 10);
    EXPECT_DOUBLE_EQ(periodic.utilization(), 0.1);

    Task aperiodic(2, "B", 0, 1, 1);
    EXPECT_EQ(aperiodic.relativeDeadline(), kNoDeadline);
    EXPECT_DOUBLE_EQ(aperiodic.utilization(), 0.0);
}

TEST(Task, RejectsInvalidFields) {
    EXPECT_THROW(Task(1, "A", 0, 0, 1), std::invalid_argument);    // burst 0
    EXPECT_THROW(Task(1, "A", -1, 1, 1), std::invalid_argument);   // negative arrival
    EXPECT_THROW(Task(1, "", 0, 1, 1), std::invalid_argument);     // empty name
}

TEST(TaskLoader, ParsesHeaderCommentsAndSortsByArrival) {
    std::istringstream csv(
        "id,name,arrival,burst,priority,period,deadline\n"
        "# a comment\n"
        "2, Late , 5, 1, 1, 0, 0\n"
        "\n"
        "1,Early,0,2,3,10,8\n");
    const auto tasks = TaskLoader::fromStream(csv);
    ASSERT_EQ(tasks.size(), 2u);
    EXPECT_EQ(tasks[0].name(), "Early");
    EXPECT_EQ(tasks[0].relativeDeadline(), 8);
    EXPECT_EQ(tasks[1].name(), "Late");   // whitespace trimmed
}

TEST(TaskLoader, ReportsLineNumberOnBadRow) {
    std::istringstream csv("id,name,arrival,burst,priority,period,deadline\n1,A,0,abc,1,0,0\n");
    try {
        TaskLoader::fromStream(csv);
        FAIL() << "expected an exception";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("line 2"), std::string::npos);
    }
}

TEST(TaskLoader, RejectsWrongFieldCount) {
    std::istringstream csv("1,A,0,1\n");
    EXPECT_THROW(TaskLoader::fromStream(csv), std::runtime_error);
}

TEST(TaskLoader, LoadsSampleFile) {
    const auto tasks = TaskLoader::fromCsv(std::string(RTOS_DATA_DIR) + "/sample_tasks.csv");
    EXPECT_EQ(tasks.size(), 4u);
}

TEST(TaskLoader, MissingFileThrows) {
    EXPECT_THROW(TaskLoader::fromCsv("does/not/exist.csv"), std::runtime_error);
}
