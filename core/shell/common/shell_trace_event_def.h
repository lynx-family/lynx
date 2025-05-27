// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_COMMON_SHELL_TRACE_EVENT_DEF_H_
#define CORE_SHELL_COMMON_SHELL_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

/**
 * @trace_description: Load the Lynx bundle.
 */
inline constexpr const char* const LYNX_ENGINE_LOAD_TEMPLATE =
    "LynxEngine::LoadTemplate";
/**
 * @trace_description: Load the Lynx bundle with templateBundle. Reference:
 * @link{https://lynxjs.org/api/lynx-native-api/template-bundle.html}
 */
inline constexpr const char* const LYNX_ENGINE_LOAD_TEMPLATE_BUNDLE =
    "LynxEngine::LoadTemplateBundle";

inline constexpr const char* const LYNX_ENGINE_DESTRUCTOR =
    "LynxEngine::~LynxEngine";

/**
 * @trace_description: Execute the @args{module_name}.@args{method} method on
 * the background scripting thread (historically known as "JS Thread").
 */
inline constexpr const char* const RUNTIME_ACTOR_CALL_JS_FUNCTION =
    "CallJSFunction";

/**
 * @trace_description: Batch update for component on Engine Thread(historically
 * known as "Tasm Thread").
 */
inline constexpr const char* const LYNX_BATCHED_UPDATE_DATA =
    "LynxBatchedUpdateData";

/**
 * @trace_description: Start a pipeline. Pipeline trigger is
 * @args{pipeline_origin}. About Lynx pipeline:
 * @link{https://lynxjs.org/api/lynx-api/performance-api/performance-entry/pipeline-entry.html}
 */
inline constexpr const char* const TIMING_PIPELINE_START =
    "Timing::OnPipelineStart";

/**
 * @trace_description: Execute UI operations, such as creating
 * platform ui.
 */
inline constexpr const char* const UI_OPERATION_ASYNC_QUEUE_FLUSH =
    "LynxUIOperationAsyncQueue::FlushInterval";
/**
 * @trace_description: Synchronously execute UI operations, such as creating
 * platform ui.
 */
inline constexpr const char* const UI_OPERATION_QUEUE_FLUSH =
    "LynxUIOperationQueue.Flush";
/**
 * @trace_description: Execute the platform ui operations, such as creating,
 * inserting, updating, and deleting platform ui.
 */
inline constexpr const char* const UI_OPERATION_QUEUE_EXECUTE =
    "LynxUIOperationQueue::ExecuteOperation";

inline constexpr const char* const DYNAMIC_UI_OPERATION_QUEUE_TRANSFER =
    "DynamicUIOperationQueue::Transfer";
inline constexpr const char* const LAYOUT_MEDIATOR_ON_LAYOUT_AFTER =
    "LayoutMediator.OnLayoutAfter";
inline constexpr const char* const LAYOUT_MEDIATOR_HANDLE_PENDING_LAYOUT_TASK =
    "LayoutMediator.HandlePendingLayoutTask";
inline constexpr const char* const
    LAYOUT_MEDIATOR_HANDLE_LIST_OR_COMPONENT_UPDATED =
        "LayoutMediator.HandleListOrComponentUpdated";
inline constexpr const char* const LAYOUT_MEDIATOR_HANDLE_LAYOUT_VOLUNTARILY =
    "LayoutMediator.HandleLayoutVoluntarily";

inline constexpr const char* const LAYOUT_AFTER_ON_LIST_ELEMENT_UPDATED =
    "OnLayoutAfter.OnListElementUpdated";
inline constexpr const char* const LAYOUT_AFTER_ON_COMPONENT_FINISHED =
    "OnLayoutAfter.OnComponentFinished";
inline constexpr const char* const LAYOUT_AFTER_ON_LIST_ITEM_BATCH_FINISHED =
    "OnLayoutAfter.OnListItemBatchFinished";
inline constexpr const char* const LAYOUT_AFTER_APPEND_OPTIONS_FOR_TIMING =
    "OnLayoutAfter.EnqueueOperation.AppendOptionsForTiming";
inline constexpr const char* const PAINTING_CONTEXT_APPEND_OPTIONS_FOR_TIMING =
    "PaintingContext.AppendOptionsForTiming";
inline constexpr const char* const LYNX_ENGINE_UPDATE_VIEWPORT =
    "LynxEngine.UpdateViewport";
inline constexpr const char* const
    CALL_JS_FUNCTION_CONVERT_VALUE_TO_PIPER_ARRAY =
        "CallJSFunction:ConvertValueToPiperArray";
