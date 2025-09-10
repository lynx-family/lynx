// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_FML_TRACE_EVENT_DEF_H_
#define BASE_INCLUDE_FML_FML_TRACE_EVENT_DEF_H_

static constexpr const char* const CONCURRENT_WORKER_AWOKE =
    "ConcurrentWorker AWoke";
static constexpr const char* const MESSAGE_LOOP_FLUSH_TASK =
    "MessageLoop::FlushTasks";
static constexpr const char* const MESSAGE_LOOP_FLUSH_VASYNC_ALIGNED_TASKS =
    "MessageLoop::FlushVSyncAlignedTasks";
static constexpr const char* const MESSAGE_LOOP_IMPL_BIND =
    "MessageLoopImpl::Bind";

#endif  // BASE_INCLUDE_FML_FML_TRACE_EVENT_DEF_H_
