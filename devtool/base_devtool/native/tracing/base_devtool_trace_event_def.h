// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TRACING_BASE_DEVTOOL_TRACE_EVENT_DEF_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TRACING_BASE_DEVTOOL_TRACE_EVENT_DEF_H_

#include <string>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

static constexpr const char* const LYNX_TRACE_CATEGORY_DEVTOOL = "devtool";

inline constexpr const char* const
    INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_DISPATCH_MESSAGE_ASYNC =
        "InspectorClientDelegateBaseImpl::DispatchMessageAsync";
inline constexpr const char* const
    INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_FLUSH_MESSAGE_QUEUE_WITH_LOCK_HELD =
        "InspectorClientDelegateBaseImpl::FlushMessageQueueWithLockHeld";
inline constexpr const char* const
    INSPECTOR_CLIENT_DELEGATE_BASE_IMPL_DISPATCH_INIT_MESSAGE =
        "InspectorClientDelegateBaseImpl::DispatchInitMessage";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TRACING_BASE_DEVTOOL_TRACE_EVENT_DEF_H_
