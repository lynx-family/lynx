// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_TRACE_EVENT_DEF_H_
#define CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

inline constexpr const char* const kJSTaskEnqueue = "JSTaskEnqueue";
inline constexpr const char* const kJSTaskCallJSFunction =
    "JSTask::CallJSFunction";
inline constexpr const char* const kJSTaskCallJSApiCallbackWithValue =
    "JSTask::CallJSApiCallbackWithValue";
inline constexpr const char* const kJSTaskCallJSApiCallback =
    "JSTask::CallJSApiCallback";
inline constexpr const char* const kJSTaskCallJSIntersectionObserver =
    "JSTask::CallJSIntersectionObserver";
inline constexpr const char* const kJSTaskEvaluateScript =
    "JSTask::EvaluateScript";
inline constexpr const char* const kJSTaskAddLifecycleListener =
    "JSTask::AddLifecycleListener";
inline constexpr const char* const kJSTaskInitRuntime = "JSTask::InitRuntime";
inline constexpr const char* const kJSTaskAttachRuntime =
    "JSTask::AttachRuntime";
inline constexpr const char* const kJSTaskCallJSCallback =
    "JSTask::CallJSCallback";
inline constexpr const char* const kJSTaskNotifyJSUpdatePageData =
    "JSTask::NotifyJSUpdatePageData";
inline constexpr const char* const kJSTaskOnCardConfigDataChanged =
    "JSTask::OnCardConfigDataChanged";
inline constexpr const char* const kJSTaskOnJSSourcePrepared =
    "JSTask::OnJSSourcePrepared";
inline constexpr const char* const kJSTaskDispatchMessageEvent =
    "JSTask::DispatchMessageEvent";
inline constexpr const char* const kJSTaskOnGlobalPropsUpdated =
    "JSTask::OnGlobalPropsUpdated";
inline constexpr const char* const kJSTaskOnJSAppReload =
    "JSTask::OnJSAppReload";
inline constexpr const char* const kJSTaskOnI18nResourceChanged =
    "JSTask::OnI18nResourceChanged";
inline constexpr const char* const kJSTaskOnComponentDecoded =
    "JSTask::OnComponentDecoded";
inline constexpr const char* const kJSTaskSetPageOptions =
    "JSTask::SetPageOptions";
inline constexpr const char* const kTaskName = "task_name";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_TRACE_EVENT_DEF_H_
