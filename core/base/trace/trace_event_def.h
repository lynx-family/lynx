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
static constexpr const char* const LYNX_ENV_GET_BOOL_ENV = "GetBoolEnv";
static constexpr const char* const LYNX_ENV_GET_LONG_ENV = "GetLongEnv";
static constexpr const char* const LYNX_ENV_GET_STRING_ENV = "GetStringEnv";
static constexpr const char* const LYNX_ENV_GET_EXTERNAL_ENV = "GetExternalEnv";
static constexpr const char* const CLAY_LAYOUT_CONTEXT_CREATE_LAYOUT_NODE =
    "LayoutContextClay::CreateLayoutNode";
static constexpr const char* const CLAY_PAINTING_CONTEXT_CREATE_PAINTING_NODE =
    "PaintingContextClay::CreatePaintingNode";
static constexpr const char* const CLAY_NATIVE_VIEW_CONSTRUCTOR =
    "NativeView::NativeView";
static constexpr const char* const CLAY_VIEW_CONTEXT_CREATE_VIEW =
    "ViewContext::CreateView";
static constexpr const char* const CLAY_VIEW_CONTEXT_ADD_VIEW =
    "ViewContext::AddView";
static constexpr const char* const CLAY_VIEW_CONTEXT_CREATE_SHADOW_NODE =
    "ViewContext::CreateShadowNode";
static constexpr const char* const CLAY_VIEW_CONTEXT_SYNC_NATIVE_VIEW_TAGS =
    "ViewContext::SyncNativeViewTags";
static constexpr const char* const
    CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE =
        "ViewRegistry::CreateNativeViewIfAvailable";
static constexpr const char* const
    CLAY_VIEW_REGISTRY_CREATE_NATIVE_VIEW_IF_AVAILABLE_RESULT =
        "ViewRegistry::CreateNativeViewIfAvailableResult";
static constexpr const char* const CLAY_VIEW_REGISTRY_CREATE_VIEW =
    "ViewRegistry::CreateView";
static constexpr const char* const CLAY_VIEW_REGISTRY_CREATE_SHADOW_NODE =
    "ViewRegistry::CreateShadowNode";
static constexpr const char* const
    LYNX_UI_RENDERER_CLAY_COLLECT_WRAPPED_BEHAVIOR_TAGS =
        "LynxUIRendererClay::CollectWrappedBehaviorTags";
static constexpr const char* const LYNX_UI_RENDERER_CLAY_INIT =
    "LynxUIRendererClay::Init";
static constexpr const char* const LYNX_UI_RENDERER_CLAY_SETUP_EVENT_HANDLER =
    "LynxUIRendererClay::SetupEventHandler";
static constexpr const char* const LYNX_UI_RENDERER_CLAY_SETUP_UI_DELEGATE =
    "LynxUIRendererClay::SetupUIDelegate";
static constexpr const char* const LYNX_CLAY_WRAPPED_UI_OWNER_INIT =
    "LynxClayWrappedUIOwner::Init";
static constexpr const char* const
    LYNX_CLAY_WRAPPED_UI_OWNER_CREATE_UI_WITH_SIGN =
        "LynxClayWrappedUIOwner::CreateUIWithSign";
static constexpr const char* const LYNX_CLAY_WRAPPED_PLATFORM_VIEW_INIT =
    "LynxClayWrappedPlatformView::Init";
static constexpr const char* const LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_INIT =
    "LynxClayCustomBehaviorSupport::Init";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_SETUP_EVENT_HANDLER =
        "LynxClayCustomBehaviorSupport::SetupEventHandler";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_ENSURE_EVENT_HANDLER_IF_NEEDED =
        "LynxClayCustomBehaviorSupport::EnsureEventHandlerIfNeeded";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_ENSURE_EVENT_HANDLER =
        "LynxClayCustomBehaviorSupport::EnsureEventHandler";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_SETUP_PAGE_NODE_OWNER =
        "LynxClayCustomBehaviorSupport::SetupPageNodeOwner";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_REFRESH_WRAPPED_BEHAVIOR_TAG_IF_NEEDED =
        "LynxClayCustomBehaviorSupport::RefreshWrappedBehaviorTagIfNeeded";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_CREATE_WRAPPED_PLATFORM_VIEW =
        "LynxClayCustomBehaviorSupport::CreateWrappedPlatformView";
static constexpr const char* const
    LYNX_CLAY_CUSTOM_BEHAVIOR_SUPPORT_SYNC_UI_CONTEXT_COMPATIBILITY =
        "LynxClayCustomBehaviorSupport::SyncUIContextCompatibility";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_BASE_TRACE_TRACE_EVENT_DEF_H_
