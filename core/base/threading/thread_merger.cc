// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/thread_merger.h"

#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {

void ThreadMerger::Merge(fml::TaskRunner* owner, fml::TaskRunner* subsumed) {
  DCHECK(owner);
  DCHECK(subsumed);

  if (owner == subsumed) {
    return;
  }

  // ensure on owner's thread.
  DCHECK(owner->RunsTasksOnCurrentThread());

  if (subsumed->RunsTasksOnCurrentThread()) {
    return;
  }

  fml::AutoResetWaitableEvent arwe;
  subsumed->PostEmergencyTask([owner_id = owner->GetTaskQueueId(),
                               subsumed_id = subsumed->GetTaskQueueId(),
                               &arwe]() {
    fml::MessageLoopTaskQueues::GetInstance()->Merge(owner_id, subsumed_id);
    arwe.Signal();
  });
  arwe.Wait();
}

ThreadMerger::ThreadMerger(fml::TaskRunner* owner, fml::TaskRunner* subsumed)
    : owner_(owner), subsumed_(subsumed) {
  TRACE_EVENT("lynx", "ThreadMerger::ThreadMerger");

  Merge(owner_, subsumed_);
}

ThreadMerger::~ThreadMerger() {
  TRACE_EVENT("lynx", "ThreadMerger::~ThreadMerger");
  if (owner_ == subsumed_) {
    return;
  }

  // ensure on owner's thread.
  DCHECK(owner_->RunsTasksOnCurrentThread());

  fml::MessageLoopTaskQueues::GetInstance()->Unmerge(
      owner_->GetTaskQueueId(), subsumed_->GetTaskQueueId());
}

ThreadMerger::ThreadMerger(ThreadMerger&& other)
    : owner_(other.owner_), subsumed_(other.subsumed_) {
  other.owner_ = nullptr;
  other.subsumed_ = nullptr;
}

ThreadMerger& ThreadMerger::operator=(ThreadMerger&& other) {
  owner_ = other.owner_;
  subsumed_ = other.subsumed_;
  other.owner_ = nullptr;
  other.subsumed_ = nullptr;
  return *this;
}

}  // namespace base
}  // namespace lynx
