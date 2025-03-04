// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_EVENT_RUNTIME_CONSTANTS_H_
#define CORE_RUNTIME_BINDINGS_COMMON_EVENT_RUNTIME_CONSTANTS_H_

#include <stdint.h>

namespace lynx {
namespace runtime {

// For Runtime communication API
static constexpr const char* kGetDevTool = "getDevtool";
static constexpr const char* kGetJSContext = "getJSContext";
static constexpr const char* kGetCoreContext = "getCoreContext";
static constexpr const char* kGetUIContext = "getUIContext";
static constexpr const char* kGetCustomSectionSync = "getCustomSectionSync";
// For Runtime Timing API
static constexpr const char kPerformanceObject[] = "performance";
static constexpr const char* kGeneratePipelineOptions =
    "_generatePipelineOptions";
static constexpr const char* kOnPipelineStart = "_onPipelineStart";
static constexpr const char* kBindPipelineIDWithTimingFlag =
    "_bindPipelineIDWithTimingFlag";
static constexpr const char* kMarkTiming = "_markTiming";
static constexpr const char* kAddTimingListener = "addTimingListener";

static constexpr const char* kDevTool = "Devtool";
static constexpr const char* kJSContext = "JSContext";
static constexpr const char* kCoreContext = "CoreContext";
static constexpr const char* kUIContext = "UIContext";
static constexpr const char* kUnknown = "Unknown";

static constexpr const char* kPostMessage = "postMessage";
static constexpr const char* kDispatchEvent = "dispatchEvent";
static constexpr const char* kAddEventListener = "addEventListener";
static constexpr const char* kRemoveEventListener = "removeEventListener";
static constexpr const char kOnTriggerEvent[] = "onTriggerEvent";
static constexpr const char kInnerRuntimeProxy[] = "__runtime_proxy";
static constexpr const char kMessage[] = "message";
static constexpr const char kType[] = "type";
static constexpr const char kData[] = "data";
static constexpr const char kOrigin[] = "origin";

static constexpr const char* kReplaceStyleSheetByIdWithBase64 =
    "replaceStyleSheetByIdWithBase64";
static constexpr const char* kRemoveStyleSheetById = "removeStyleSheetById";

static constexpr const char kTargetSdkVersion[] = "targetSdkVersion";

static constexpr const char kAddReporterCustomInfo[] =
    "__addReporterCustomInfo";

static constexpr const char kProfileStart[] = "profileStart";
static constexpr const char kProfileEnd[] = "profileEnd";
static constexpr const char kProfileMark[] = "profileMark";
static constexpr const char kProfileFlowId[] = "profileFlowId";
static constexpr const char kArgs[] = "args";
static constexpr const char kFlowId[] = "flowId";
static constexpr const char kIsProfileRecording[] = "isProfileRecording";

static constexpr const char* kQueueMicrotask = "queueMicrotask";

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_EVENT_RUNTIME_CONSTANTS_H_
