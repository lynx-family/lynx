// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_DEVTOOL_TRACE_EVENT_DEF_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_DEVTOOL_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"
#include "devtool/base_devtool/native/tracing/base_devtool_trace_event_def.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

inline constexpr const char* const ELEMENT_OBSERVER_ON_DOCUMENT_UPDATE =
    "OnDocumentUpdated";
inline constexpr const char* const ELEMENT_OBSERVER_ON_ELEMENT_NODE_ADDED =
    "OnElementNodeAdded";
inline constexpr const char* const ELEMENT_OBSERVER_ON_ELEMENT_NODE_REMOVED =
    "OnElementNodeRemoved";
inline constexpr const char* const ELEMENT_OBSERVER_ON_CHARACTER_DATA_MODIFIED =
    "OnCharacterDataModified";
inline constexpr const char* const ELEMENT_OBSERVER_ON_ELEMENT_DATA_MODEL_SET =
    "OnElementDataModelSet";
inline constexpr const char* const
    ELEMENT_OBSERVER_ON_ELEMENT_MANAGER_WILL_DESTORY =
        "OnElementManagerWillDestroy";
inline constexpr const char* const ELEMENT_OBSERVER_ON_SET_NATIVE_PROPS =
    "OnSetNativeProps";
inline constexpr const char* const ELEMENT_OBSERVER_ON_CSS_STYLE_SHEET_ADDED =
    "OnCSSStyleSheetAdded";

inline constexpr const char* const LYNX_ENGINE_VERSION = "LynxEngineVersion";
inline constexpr const char* const FRAME_TRACE_SERVICE_STARTED_IN_BROWSER =
    "TracingStartedInBrowser";
inline constexpr const char* const FRAME_TRACE_SERVICE_SET_LAYER_TREE_ID =
    "SetLayerTreeId";
inline constexpr const char* const FRAME_TRACE_SERVICE_SCREENSHOT =
    "Screenshot";
inline constexpr const char* const
    FRAME_TRACE_SERVICE_NEEDS_BEGIN_FRAME_CHANGED = "NeedsBeginFrameChanged";
inline constexpr const char* const FRAME_TRACE_SERVICE_BEGIN_FRAME =
    "BeginFrame";
inline constexpr const char* const FRAME_TRACE_SERVICE_DRAW_FRAME = "DrawFrame";
inline constexpr const char* const INSTANCE_COUNTER_TRACE_UPDATE_COUNTERS =
    "UpdateCounters";

inline constexpr const char* const LYNX_DEVTOOL_MEDIATOR_DISPATCH_BTS_MESSAGE =
    "LynxDevToolMediator::DispatchBTSMessage";
inline constexpr const char* const LYNX_DEVTOOL_MEDIATOR_DISPATCH_MTS_MESSAGE =
    "LynxDevToolMediator::DispatchMTSMessage";

inline constexpr const char* const
    INSPECTOR_CLIENT_DELEGATE_IMPL_SEND_RESPONSE =
        "InspectorClientDelegateImpl::SendResponse";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_DEVTOOL_TRACE_EVENT_DEF_H_
