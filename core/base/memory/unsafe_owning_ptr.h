// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_MEMORY_UNSAFE_OWNING_PTR_H_
#define CORE_BASE_MEMORY_UNSAFE_OWNING_PTR_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#include "base/include/log/logging.h"

#ifndef NDEBUG
#include <thread>
#endif

namespace lynx {
namespace base {

// UnsafeOwningPtr / UnsafeWeakPtr
// --------------------------------
// A pair of single-threaded smart pointers that model `unique_ptr`-style sole
// ownership while still allowing observers (weak refs) to be derived.
//
// Rationale:
//   * `std::shared_ptr` has atomic ref-counting overhead that is unnecessary
//     when an object is only ever accessed on a single thread.
//   * `std::unique_ptr` models sole ownership but has no weak-observer
//     companion.
//
// Semantics:
//   * `UnsafeOwningPtr<T>` owns exactly one `T` instance. It is move-only and
//     destroys the underlying object when it goes out of scope / is reset /
//     is overwritten.
//   * `UnsafeWeakPtr<T>` is a non-owning observer that can only be created
//     from an `UnsafeOwningPtr<T>`. `Lock()` returns a raw `T*` that is
//     `nullptr` if the owning pointer has already destroyed the object.
//
// Thread safety:
//   * THESE TYPES ARE NOT THREAD SAFE BY DESIGN.
//   * In Debug builds a DCHECK verifies that every operation (creation,
//     copy, destruction, `Lock()`, etc.) happens on the same thread that
//     created the associated `UnsafeOwningPtr`.
//   * In Release builds no thread-affinity check is performed and no atomic
//     operations are used.
//
// Nullptr convention:
//   * `UnsafeOwningPtr` can be empty (`operator bool()` returns false).
//   * `UnsafeWeakPtr::Lock()` returns `nullptr` when the weak ref has
//     expired. Callers MUST check the result before dereferencing.

namespace internal {

// The control block keeps an unowned raw pointer to the managed object plus a
// reference count of live observers. The owner owns the block and clears the
// raw pointer when it destroys the managed object; observers keep the block
// alive until they all go away.
//
// NOTE: ref_count_ is a plain size_t - NOT atomic - because these smart
// pointers are single-thread-only by contract.
class UnsafeOwningControlBlock {
 public:
  explicit UnsafeOwningControlBlock(void* ptr) noexcept
      : managed_ptr_(ptr),
        weak_count_(0)
#ifndef NDEBUG
        ,
        owner_thread_id_(std::this_thread::get_id())
#endif
  {
  }

  UnsafeOwningControlBlock(const UnsafeOwningControlBlock&) = delete;
  UnsafeOwningControlBlock& operator=(const UnsafeOwningControlBlock&) = delete;

  void* managed_ptr() const noexcept { return managed_ptr_; }

  // Called by UnsafeOwningPtr when it destroys the managed object.
  void OnOwnerReleased() noexcept { managed_ptr_ = nullptr; }

  void IncrementWeakCount() noexcept { ++weak_count_; }
  // Returns true when the last weak reference has been released and the
  // caller must delete this control block.
  bool DecrementWeakCount() noexcept {
    DCHECK(weak_count_ > 0);
    --weak_count_;
    return weak_count_ == 0;
  }

  bool HasWeakRefs() const noexcept { return weak_count_ > 0; }

#ifndef NDEBUG
  void CheckThread() const {
    DCHECK(owner_thread_id_ == std::this_thread::get_id());
  }
#else
  void CheckThread() const {}
#endif

 private:
  // Managed object pointer. Cleared (set to nullptr) by the owner when it
  // destroys the object. Weak observers check this for expiration.
  void* managed_ptr_;
  // Number of UnsafeWeakPtr that reference this control block. The owner
  // itself is NOT counted here.
  std::size_t weak_count_;
#ifndef NDEBUG
  std::thread::id owner_thread_id_;
#endif
};

}  // namespace internal

template <typename T>
class UnsafeWeakPtr;

template <typename T>
class UnsafeOwningPtr {
 public:
  using element_type = T;

