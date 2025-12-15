// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/renderer.h"

#include <assert.h>

#include <sstream>

#include "base/include/compiler_specific.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/runtime/vm/lepus/builtin.h"

namespace lynx {
namespace tasm {

#if defined(OS_WIN)
#ifdef SetProp
#undef SetProp
#endif  // SetProp
#endif  // OS_WIN

void Utils::RegisterMethodToLynx(lepus::Context* context, lepus::Value& lynx) {
  if (lynx.IsTable()) {
    auto lynx_table = lynx.Table();
    lepus::RegisterTableFunction(context, lynx_table, kGetTextInfo,
                                 &RendererFunctions::GetTextInfo);

    lepus::RegisterTableFunction(context, lynx_table, kSetTimeout,
                                 &RendererFunctions::SetTimeout);
    lepus::RegisterTableFunction(context, lynx_table, kClearTimeout,
                                 &RendererFunctions::ClearTimeout);
    lepus::RegisterTableFunction(context, lynx_table, kSetInterval,
                                 &RendererFunctions::SetInterval);
    lepus::RegisterTableFunction(context, lynx_table, kClearTimeInterval,
                                 &RendererFunctions::ClearTimeInterval);
    lepus::RegisterTableFunction(context, lynx_table,
                                 kCFunctionTriggerLepusBridge,
                                 &RendererFunctions::TriggerLepusBridge);
    lepus::RegisterTableFunction(context, lynx_table,
                                 kCFunctionTriggerComponentEvent,
                                 &RendererFunctions::TriggerComponentEvent);
    lepus::RegisterTableFunction(context, lynx_table,
                                 kCFunctionTriggerLepusBridgeSync,
                                 &RendererFunctions::TriggerLepusBridgeSync);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetDevTool,
                                 &RendererFunctions::GetDevTool);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetCoreContext,
                                 &RendererFunctions::GetCoreContext);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetJSContext,
                                 &RendererFunctions::GetJSContext);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetUIContext,
                                 &RendererFunctions::GetUIContext);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetNative,
                                 &RendererFunctions::GetNative);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetEngine,
                                 &RendererFunctions::GetEngine);
    lepus::RegisterTableFunction(context, lynx_table, kRequestAnimationFrame,
                                 &RendererFunctions::RequestAnimationFrame);
    lepus::RegisterTableFunction(context, lynx_table, kCancelAnimationFrame,
                                 &RendererFunctions::CancelAnimationFrame);
    lepus::RegisterTableFunction(context, lynx_table,
                                 runtime::kGetCustomSectionSync,
                                 &RendererFunctions::GetCustomSectionSync);
    lepus::RegisterTableFunction(context, lynx_table, kSetSessionStorageItem,
                                 &RendererFunctions::SetSessionStorageItem);
    lepus::RegisterTableFunction(context, lynx_table, kGetSessionStorageItem,
                                 &RendererFunctions::GetSessionStorageItem);
    lepus::RegisterTableFunction(context, lynx_table, kLoadScript,
                                 &RendererFunctions::LoadScript);
    // Timing
    RegisterMethodToLynxPerformance(context, lynx);
    lepus::RegisterTableFunction(context, lynx_table, kFetchBundle,
                                 &RendererFunctions::FetchBundle);
    lepus::RegisterTableFunction(context, lynx_table,
                                 runtime::kAddReporterCustomInfo,
                                 &RendererFunctions::LynxAddReporterCustomInfo);
    // NativeModule GetModule Method
    lepus::RegisterTableFunction(context, lynx_table, kGetModule,
                                 &RendererFunctions::GetModule);
    // exposure
    lepus::RegisterTableFunction(context, lynx_table, kStopExposure,
                                 &RendererFunctions::StopExposure);
    lepus::RegisterTableFunction(context, lynx_table, kResumeExposure,
                                 &RendererFunctions::ResumeExposure);
  }
}

void Utils::RegisterMethodToLynxPerformance(lepus::Context* context,
                                            lepus::Value& lynx) {
  if (lynx.IsTable()) {
    lepus::Value perf_obj(lepus::LEPUSValueHelper::CreateObject(context));
    lynx.SetProperty(BASE_STATIC_STRING(runtime::kPerformanceObject), perf_obj);

    auto perf_table = perf_obj.Table();
    lepus::RegisterTableFunction(context, perf_table,
                                 runtime::kGeneratePipelineOptions,
                                 &RendererFunctions::GeneratePipelineOptions);
    lepus::RegisterTableFunction(context, perf_table, runtime::kOnPipelineStart,
                                 &RendererFunctions::OnPipelineStart);
    lepus::RegisterTableFunction(context, perf_table, runtime::kMarkTiming,
                                 &RendererFunctions::MarkTiming);
    lepus::RegisterTableFunction(
        context, perf_table, runtime::kBindPipelineIDWithTimingFlag,
        &RendererFunctions::BindPipelineIDWithTimingFlag);
    lepus::RegisterTableFunction(context, perf_table,
                                 runtime::kAddTimingListener,
                                 &RendererFunctions::AddTimingListener);

    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileStart,
                                 &RendererFunctions::ProfileStart);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileEnd,
                                 &RendererFunctions::ProfileEnd);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileMark,
                                 &RendererFunctions::ProfileMark);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileFlowId,
                                 &RendererFunctions::ProfileFlowId);
    lepus::RegisterTableFunction(context, perf_table,
                                 runtime::kIsProfileRecording,
                                 &RendererFunctions::IsProfileRecording);
  }
};

void Utils::RegisterMethodToResponseHandler(lepus::Context* context,
                                            lepus::Value& response_handler) {
  if (response_handler.IsTable()) {
    auto target_table = response_handler.Table();
    lepus::RegisterTableFunction(context, target_table, runtime::kWait,
                                 &RendererFunctions::WaitingForResponse);
    lepus::RegisterTableFunction(context, target_table, runtime::kThen,
                                 &RendererFunctions::AddListenerForResponse);
  }
}

void Utils::RegisterMethodToContextProxy(lepus::Context* context,
                                         lepus::Value& target,
                                         runtime::ContextProxy::Type type) {
  if (target.IsTable()) {
    auto target_table = target.Table();
    lepus::RegisterTableFunction(context, target_table, runtime::kPostMessage,
                                 &RendererFunctions::PostMessage);
    lepus::RegisterTableFunction(context, target_table, runtime::kDispatchEvent,
                                 &RendererFunctions::DispatchEvent);
    lepus::RegisterTableFunction(context, target_table,
                                 runtime::kAddEventListener,
                                 &RendererFunctions::RuntimeAddEventListener);
    lepus::RegisterTableFunction(
        context, target_table, runtime::kRemoveEventListener,
        &RendererFunctions::RuntimeRemoveEventListener);

    if (type == runtime::ContextProxy::Type::kDevTool) {
      RegisterTableFunction(
          context, target_table, runtime::kReplaceStyleSheetByIdWithBase64,
          &RendererFunctions::ReplaceStyleSheetByIdWithBase64);
      RegisterTableFunction(context, target_table,
                            runtime::kRemoveStyleSheetById,
                            &RendererFunctions::RemoveStyleSheetById);
    }
  }
}

void Utils::RegisterMethodToLepusModule(lepus::Context* context,
                                        lepus::Value& lepus_module) {
  if (lepus_module.IsTable()) {
    lepus::RegisterTableFunction(context, lepus_module.Table(),
                                 runtime::kInvoke,
                                 &RendererFunctions::InvokeModuleMethod);
  }
}

}  // namespace tasm
}  // namespace lynx
