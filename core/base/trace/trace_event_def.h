// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_TRACE_TRACE_EVENT_DEF_H_
#define CORE_BASE_TRACE_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

static constexpr const char* const THREAD_MERGER_CONSTRUCTOR =
    "ThreadMerger::ThreadMerger";
static constexpr const char* const THREAD_MERGER_DECONSTRUCTOR =
    "ThreadMerger::~ThreadMerger";
static constexpr const char* const MESSAGE_LOOP_ANDROID_VASYNC_FLUSH_TASKS =
    "MessageLoopAndroidVSync::FlushTasks";
static constexpr const char* const VSYNC_MONITOR_DARWIN_ON_MAIN_DISPLAY =
    "LynxVSyncPulse::onMainDisplay";
static constexpr const char* const JSI_OBJECT_GET =
    "LynxPlatformJSIObjectAndroid::get";
static constexpr const char* const JSI_OBJECT_GET_DESCRIPTOR =
    "LynxPlatformJSIObjectAndroid::GetJSIObjectDescriptor";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_BASE_TRACE_TRACE_EVENT_DEF_H_
