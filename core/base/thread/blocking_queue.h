// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREAD_BLOCKING_QUEUE_H_
#define CORE_BASE_THREAD_BLOCKING_QUEUE_H_

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace lynx {
namespace base {

template <typename T>
class BlockingQueue {
  std::mutex mutex_;
  std::condition_variable not_full_;
  std::condition_variable not_empty_;
  size_t start_;
  size_t end_;
  size_t capacity_;
  std::vector<T> vt_;

 public:
  BlockingQueue(const BlockingQueue<T>& other) = delete;
  BlockingQueue<T>& operator=(const BlockingQueue<T>& other) = delete;
  explicit BlockingQueue(size_t capacity)
      : start_(0), end_(0), capacity_(capacity), vt_(capacity + 1) {}

  bool IsEmpty() { return end_ == start_; }

  bool IsFull() { return (start_ + capacity_ - end_) % (capacity_ + 1) == 0; }

  /**
   * Add a new object to the queue.
   * Blocks execution while queue is full.
   */
  void Push(const T& e) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (IsFull()) {
      not_full_.wait(lock);
    }

    vt_[end_++] = e;
    end_ %= (capacity_ + 1);
    not_empty_.notify_one();
  }

  /**
   * Retrieve and remove the oldest object.
   * Blocks execution while queue is empty.
   */
  T Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (IsEmpty()) {
      not_empty_.wait(lock);
    }

    auto res = vt_[start_++];
    start_ %= (capacity_ + 1);
    not_full_.notify_one();
    return res;
  }
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREAD_BLOCKING_QUEUE_H_