  // Construct an empty owner.
  constexpr UnsafeOwningPtr() noexcept : block_(nullptr), ptr_(nullptr) {}
  constexpr UnsafeOwningPtr(std::nullptr_t) noexcept
      : block_(nullptr), ptr_(nullptr) {}

  // Adopt ownership of `ptr`. Passing nullptr yields an empty owner.
  explicit UnsafeOwningPtr(T* ptr)
      : block_(ptr ? new internal::UnsafeOwningControlBlock(ptr) : nullptr),
        ptr_(ptr) {}

  UnsafeOwningPtr(const UnsafeOwningPtr&) = delete;
  UnsafeOwningPtr& operator=(const UnsafeOwningPtr&) = delete;

  UnsafeOwningPtr(UnsafeOwningPtr&& other) noexcept
      : block_(other.block_), ptr_(other.ptr_) {
    other.block_ = nullptr;
    other.ptr_ = nullptr;
  }

  // Converting move (e.g. derived -> base). Enabled only when T* can be
  // constructed from U*.
  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  UnsafeOwningPtr(UnsafeOwningPtr<U>&& other) noexcept
      : block_(other.block_), ptr_(static_cast<T*>(other.ptr_)) {
    other.block_ = nullptr;
    other.ptr_ = nullptr;
  }

