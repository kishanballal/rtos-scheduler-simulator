#pragma once
// MinHeap.hpp — hand-written, array-backed binary min-heap.
//
// Layout: a complete binary tree stored in a vector. For node i:
//   parent = (i - 1) / 2,  left = 2i + 1,  right = 2i + 2.
// Invariant: no child "comes before" its parent according to Compare.
//
//   push  O(log n)  append at the end, sift up
//   pop   O(log n)  move last element to the root, sift down
//   top   O(1)
//
// Compare(a, b) returns true when `a` must come out before `b`, exactly like
// std::less. Swapping the comparator turns one heap into a priority queue,
// an RM queue or an EDF queue (see Schedulers.hpp).

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rtos {

template <typename T, typename Compare = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Compare cmp = Compare{}) : cmp_(std::move(cmp)) {}

    void push(T value) {
        data_.push_back(std::move(value));
        siftUp(data_.size() - 1);
    }

    const T& top() const {
        if (data_.empty()) throw std::out_of_range("MinHeap::top on empty heap");
        return data_.front();
    }

    T pop() {
        if (data_.empty()) throw std::out_of_range("MinHeap::pop on empty heap");
        T result = std::move(data_.front());
        if (data_.size() > 1) {
            data_.front() = std::move(data_.back());
            data_.pop_back();
            siftDown(0);
        } else {
            data_.pop_back();
        }
        return result;
    }

    bool empty() const noexcept { return data_.empty(); }
    std::size_t size() const noexcept { return data_.size(); }

private:
    void siftUp(std::size_t i) {
        while (i > 0) {
            const std::size_t parent = (i - 1) / 2;
            if (!cmp_(data_[i], data_[parent])) break;
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }

    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        for (;;) {
            const std::size_t left = 2 * i + 1;
            const std::size_t right = left + 1;
            std::size_t best = i;
            if (left < n && cmp_(data_[left], data_[best])) best = left;
            if (right < n && cmp_(data_[right], data_[best])) best = right;
            if (best == i) return;
            std::swap(data_[i], data_[best]);
            i = best;
        }
    }

    std::vector<T> data_;
    Compare cmp_;
};

} // namespace rtos
