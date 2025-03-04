// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_CONCURRENT_QUEUE_H_
#define BASE_INCLUDE_CONCURRENT_QUEUE_H_

#include <atomic>
#include <iterator>
#include <utility>

namespace lynx {
namespace base {

/*
  Thread safe lock free queue. Not provide node access like
  Front/Back/Top since they may break thread safe. Not provide Pop single
  node since it needs more code and we dont need it now. Last one is appended
  to the end of internal list. PopAll will return in pushed order.
*/
template <typename T>
class ConcurrentQueue {
 public:
  struct Node {
    T data;
    Node* next;
    explicit Node(T data) : data(std::move(data)), next(nullptr) {}
  };

  struct Iterator {
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::forward_iterator_tag;

    Node* ptr;
    Iterator() : ptr(nullptr) {}
    explicit Iterator(Node* ptr) : ptr(ptr) {}

    T& operator*() const { return ptr->data; }

    T* operator->() const { return &ptr->data; }

    Iterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator t(*this);
      ++(*this);
      return t;
    }

    friend bool operator==(const Iterator& x, const Iterator& y) {
      return x.ptr == y.ptr;
    }

    friend bool operator!=(const Iterator& x, const Iterator& y) {
      return !(x == y);
    }
  };

  struct IterableContainer {
    IterableContainer() : head_(nullptr) {}
    explicit IterableContainer(Node* head, bool reverse_order) {
      if (reverse_order) {
        head_ = head;
        return;
      }

      // Reverse the single linked list.
      Node* prev = nullptr;
      Node* curr = head;
      while (curr != nullptr) {
        Node* next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
      }
      head_ = prev;
    }

    IterableContainer(const IterableContainer&) = delete;
    IterableContainer& operator=(const IterableContainer&) = delete;
    IterableContainer(IterableContainer&& other) : head_(other.head_) {
      other.head_ = nullptr;
    }
    IterableContainer& operator=(IterableContainer&& other) {
      if (this != &other) {
        FreeList();
        head_ = other.head_;
        other.head_ = nullptr;
      }
      return *this;
    }

    ~IterableContainer() { FreeList(); }

    bool empty() const { return head_ == nullptr; }

    Iterator begin() { return Iterator(head_); }

    Iterator end() { return Iterator(); }

    Iterator begin() const { return Iterator(head_); }

    Iterator end() const { return Iterator(); }

    T& front() { return head_->data; }

    size_t size() const {
      // Normally size() is only used by unittests.
      // We calculate it everytime to keep IterableContainer fits
      // into single register.
      size_t result = 0;
      Node* n = head_;
      while (n != nullptr) {
        ++result;
        n = n->next;
      }
      return result;
    }

    void reset() { FreeList(); }

   private:
    Node* head_;

    void FreeList() {
      while (head_ != nullptr) {
        Node* pre = head_;
        head_ = head_->next;
        delete pre;
      }
    }
  };

  void Push(T data) {
    Node* const new_head = new Node(std::move(data));
    new_head->next = head_.load();
    while (!head_.compare_exchange_weak(new_head->next, new_head)) {
    }
  }

  void Push(ConcurrentQueue<T>& other) {
    Node* pop_head = other.head_.exchange(nullptr);
    // afterwards pop_head is thread safe
    if (pop_head == nullptr) {
      return;
    }

    Node* pop_tail = pop_head;
    while (pop_tail->next != nullptr) {
      pop_tail = pop_tail->next;
    }

    pop_tail->next = head_.load();
    while (!head_.compare_exchange_weak(pop_tail->next, pop_head)) {
    }
  }

  IterableContainer PopAll() {
    return IterableContainer(head_.exchange(nullptr), false);
  }

  IterableContainer ReversePopAll() {
    return IterableContainer(head_.exchange(nullptr), true);
  }

  bool Empty() { return (head_.load() == nullptr); }

  ConcurrentQueue() : head_(nullptr) {}

  ~ConcurrentQueue() {
    Node* pop_head = head_.exchange(nullptr);
    // afterwards pop_head is thread safe
    DestroyUnsafe(pop_head);
  }

  ConcurrentQueue(ConcurrentQueue&& other) {
    head_ = other.head_.exchange(nullptr);
  }

  ConcurrentQueue& operator=(ConcurrentQueue&& other) {
    if (this != &other) {
      Node* other_head = other.head_.exchange(nullptr);
      Node* pop_head = head_.load();
      while (!head_.compare_exchange_weak(pop_head, other_head)) {
      }
      DestroyUnsafe(pop_head);
    }
    return *this;
  }

 private:
  std::atomic<Node*> head_;

  static void DestroyUnsafe(Node* head) {
    while (head != nullptr) {
      Node* pre = head;
      head = head->next;
      delete pre;
    }
  }

  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_CONCURRENT_QUEUE_H_
