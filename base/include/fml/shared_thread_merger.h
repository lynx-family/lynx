// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_SHARED_THREAD_MERGER_H_
#define BASE_INCLUDE_FML_SHARED_THREAD_MERGER_H_

#include <condition_variable>
#include <map>
#include <mutex>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/message_loop_task_queues.h"

namespace lynx {
namespace fml {

class RasterThreadMerger;

typedef void* RasterThreadMergerId;

/// Instance of this class is shared between multiple |RasterThreadMerger|
/// instances, Most of the callings from |RasterThreadMerger| will be redirected
/// to this class with one more caller parameter.
class SharedThreadMerger
    : public fml::RefCountedThreadSafe<SharedThreadMerger> {
 public:
  SharedThreadMerger(TaskQueueId owner, TaskQueueId subsumed);

  // It's called by |RasterThreadMerger::MergeWithLease|.
  // See the doc of |RasterThreadMerger::MergeWithLease|.
  bool MergeWithLease(RasterThreadMergerId caller, size_t lease_term);

  // It's called by |RasterThreadMerger::UnMergeNowIfLastOne|.
  // See the doc of |RasterThreadMerger::UnMergeNowIfLastOne|.
  bool UnMergeNowIfLastOne(RasterThreadMergerId caller);

  // It's called by |RasterThreadMerger::ExtendLeaseTo|.
  // See the doc of |RasterThreadMerger::ExtendLeaseTo|.
  void ExtendLeaseTo(RasterThreadMergerId caller, size_t lease_term);

  // It's called by |RasterThreadMerger::IsMergedUnSafe|.
  // See the doc of |RasterThreadMerger::IsMergedUnSafe|.
  bool IsMergedUnSafe() const;

  // It's called by |RasterThreadMerger::IsEnabledUnSafe|.
  // See the doc of |RasterThreadMerger::IsEnabledUnSafe|.
  bool IsEnabledUnSafe() const;

  // It's called by |RasterThreadMerger::Enable| or
  // |RasterThreadMerger::Disable|.
  void SetEnabledUnSafe(bool enabled);

  // It's called by |RasterThreadMerger::DecrementLease|.
  // See the doc of |RasterThreadMerger::DecrementLease|.
  bool DecrementLease(RasterThreadMergerId caller);

 private:
  fml::TaskQueueId owner_;
  fml::TaskQueueId subsumed_;
  fml::MessageLoopTaskQueues* task_queues_;
  std::mutex mutex_;
  bool enabled_;

  /// The |MergeWithLease| or |ExtendLeaseTo| method will record the caller
  /// into this lease_term_by_caller_ map, |UnMergeNowIfLastOne|
  /// method will remove the caller from this lease_term_by_caller_.
  std::map<RasterThreadMergerId, std::atomic_size_t> lease_term_by_caller_;

  bool IsAllLeaseTermsZeroUnSafe() const;

  bool UnMergeNowUnSafe();

  BASE_DISALLOW_COPY_AND_ASSIGN(SharedThreadMerger);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::SharedThreadMerger;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_SHARED_THREAD_MERGER_H_
