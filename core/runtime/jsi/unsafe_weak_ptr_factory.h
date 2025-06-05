// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_UNSAFE_WEAK_PTR_FACTORY_H_
#define CORE_RUNTIME_JSI_UNSAFE_WEAK_PTR_FACTORY_H_

#include <cstddef>
#include <utility>

namespace lynx {
namespace piper {

template <typename T>
class UnsafeWeakPtr;
template <typename T>
class UnsafeWeakPtrFactory;

namespace internal {

template <typename T>
struct UnsafeWeakPtrControlBlock {
  T* target_ = nullptr;
  int weak_references_ = 0;  // Non-atomic for non-thread-safety

  explicit UnsafeWeakPtrControlBlock(T* t) : target_(t), weak_references_(0) {}

  void AddWeakRef() { weak_references_++; }

  void ReleaseWeakRef() {
    weak_references_--;
    if (weak_references_ == 0 && !target_) {
      delete this;
    }
  }

  void Invalidate() {
    target_ = nullptr;
    if (weak_references_ == 0) {
      delete this;
    }
  }

  T* Lock() const { return target_; }
};

}  // namespace internal

template <typename T>
class UnsafeWeakPtr {
 public:
  UnsafeWeakPtr() = default;

  explicit UnsafeWeakPtr(internal::UnsafeWeakPtrControlBlock<T>* cb) : cb_(cb) {
    if (cb_) {
      cb_->AddWeakRef();
    }
  }

  UnsafeWeakPtr(const UnsafeWeakPtr& other) : cb_(other.cb_) {
    if (cb_) {
      cb_->AddWeakRef();
    }
  }

  UnsafeWeakPtr(UnsafeWeakPtr&& other) noexcept : cb_(other.cb_) {
    other.cb_ = nullptr;
  }

  ~UnsafeWeakPtr() {
    if (cb_) {
      cb_->ReleaseWeakRef();
    }
  }

  UnsafeWeakPtr& operator=(const UnsafeWeakPtr& other) {
    if (this != &other) {
      if (cb_) {
        cb_->ReleaseWeakRef();
      }
      cb_ = other.cb_;
      if (cb_) {
        cb_->AddWeakRef();
      }
    }
    return *this;
  }

  UnsafeWeakPtr& operator=(UnsafeWeakPtr&& other) noexcept {
    if (this != &other) {
      if (cb_) {
        cb_->ReleaseWeakRef();
      }
      cb_ = other.cb_;
      other.cb_ = nullptr;
    }
    return *this;
  }

  T* Lock() const { return cb_ ? cb_->Lock() : nullptr; }

  bool Expired() const { return !Lock(); }

  void Reset() {
    if (cb_) {
      cb_->ReleaseWeakRef();
    }
    cb_ = nullptr;
  }

  explicit operator bool() const { return Lock() != nullptr; }

 private:
  template <typename U>
  friend class UnsafeWeakPtrFactory;
  internal::UnsafeWeakPtrControlBlock<T>* cb_ = nullptr;
};

template <typename T>
class UnsafeWeakPtrFactory {
 public:
  UnsafeWeakPtrFactory()
      : control_block_(
            new internal::UnsafeWeakPtrControlBlock<T>(static_cast<T*>(this))) {
  }

  virtual ~UnsafeWeakPtrFactory() {
    if (control_block_) {
      control_block_->Invalidate();
    }
  }

  UnsafeWeakPtrFactory(const UnsafeWeakPtrFactory&) = delete;
  UnsafeWeakPtrFactory& operator=(const UnsafeWeakPtrFactory&) = delete;
  UnsafeWeakPtrFactory(UnsafeWeakPtrFactory&&) = delete;
  UnsafeWeakPtrFactory& operator=(UnsafeWeakPtrFactory&&) = delete;

  UnsafeWeakPtr<T> WeakFromThis() { return UnsafeWeakPtr<T>(control_block_); }

  UnsafeWeakPtr<const T> WeakFromThis() const {
    // For const-correctness with WeakFromThis() const, the control block should
    // ideally also be typed for const T, or we accept that WeakFromThis() const
    // still gives a UnsafeWeakPtr<const T> that internally might point
    // to a UnsafeWeakPtrControlBlock<T>. This is a common pattern. The
    // reinterpret_cast is to satisfy the type system for the
    // UnsafeWeakPtr<const T> constructor.
    return UnsafeWeakPtr<const T>(
        reinterpret_cast<internal::UnsafeWeakPtrControlBlock<const T>*>(
            control_block_));
  }

 protected:
  internal::UnsafeWeakPtrControlBlock<T>* GetControlBlock() const {
    return control_block_;
  }

 private:
  internal::UnsafeWeakPtrControlBlock<T>* control_block_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_UNSAFE_WEAK_PTR_FACTORY_H_
