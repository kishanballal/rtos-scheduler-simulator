#pragma once
// CircularQueue.hpp — hand-written ring buffer FIFO, used by Round Robin.
//
// `head_` is the index of the front element; the back slot is
// (head_ + count_) % capacity. Push and pop are O(1) and never shift elements.
// When full, capacity doubles (amortised O(1)). A real RTOS queue (FreeRTOS
// xQueueCreate) uses a fixed capacity and blocks or fails instead; growing is a
// deliberate simplification because the simulator does not know job counts ahead.
//
// T must be default-constructible and movable (Job* is).

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rtos {

template <typename T>
class CircularQueue {
public:
    explicit CircularQueue(std::size_t capacity = 16) : buf_(capacity > 0 ? capacity : 1) {}

    void push(T value) {
        if (count_ == buf_.size()) grow();
        buf_[(head_ + count_) % buf_.size()] = std::move(value);
        ++count_;
    }

    T pop() {
        if (empty()) throw std::out_of_range("CircularQueue::pop on empty queue");
        T value = std::move(buf_[head_]);
        head_ = (head_ + 1) % buf_.size();
        --count_;
        return value;
    }

    const T& front() const {
        if (empty()) throw std::out_of_range("CircularQueue::front on empty queue");
        return buf_[head_];
    }

    bool empty() const noexcept { return count_ == 0; }
    std::size_t size() const noexcept { return count_; }
    std::size_t capacity() const noexcept { return buf_.size(); }

private:
    void grow() {
        std::vector<T> bigger(buf_.size() * 2);
        for (std::size_t i = 0; i < count_; ++i)
            bigger[i] = std::move(buf_[(head_ + i) % buf_.size()]);  // unwrap to index 0
        buf_ = std::move(bigger);
        head_ = 0;
    }

    std::vector<T> buf_;
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

} // namespace rtos
