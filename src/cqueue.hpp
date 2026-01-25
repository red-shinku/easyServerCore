#include <atomic>
#include <cstddef>
#include <vector>
#include <optional>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace easysv
{

template<typename T>
class queue {
public:
    explicit queue(size_t capacity)
        : buf_(capacity), capacity_(capacity), head_(0), tail_(0), count_(0)
    {
        if (capacity_ == 0) throw std::invalid_argument("capacity must be > 0");
    }

    // 返回当前元素个数
    size_t size() const noexcept { return count_; }

    // 是否为空
    bool empty() const noexcept { return count_ == 0; }

    // 是否已满
    bool full() const noexcept { return count_ == capacity_; }

    // 返回队头元素的引用（队空时抛出）
    T& front() {
        if (empty()) throw std::runtime_error("front() called on empty queue");
        return *buf_[head_];
    }
    const T& front() const {
        if (empty()) throw std::runtime_error("front() called on empty queue");
        return *buf_[head_];
    }

    // 将元素推入队尾；队满返回 false
    bool push(const T& value) {
        if (full()) return false;
        buf_[tail_].emplace(value);
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
        return true;
    }
    bool push(T&& value) {
        if (full()) return false;
        buf_[tail_].emplace(std::move(value));
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
        return true;
    }

    // 删除队头元素；队空返回 false
    bool pop() {
        if (empty()) return false;
        buf_[head_].reset(); // 销毁存放的对象
        head_ = (head_ + 1) % capacity_;
        --count_;
        return true;
    }

    // 清空队列
    void clear() noexcept {
        if (count_ == 0) return;
        // 重置所有已占用位置（简单遍历）
        for (size_t i = 0, idx = head_; i < count_; ++i, idx = (idx + 1) % capacity_) {
            buf_[idx].reset();
        }
        head_ = tail_ = count_ = 0;
    }

    // 返回容量
    size_t capacity() const noexcept { return capacity_; }

private:
    std::vector<std::optional<T>> buf_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    size_t count_;
};

}