inline constexpr const char* const CALL_JS_FUNCTION_FIRE =
    "CallJSFunction:Fire";
inline constexpr const char* const LYNX_SHELL_BUILDER_BUILD =
    "LynxShell::Create";
inline constexpr const char* const LYNX_SHELL_DESTRUCTOR =
    "LynxShell::~LynxShell";
inline constexpr const char* const LYNX_SHELL_BUILDER_CREATE_ENGINE_ACTOR =
    "LynxShell::Create::CreateEngineActor";
inline constexpr const char* const LYNX_SHELL_INIT_RUNTIME =
    "LynxShell::InitRuntime";
inline constexpr const char* const LYNX_SHELL_START_JS_RUNTIME =
    "LynxShell::StartJsRuntime";
inline constexpr const char* const LYNX_SHELL_ATTACH_ENGINE_TO_UI_THREAD =
    "LynxShell::AttachEngineToUIThread";
inline constexpr const char* const LYNX_SHELL_DETACH_ENGINE_TO_UI_THREAD =
    "LynxShell::DetachEngineFromUIThread";
inline constexpr const char* const LYNX_SHELL_UPDATE_VIEWPORT =
    "LynxShell.UpdateViewport";
inline constexpr const char* const LYNX_SHELL_ENSURE_GLOBAL_PROPS_THREAD_SAFE =
    "LynxShell.EnsureGlobalPropsThreadSafe";

inline constexpr const char* const UI_OPERATION_ASYNC_QUEUE_FLUSH_ON_UI_THREAD =
    "LynxUIOperationAsyncQueue::FlushOnUIThread.";
inline constexpr const char* const UI_OPERATION_ASYNC_RENDER_FLUSH_WAIN_TASM =
    "UIOperationQueueAsyncRender::flush.waitTASM";
inline constexpr const char* const UI_OPERATION_ASYNC_RENDER_FLUSH_WAIN_LAYOUT =
    "UIOperationQueueAsyncRender::flush.waitLayout";
inline constexpr const char* const
    UI_OPERATION_ASYNC_QUEUE_FLUSH_ON_TASM_THREAD =
        "LynxUIOperationAsyncQueue::FlushOnTASMThread.";
inline constexpr const char* const
    UI_OPERATION_QUEUE_EXECUTE_HIGH_PRIORITY_OPERATION =
        "LynxUIOperationQueue::ExecuteHighPriorityOperation.";

inline constexpr const char* const TIMING_BIND_PIPELINE_ID_WITH_TIMING_FLAG =
    "Timing::BindPipelineIDWithTimingFlag";

inline constexpr const char* const TASM_MEDIATOR_CALL_JS_API_CALLBACK =
    "CallJSApiCallback";
inline constexpr const char* const
    TASM_MEDIATOR_CALL_JS_API_CALLBACK_WITH_VALUE =
        "CallJSApiCallbackWithValue";

inline constexpr const char* const TASM_OPERATION_QUEUE_ASYNC_FLUSH =
    "TASMOperationQueueAsync::Flush";
inline constexpr const char* const JSB_TIMING_FLUSH_JSB_TIMING =
    "JSBTiming::FlushJSBTiming";
inline constexpr const char* const
    CALL_JS_FUNCTION_JAVA_ONLY_ARRAY_TO_JS_ARRAY =
        "CallJSFunction:JavaOnlyArrayToJSArray";

inline constexpr const char* const CONVERT_NS_BINARY = "ConvertNSBinary";

inline constexpr const char* const NATIVE_FACADE_DARWIN_ON_DATA_UPDATED =
    "NativeFacadeDarwin::OnDataUpdated";
inline constexpr const char* const NATIVE_FACADE_DARWIN_ON_PAGE_CHANGED =
    "NativeFacadeDarwin::OnPageChanged";
inline constexpr const char* const
    NATIVE_FACADE_DARWIN_ON_TASM_FINISH_BY_NATIVE =
        "NativeFacadeDarwin::OnTasmFinishByNative";
inline constexpr const char* const NATIVE_FACADE_DARWIN_ON_TEMPLATE_LOADED =
    "NativeFacadeDarwin::OnTemplateLoaded";
inline constexpr const char* const
    NATIVE_FACADE_DARWIN_ON_SSR_HYDRATE_FINISHED =
        "NativeFacadeDarwin::OnSSRHydrateFinished";
inline constexpr const char* const NATIVE_FACADE_DARWIN_ON_RUNTIME_READY =
    "NativeFacadeDarwin::OnRuntimeReady";
#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_SHELL_COMMON_SHELL_TRACE_EVENT_DEF_H_
