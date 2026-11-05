// Tests for the hand-written MinHeap and CircularQueue.
#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#include "rtos/CircularQueue.hpp"
#include "rtos/MinHeap.hpp"

using rtos::CircularQueue;
using rtos::MinHeap;

TEST(MinHeap, PopsInSortedOrder) {
    MinHeap<int> h;
    for (int v : {5, 3, 8, 1, 9, 2, 7}) h.push(v);
    std::vector<int> out;
    while (!h.empty()) out.push_back(h.pop());
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3, 5, 7, 8, 9}));
}

TEST(MinHeap, CustomComparatorMakesMaxHeap) {
    MinHeap<int, std::greater<int>> h;
    for (int v : {4, 10, 1}) h.push(v);
    EXPECT_EQ(h.top(), 10);
}

TEST(MinHeap, MatchesStdSortOnRandomInput) {
    std::mt19937 rng(42);
    std::vector<int> values(500);
    for (int& v : values) v = static_cast<int>(rng() % 1000);
    MinHeap<int> h;
    for (int v : values) h.push(v);
    std::sort(values.begin(), values.end());
    for (int expected : values) EXPECT_EQ(h.pop(), expected);
}

TEST(MinHeap, EmptyAccessThrows) {
    MinHeap<int> h;
    EXPECT_THROW(h.top(), std::out_of_range);
    EXPECT_THROW(h.pop(), std::out_of_range);
}

TEST(CircularQueue, FifoOrderAcrossWrapAround) {
    CircularQueue<int> q(3);
    q.push(1); q.push(2); q.push(3);
    EXPECT_EQ(q.pop(), 1);
    q.push(4);                       // wraps to index 0
    EXPECT_EQ(q.capacity(), 3u);     // no growth needed
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_EQ(q.pop(), 4);
    EXPECT_TRUE(q.empty());
}

TEST(CircularQueue, GrowsWhenFullAndKeepsOrder) {
    CircularQueue<int> q(2);
    q.push(1); q.push(2);
    EXPECT_EQ(q.pop(), 1);
    q.push(3); q.push(4);            // full while wrapped -> grow must unwrap
    EXPECT_GE(q.capacity(), 3u);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_EQ(q.pop(), 4);
    EXPECT_THROW(q.pop(), std::out_of_range);
}