  UnsafeOwningPtr& operator=(UnsafeOwningPtr&& other) noexcept {
    if (this != &other) {
      Reset();
      block_ = other.block_;
      ptr_ = other.ptr_;
      other.block_ = nullptr;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  UnsafeOwningPtr& operator=(std::nullptr_t) noexcept {
    Reset();
    return *this;
  }

  ~UnsafeOwningPtr() { Reset(); }

  // Returns the raw pointer without giving up ownership.
  T* get() const noexcept { return ptr_; }

  T& operator*() const noexcept {
    DCHECK(ptr_ != nullptr);
    return *ptr_;
  }

  T* operator->() const noexcept {
    DCHECK(ptr_ != nullptr);
    return ptr_;
  }

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  // Destroy the managed object (if any) and detach from the control block.
  // If there are outstanding weak refs the control block is kept alive (but
  // marked expired); otherwise it is destroyed as well.
  void Reset() noexcept {
    if (ptr_ != nullptr) {
      DCHECK(block_ != nullptr);
      block_->CheckThread();
      T* to_delete = ptr_;
      ptr_ = nullptr;
      block_->OnOwnerReleased();
      // Pin the block with a synthetic weak ref so that any weak pointer
      // living inside `to_delete` (e.g. a self-referencing member) cannot
      // delete the block while we are still using it after the destructor
      // returns.
      block_->IncrementWeakCount();
      delete to_delete;
      // Release our synthetic pin. If we were the last observer, delete the
      // block now that nobody else references it.
      if (block_->DecrementWeakCount()) {
        delete block_;
      }
      block_ = nullptr;
    } else {
      DCHECK(block_ == nullptr);
    }
  }

  // Release ownership of the underlying object without destroying it.
  // After this call `*this` is empty and the caller is responsible for the
  // returned pointer. Any outstanding weak refs become expired.
  T* Release() noexcept {
    if (ptr_ == nullptr) {
      DCHECK(block_ == nullptr);
      return nullptr;
    }
    DCHECK(block_ != nullptr);
    block_->CheckThread();
    T* released = ptr_;
    ptr_ = nullptr;
    block_->OnOwnerReleased();
    if (!block_->HasWeakRefs()) {
      delete block_;
    }
    block_ = nullptr;
    return released;
  }

  // Derive a non-owning observer. Valid only while *this is non-empty.
  UnsafeWeakPtr<T> GetWeakPtr() const noexcept {
    if (ptr_ == nullptr) {
      return UnsafeWeakPtr<T>();
    }
    DCHECK(block_ != nullptr);
    block_->CheckThread();
    return UnsafeWeakPtr<T>(block_);
  }

 private:
  template <typename U>
  friend class UnsafeOwningPtr;
  template <typename U>
  friend class UnsafeWeakPtr;

  internal::UnsafeOwningControlBlock* block_;
  T* ptr_;
};

template <typename T>
class UnsafeWeakPtr {
 public:
  using element_type = T;

  constexpr UnsafeWeakPtr() noexcept : block_(nullptr) {}
  constexpr UnsafeWeakPtr(std::nullptr_t) noexcept : block_(nullptr) {}

  UnsafeWeakPtr(const UnsafeWeakPtr& other) noexcept : block_(other.block_) {
    if (block_ != nullptr) {
      block_->CheckThread();
      block_->IncrementWeakCount();
    }
  }

  // Converting copy (e.g. derived -> base).
  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  UnsafeWeakPtr(const UnsafeWeakPtr<U>& other) noexcept : block_(other.block_) {
    if (block_ != nullptr) {
      block_->CheckThread();
      block_->IncrementWeakCount();
    }
  }

  UnsafeWeakPtr(UnsafeWeakPtr&& other) noexcept : block_(other.block_) {
    other.block_ = nullptr;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  UnsafeWeakPtr(UnsafeWeakPtr<U>&& other) noexcept : block_(other.block_) {
    other.block_ = nullptr;
  }

  UnsafeWeakPtr& operator=(const UnsafeWeakPtr& other) noexcept {
    if (this != &other) {
      Reset();
      block_ = other.block_;
      if (block_ != nullptr) {
        block_->CheckThread();
        block_->IncrementWeakCount();
      }
    }
    return *this;
  }

  UnsafeWeakPtr& operator=(UnsafeWeakPtr&& other) noexcept {
    if (this != &other) {
      Reset();
      block_ = other.block_;
      other.block_ = nullptr;
    }
    return *this;
  }

  UnsafeWeakPtr& operator=(std::nullptr_t) noexcept {
    Reset();
    return *this;
  }

  ~UnsafeWeakPtr() { Reset(); }

  // Returns the raw pointer (or nullptr if expired / empty).
  //
  // The caller MUST only use the returned pointer on the same thread that
  // owns the originating `UnsafeOwningPtr`. The pointer is invalidated by
  // any operation that destroys the owning pointer.
  T* Lock() const noexcept {
    if (block_ == nullptr) {
      return nullptr;
    }
    block_->CheckThread();
    return static_cast<T*>(block_->managed_ptr());
  }

  bool Expired() const noexcept { return Lock() == nullptr; }

  explicit operator bool() const noexcept { return !Expired(); }

  void Reset() noexcept {
    if (block_ != nullptr) {
      block_->CheckThread();
      if (block_->DecrementWeakCount()) {
        // No more weak refs. Delete the block only if the owner has already
        // released the object (otherwise the owner still references it).
        if (block_->managed_ptr() == nullptr) {
          delete block_;
        }
      }
      block_ = nullptr;
    }
  }

 private:
  template <typename U>
  friend class UnsafeOwningPtr;
  template <typename U>
  friend class UnsafeWeakPtr;

  explicit UnsafeWeakPtr(internal::UnsafeOwningControlBlock* block) noexcept
      : block_(block) {
    if (block_ != nullptr) {
      block_->IncrementWeakCount();
    }
  }

  internal::UnsafeOwningControlBlock* block_;
};

// Convenience factory, akin to std::make_unique.
template <typename T, typename... Args>
UnsafeOwningPtr<T> MakeUnsafeOwning(Args&&... args) {
  return UnsafeOwningPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
bool operator==(const UnsafeOwningPtr<T>& p, std::nullptr_t) noexcept {
  return p.get() == nullptr;
}
template <typename T>
bool operator==(std::nullptr_t, const UnsafeOwningPtr<T>& p) noexcept {
  return p.get() == nullptr;
}
template <typename T>
bool operator!=(const UnsafeOwningPtr<T>& p, std::nullptr_t) noexcept {
  return p.get() != nullptr;
}
template <typename T>
bool operator!=(std::nullptr_t, const UnsafeOwningPtr<T>& p) noexcept {
  return p.get() != nullptr;
}

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_MEMORY_UNSAFE_OWNING_PTR_H_
