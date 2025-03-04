// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Internal implementation details for ref_counted.h.

#ifndef BASE_INCLUDE_FML_MEMORY_REF_COUNTED_INTERNAL_H_
#define BASE_INCLUDE_FML_MEMORY_REF_COUNTED_INTERNAL_H_

#include <atomic>

#include "base/include/fml/macros.h"

namespace lynx {
namespace fml {
namespace internal {

// See ref_counted.h for comments on the public methods.
class RefCountedThreadSafeBase {
 public:
  void AddRef() const {
#ifndef NDEBUG
    // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
    // DCHECK(!adoption_required_);
    // DCHECK(!destruction_started_);
#endif
    ref_count_.fetch_add(1u, std::memory_order_relaxed);
  }

  bool HasOneRef() const {
    return ref_count_.load(std::memory_order_acquire) == 1u;
  }

  void AssertHasOneRef() const { HasOneRef(); }

  // Returns the current reference count (with no barriers). This is subtle, and
  // should be used only for debugging.
  int SubtleRefCountForDebug() const {
    return ref_count_.load(std::memory_order_relaxed);
  }

 protected:
  RefCountedThreadSafeBase();
  ~RefCountedThreadSafeBase();

  // Returns true if the object should self-delete.
  bool Release() const {
#ifndef NDEBUG
    // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
    // DCHECK(!adoption_required_);
    // DCHECK(!destruction_started_);
#endif
    // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
    // DCHECK(ref_count_.load(std::memory_order_acquire) != 0u);
    // TODO(vtl): We could add the following:
    //     if (ref_count_.load(std::memory_order_relaxed) == 1u) {
    // #ifndef NDEBUG
    //       destruction_started_= true;
    // #endif
    //       return true;
    //     }
    // This would be correct. On ARM (an Nexus 4), in *single-threaded* tests,
    // this seems to make the destruction case marginally faster (barely
    // measurable), and while the non-destruction case remains about the same
    // (possibly marginally slower, but my measurements aren't good enough to
    // have any confidence in that). I should try multithreaded/multicore tests.
    if (ref_count_.fetch_sub(1u, std::memory_order_release) == 1u) {
      std::atomic_thread_fence(std::memory_order_acquire);
#ifndef NDEBUG
      destruction_started_ = true;
#endif
      return true;
    }
    return false;
  }

#ifndef NDEBUG
  void Adopt() {
    // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
    // DCHECK(adoption_required_);
    adoption_required_ = false;
  }
#endif

 private:
  mutable std::atomic_uint_fast32_t ref_count_;

#ifndef NDEBUG
  mutable bool adoption_required_;
  mutable bool destruction_started_;
#endif

  BASE_DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
};

inline RefCountedThreadSafeBase::RefCountedThreadSafeBase()
    : ref_count_(1u)
#ifndef NDEBUG
      ,
      adoption_required_(true),
      destruction_started_(false)
#endif
{
}

inline RefCountedThreadSafeBase::~RefCountedThreadSafeBase() {
#ifndef NDEBUG
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(!adoption_required_);
  // Should only be destroyed as a result of |Release()|.
  // DCHECK(destruction_started_);
#endif
}

}  // namespace internal
}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_MEMORY_REF_COUNTED_INTERNAL_H_
