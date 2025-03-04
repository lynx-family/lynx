// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides weak pointers and weak pointer factories that work like
// Chromium's |base::WeakPtr<T>| and |base::WeakPtrFactory<T>|.

#ifndef BASE_INCLUDE_FML_MEMORY_WEAK_PTR_H_
#define BASE_INCLUDE_FML_MEMORY_WEAK_PTR_H_

#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/task_runner_checker.h"
#include "base/include/fml/memory/weak_ptr_internal.h"
#include "base/include/fml/task_runner.h"

#ifdef __OBJC__
#include "base/include/platform/darwin/type_utils.h"
#endif

namespace lynx {
namespace fml {

struct DebugTaskRunnerChecker {
  FML_DECLARE_TASK_RUNNER_CHECKER(checker);
};

// Forward declaration, so |WeakPtr<T>| can friend it.
template <typename T>
class WeakPtrFactory;

// Class for "weak pointers" that can be invalidated. Valid weak pointers
// can only originate from a |WeakPtrFactory| (see below), though weak
// pointers are copyable and movable.
//
// Weak pointers are not in general thread-safe. They may only be *used* on
// threads which are belong to the same |TaskRunner|, namely the same thread as
// the "originating" |WeakPtrFactory| (which can invalidate the weak pointers
// that it generates).
//
// However, weak pointers may be passed to other threads, reset on other
// threads, or destroyed on other threads. They may also be reassigned on
// other threads (in which case they should then only be used on the thread
// corresponding to the new "originating" |WeakPtrFactory|).
template <typename T>
class WeakPtr {
 public:
#ifdef __OBJC__
  static_assert(!base::is_objc_class<T*>::value,
                "WeakPtr<T> is not supported for Objective-C class");
#endif
  WeakPtr() : ptr_(nullptr) {}

  WeakPtr(const WeakPtr<T>& r) = default;

  template <typename U>
  // NOLINTNEXTLINE
  WeakPtr(const WeakPtr<U>& r)
      : ptr_(static_cast<T*>(r.ptr_)), flag_(r.flag_), checker_(r.checker_) {}

  WeakPtr(WeakPtr<T>&& r) = default;

  template <typename U>
  // NOLINTNEXTLINE
  WeakPtr(WeakPtr<U>&& r)
      : ptr_(static_cast<T*>(r.ptr_)),
        flag_(std::move(r.flag_)),
        checker_(r.checker_) {}

  ~WeakPtr() = default;

  WeakPtr<T>& operator=(const WeakPtr<T>& r) = default;

  WeakPtr<T>& operator=(WeakPtr<T>&& r) = default;

  void reset() { flag_ = nullptr; }

  // The following methods should only be called on the same thread as the
  // "originating" |WeakPtrFactory|.

  explicit operator bool() const {
    CheckThreadSafety();
    return flag_ && flag_->is_valid();
  }

  T* get() const {
    CheckThreadSafety();
    return *this ? ptr_ : nullptr;
  }

  T& operator*() const {
    CheckThreadSafety();
    LYNX_BASE_DCHECK(*this);
    return *get();
  }

  T* operator->() const {
    CheckThreadSafety();
    LYNX_BASE_DCHECK(*this);
    return get();
  }

  bool operator==(const WeakPtr<T>& rhs) const {
    CheckThreadSafety();
    rhs.CheckThreadSafety();
    return get() == rhs.get();
  }

  bool operator!=(const WeakPtr<T>& rhs) const { return !operator==(rhs); }

 protected:
  void CheckThreadSafety() const {
    FML_DCHECK_TASK_RUNNER_IS_CURRENT(checker_.checker);
  }

 private:
  template <typename U>
  friend class WeakPtr;
  friend class WeakPtrFactory<T>;

  explicit WeakPtr(T* ptr, fml::RefPtr<fml::internal::WeakPtrFlag>&& flag,
                   DebugTaskRunnerChecker checker)
      : ptr_(ptr), flag_(std::move(flag)), checker_(checker) {}

  T* ptr_;
  fml::RefPtr<fml::internal::WeakPtrFlag> flag_;
  DebugTaskRunnerChecker checker_;
};

// Class that produces (valid) |WeakPtr<T>|s. Typically, this is used as a
// member variable of |T| (preferably the last one -- see below), and |T|'s
// methods control how weak pointers to it are vended. This class is not
// thread-safe, and should only be created, destroyed and used on threads that
// are belong to the same |TaskRunner|
//
// Example:
//
//  class Controller {
//   public:
//    Controller() : ..., weak_factory_(this) {}
//    ...
//
//    void SpawnWorker() { Worker::StartNew(weak_factory_.GetWeakPtr()); }
//    void WorkComplete(const Result& result) { ... }
//
//   private:
//    ...
//
//    // Member variables should appear before the |WeakPtrFactory|, to ensure
//    // that any |WeakPtr|s to |Controller| are invalidated before its member
//    // variables' destructors are executed.
//    WeakPtrFactory<Controller> weak_factory_;
//  };
//
//  class Worker {
//   public:
//    static void StartNew(const WeakPtr<Controller>& controller) {
//      Worker* worker = new Worker(controller);
//      // Kick off asynchronous processing....
//    }
//
//   private:
//    Worker(const WeakPtr<Controller>& controller) : controller_(controller) {}
//
//    void DidCompleteAsynchronousProcessing(const Result& result) {
//      if (controller_)
//        controller_->WorkComplete(result);
//    }
//
//    WeakPtr<Controller> controller_;
//  };
template <typename T>
class WeakPtrFactory {
 public:
#ifdef __OBJC__
  static_assert(!base::is_objc_class<T*>::value,
                "WeakPtr<T> is not supported for Objective-C class");
#endif

  explicit WeakPtrFactory(T* ptr)
      : ptr_(ptr), flag_(fml::MakeRefCounted<fml::internal::WeakPtrFlag>()) {
    LYNX_BASE_DCHECK(ptr_);
  }

  ~WeakPtrFactory() {
    CheckThreadSafety();
    flag_->Invalidate();
  }

  // Gets a new weak pointer, which will be valid until this object is
  // destroyed.
  WeakPtr<T> GetWeakPtr() const {
    return WeakPtr<T>(ptr_, flag_.Clone(), checker_);
  }

 private:
  // Note: See weak_ptr_internal.h for an explanation of why we store the
  // pointer here, instead of in the "flag".
  T* const ptr_;
  fml::RefPtr<fml::internal::WeakPtrFlag> flag_;

  void CheckThreadSafety() const {
    FML_DCHECK_TASK_RUNNER_IS_CURRENT(checker_.checker);
  }

  DebugTaskRunnerChecker checker_;

  BASE_DISALLOW_COPY_AND_ASSIGN(WeakPtrFactory);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::WeakPtr;
using lynx::fml::WeakPtrFactory;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_MEMORY_WEAK_PTR_H_
