// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <assert.h>

#include <sstream>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace tasm {

lepus::Value Utils::CreateLynx(lepus::Context* context,
                               const std::string& version) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {tasm::kGetTextInfo, &RendererFunctions::GetTextInfo, true, true},
      {kSetTimeout, &RendererFunctions::SetTimeout, true, true},
      {kClearTimeout, &RendererFunctions::ClearTimeout, true, true},
      {kSetInterval, &RendererFunctions::SetInterval, true, true},
      {kClearTimeInterval, &RendererFunctions::ClearTimeInterval, true, true},
      {kCFunctionTriggerLepusBridge, &RendererFunctions::TriggerLepusBridge, true, true},
      {kCFunctionTriggerLepusBridgeSync, &RendererFunctions::TriggerLepusBridgeSync, true, true},
      {kCFunctionTriggerComponentEvent, &RendererFunctions::TriggerComponentEvent, true, true},
      {runtime::kGetDevTool, &RendererFunctions::GetDevTool, true, true},
      {runtime::kGetCoreContext, &RendererFunctions::GetCoreContext, true, true},
      {runtime::kGetJSContext, &RendererFunctions::GetJSContext, true, true},
      {runtime::kGetUIContext, &RendererFunctions::GetUIContext, true, true},
      {runtime::kGetNative, &RendererFunctions::GetNative, true, true},
      {runtime::kGetEngine, &RendererFunctions::GetEngine, true, true},
      // Reserved to ensure compatibility.Use global's instead.
      {kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame, true, true},
      // Reserved to ensure compatibility.Use global's instead.
      {kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame, true, true},
      {runtime::kGetCustomSectionSync, &RendererFunctions::GetCustomSectionSync, true, true},
      // shared data.
      {tasm::kSetSessionStorageItem, &RendererFunctions::SetSessionStorageItem, true, true},
      {tasm::kGetSessionStorageItem, &RendererFunctions::GetSessionStorageItem, true, true},
      // reportError
      {runtime::kAddReporterCustomInfo, &RendererFunctions::LynxAddReporterCustomInfo, true, true},
      {kReportError, &RendererFunctions::ReportError, true, true},
      {kLoadScript, &RendererFunctions::LoadScript, true, true},
      {kFetchBundle, &RendererFunctions::FetchBundle, true, true},
      {kGetModule, &RendererFunctions::GetModule, true, true},
      // exposure
      {tasm::kStopExposure, &RendererFunctions::StopExposure, true, true},
      {tasm::kResumeExposure, &RendererFunctions::ResumeExposure, true, true},
  };
  // clang-format on

  auto lynx = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(lynx, funcs,
                                  sizeof(funcs) / sizeof(funcs[0]));

  if (!version.empty()) {
    lynx.SetProperty(BASE_STATIC_STRING(runtime::kTargetSdkVersion),
                     lepus::Value(version));
  }
  lynx.SetProperty(BASE_STATIC_STRING(runtime::kPerformanceObject),
                   CreateLynxPerformance(context));
  return lynx;
}

lepus::Value Utils::CreateLynxPerformance(lepus::Context* context) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {runtime::kGeneratePipelineOptions, &RendererFunctions::GeneratePipelineOptions, true, true},
      {runtime::kOnPipelineStart, &RendererFunctions::OnPipelineStart, true, true},
      {runtime::kMarkTiming, &RendererFunctions::MarkTiming, true, true},
      {runtime::kBindPipelineIDWithTimingFlag, &RendererFunctions::BindPipelineIDWithTimingFlag, true, true},
      {runtime::kAddTimingListener, &RendererFunctions::AddTimingListener, true, true},
      {runtime::kProfileStart, &RendererFunctions::ProfileStart, true, true},
      {runtime::kProfileEnd, &RendererFunctions::ProfileEnd, true, true},
      {runtime::kProfileMark, &RendererFunctions::ProfileMark, true, true},
      {runtime::kProfileFlowId, &RendererFunctions::ProfileFlowId, true, true},
      {runtime::kIsProfileRecording, &RendererFunctions::IsProfileRecording, true, true},
  };
  // clang-format on

  auto perf = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(perf, funcs,
                                  sizeof(funcs) / sizeof(funcs[0]));
  return perf;
}

lepus::Value Utils::CreateResponseHandler(lepus::Context* context,
                                          const lepus::Value& handler_impl) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {runtime::kWait, &RendererFunctions::WaitingForResponse, true, true},
      {runtime::kThen, &RendererFunctions::AddListenerForResponse, true, true},
  };
  // clang-format on

  auto handler = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(handler, funcs,
                                  sizeof(funcs) / sizeof(funcs[0]));
  handler.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy),
                      handler_impl);
  return handler;
}

lepus::Value Utils::CreateContextProxy(lepus::Context* context,
                                       runtime::ContextProxy::Type type,
                                       const lepus::Value& proxy_impl) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {runtime::kPostMessage, &RendererFunctions::PostMessage, true, true},
      {runtime::kDispatchEvent, &RendererFunctions::DispatchEvent, true, true},
      {runtime::kAddEventListener, &RendererFunctions::RuntimeAddEventListener, true, true},
      {runtime::kRemoveEventListener, &RendererFunctions::RuntimeRemoveEventListener, true, true},
  };

  constexpr const static lepus::RenderBindingFunction devtool_funcs[] = {
      {runtime::kReplaceStyleSheetByIdWithBase64, &RendererFunctions::ReplaceStyleSheetByIdWithBase64, true, true},
      {runtime::kRemoveStyleSheetById, &RendererFunctions::RemoveStyleSheetById, true, true},
  };
  // clang-format on

  auto proxy = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(proxy, funcs,
                                  sizeof(funcs) / sizeof(funcs[0]));
  if (type == runtime::ContextProxy::Type::kDevTool) {
    context->RegisterObjectFunction(
        proxy, devtool_funcs, sizeof(devtool_funcs) / sizeof(devtool_funcs[0]));
  }
  proxy.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy),
                    proxy_impl);
  return proxy;
}

lepus::Value Utils::CreateGestureManager(lepus::Context* context) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {tasm::kCFuncSetGestureState, &RendererFunctions::FiberSetGestureState, true, true},
      {tasm::kCFuncConsumeGesture, &RendererFunctions::FiberConsumeGesture, true, true},
  };
  // clang-format on

  auto manager = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(manager, funcs,
                                  sizeof(funcs) / sizeof(funcs[0]));
  return manager;
}

lepus::Value Utils::CreateLepusModule(lepus::Context* context,
                                      const lepus::Value& module_impl) {
  // clang-format off
  constexpr const static lepus::RenderBindingFunction funcs[] = {
      {runtime::kInvoke, &RendererFunctions::InvokeModuleMethod, true, true},
  };
  // clang-format on

  auto obj = lepus::LEPUSValueHelper::CreateObject(context);
  context->RegisterObjectFunction(obj, funcs, sizeof(funcs) / sizeof(funcs[0]));
  obj.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy), module_impl);
  return obj;
}

lepus::Value Renderer::SlotFunction(lepus::Context* context, lepus::Value*,
                                    int size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SLOT_FUNCTION);
  return lepus::Value();
}

void Renderer::RegisterBuiltin(lepus::Context* context, ArchOption option) {
  int32_t size = 0;
  const lepus::RenderBindingFunction* funcs = nullptr;
  switch (option) {
    case ArchOption::FIBER_ARCH:
      funcs = GetBuiltinFunctionsForFiber(size);
      break;
    default:
      funcs = GetBuiltinFunctionsForRadon(size);
  }
  context->RegisterGlobalFunction(funcs, size);
}

const lepus::RenderBindingFunction* Renderer::GetBuiltinFunctionsForRadon(
    int32_t& size) {
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  // clang-format off
  constexpr const static lepus::RenderBindingFunction kFuncs[] = {
      /* NO-ID */ {kCFuncIndexOf, &RendererFunctions::IndexOf, true, true},
      /* NO-ID */ {kCFuncGetLength, &RendererFunctions::GetLength, true, true},
      /* NO-ID */ {kCFuncSetValueToMap, &RendererFunctions::SetValueToMap, true, true},
      /* 001 */ {kCFuncCreatePage, &RendererFunctions::CreateVirtualPage, true, true},
      /* 002 */ {kCFuncAttachPage, &RendererFunctions::AttachPage, true, true},
      /* 003 */ {kCFuncCreateVirtualComponent, &RendererFunctions::CreateVirtualComponent, true, true},
      /* 004 */ {kCFuncCreateVirtualNode, &RendererFunctions::CreateVirtualNode, true, true},
      /* 005 */ {kCFuncAppendChild, &RendererFunctions::AppendChild, true, true},
      /* 006 */ {kCFuncSetClassTo, &RendererFunctions::SetClassTo, true, true},
      /* 007 */ {kCFuncSetStyleTo, &RendererFunctions::SetStyleTo, true, true},
      /* 008 */ {kCFuncSetEventTo, &RendererFunctions::SetEventTo, true, true},
      /* 009 */ {kCFuncSetAttributeTo, &RendererFunctions::SetAttributeTo, true, true},
      /* 010 */ {kCFuncSetStaticClassTo, &RendererFunctions::SetStaticClassTo, true, true},
      /* 011 */ {kCFuncSetStaticStyleTo, &RendererFunctions::SetStaticStyleTo, true, true},
      /* 012 */ {kCFuncSetStaticAttributeTo, &RendererFunctions::SetStaticAttrTo, true, true},
      /* 013 */ {kCFuncSetDataSetTo, &RendererFunctions::SetDataSetTo, true, true},
      /* 014 */ {kCFuncSetStaticEventTo, &RendererFunctions::SetStaticEventTo, true, true},
      /* 015 */ {kCFuncSetId, &RendererFunctions::SetId, true, true},
      /* 016 */ {kCFuncCreateVirtualSlot, &RendererFunctions::CreateSlot, true, true},
      /* 017 */ {kCFuncCreateVirtualPlug, &RendererFunctions::CreateVirtualPlug, true, true},
      /* 018 */ {kCFuncMarkComponentHasRenderer, &RendererFunctions::MarkComponentHasRenderer, true, true},
      /* 019 */ {kCFuncSetProp, &RendererFunctions::SetProp, true, true},
      /* 020 */ {kCFuncSetData, &RendererFunctions::SetData, true, true},
      /* 021 */ {kCFuncAddPlugToComponent, &RendererFunctions::AddVirtualPlugToComponent, true, true},
      /* 022 */ {kCFuncGetComponentData, &RendererFunctions::GetComponentData, true, true},
      /* 023 */ {kCFuncGetComponentProps, &RendererFunctions::GetComponentProps, true, true},
      /* 024 */ {kCFuncSetDynamicStyleTo, &RendererFunctions::SetDynamicStyleTo, true, true},
      /* 025 */ {kCFuncGetLazyLoadCount, &RendererFunctions::ThemedTranslationLegacy, true, true},
      /* 026 */ {kCFuncUpdateComponentInfo, &RendererFunctions::UpdateComponentInfo, true, true},
      /* 027 */ {kCFuncGetComponentInfo, &RendererFunctions::GetComponentInfo, true, true},
      /* 028 */ {kCFuncCreateVirtualListNode, &RendererFunctions::CreateVirtualListNode, true, true},
      /* 029 */ {kCFuncAppendListComponentInfo,&RendererFunctions::AppendListComponentInfo, true, true},
      /* 030 */ {kCFuncSetListRefreshComponentInfo, &SlotFunction, true, true},
      /* 031 */ {kCFuncCreateVirtualComponentByName, &RendererFunctions::CreateComponentByName, true, true},
      /* 032 */ {kCFuncCreateDynamicVirtualComponent, &RendererFunctions::CreateDynamicVirtualComponent, true, true},
      /* 033 */ {kCFuncRenderDynamicComponent, &RendererFunctions::RenderDynamicComponent, true, true},
      /* 034 */ {kCFuncThemedTranslation, &RendererFunctions::ThemedTranslation, true, true},
      /* 035 */ {kCFuncRegisterDataProcessor, &RendererFunctions::RegisterDataProcessor, true, true},
      /* 036 */ {kCFuncThemedLangTranslation, &RendererFunctions::ThemedLanguageTranslation, true, true},
      /* 037 */ {kCFuncGetComponentContextData, &RendererFunctions::GetComponentContextData, true, true},
      /* 038 */ {kCFuncProcessComponentData, &RendererFunctions::ProcessComponentData, true, true},
      /* 039 */ {"__slot__39", &SlotFunction, true, false},
      /* 040 */ {"__slot__40", &SlotFunction, true, false},
      /* 041 */ {"__slot__41", &SlotFunction, true, false},
      /* 042 */ {"__slot__42", &SlotFunction, true, false},
      /* 043 */ {"__slot__43", &SlotFunction, true, false},
      /* 044 */ {"__slot__44", &SlotFunction, true, false},
      /* 045 */ {"__slot__45", &SlotFunction, true, false},
      /* 046 */ {"__slot__46", &SlotFunction, true, false},
      /* 047 */ {"__slot__47", &SlotFunction, true, false},
      /* 048 */ {"__slot__48", &SlotFunction, true, false},
      /* 049 */ {"__slot__49", &SlotFunction, true, false},
      /* 050 */ {"__slot__50", &SlotFunction, true, false},
      /* 051 */ {"__slot__51", &SlotFunction, true, false},
      /* 052 */ {"__slot__51_1", &SlotFunction, true, false},
      /* 053 */ {"__slot__52", &SlotFunction, true, false},
      /* 054 */ {"__slot__53", &SlotFunction, true, false},
      /* 055 */ {"__slot__54", &SlotFunction, true, false},
      /* 056 */ {"__slot__55", &SlotFunction, true, false},
      /* 057 */ {"__slot__56", &SlotFunction, true, false},
      /* 058 */ {"__slot__57", &SlotFunction, true, false},
      /* 059 */ {"__slot__58", &SlotFunction, true, false},
      /* 060 */ {"__slot__59", &SlotFunction, true, false},
      /* 061 */ {"__slot__60", &SlotFunction, true, false},
      /* 062 */ {"__slot__61", &SlotFunction, true, false},
      /* 063 */ {"__slot__62", &SlotFunction, true, false},
      /* 064 */ {"__slot__63", &SlotFunction, true, false},
      /* 065 */ {"__slot__64", &SlotFunction, true, false},
      /* 066 */ {"__slot__65", &SlotFunction, true, false},
      /* 067 */ {"__slot__66", &SlotFunction, true, false},
      /* 068 */ {"__slot__67", &SlotFunction, true, false},
      /* 069 */ {"__slot__68", &SlotFunction, true, false},
      /* 070 */ {"__slot__69", &SlotFunction, true, false},
      /* 071 */ {"__slot__70", &SlotFunction, true, false},
      /* 072 */ {"__slot__71", &SlotFunction, true, false},
      /* 073 */ {"__slot__72", &SlotFunction, true, false},
      /* 074 */ {"__slot__73", &SlotFunction, true, false},
      /* 075 */ {"__slot__74", &SlotFunction, true, false},
      /* 076 */ {kCFuncSetStaticStyleToByFiber, &RendererFunctions::SetStaticStyleTo2, true, true},
      /* 077 */ {"__slot__76", &SlotFunction, true, false},
      /* 078 */ {"__slot__77", &SlotFunction, true, false},
      /* 079 */ {"__slot__78", &SlotFunction, true, false},
      /* 080 */ {"__slot__79", &SlotFunction, true, false},
      /* 081 */ {"__slot__80", &SlotFunction, true, false},
      /* 082 */ {kCFuncSetContextData, &RendererFunctions::SetContextData, true, true},
      /* 083 */ {kCFuncSetScriptEventTo, &RendererFunctions::SetScriptEventTo, true, true},
      /* 084 */ {kCFuncRegisterElementWorklet, &RendererFunctions::RegisterElementWorklet, true, true},
      /* 085 */ {kCFuncCreateVirtualPlugWithComponent, &RendererFunctions::CreateVirtualPlugWithComponent, true, true},
      /* 086 */ {"__slot__85", &SlotFunction, true, false},
      /* 087 */ {kCFuncAddEventListener, &RendererFunctions::AddEventListener, true, true},
      /* 088 */ {kCFuncI18nResourceTranslation, &RendererFunctions::I18nResourceTranslation, true, true},
      /* 089 */ {kCFuncReFlushPage, &RendererFunctions::ReFlushPage, true, true},
      /* 090 */ {kCFuncSetComponent, &RendererFunctions::SetComponent, true, true},
      /* 091 */ {kCFuncGetGlobalProps, &RendererFunctions::GetGlobalProps, true, true},
      /* 092 */ {"__slot__91", &SlotFunction, true, false},
      /* 093 */ {kCFuncAppendSubTree, &RendererFunctions::AppendSubTree, true, true},
      /* 094 */ {kCFuncHandleExceptionInLepus, &RendererFunctions::HandleExceptionInLepus, true, true},
      /* 095 */ {kCFuncAppendVirtualPlugToComponent, &RendererFunctions::AppendVirtualPlugToComponent, true, true},
      /* 096 */ {kCFuncMarkPageElement, &RendererFunctions::MarkPageElement, true, true},
      /* 097 */ {kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource, true, true},
      /* 098 */ {kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent, true, true},
      /* 099 */ {kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease, true, true},
      /* 100 */ {kCFuncCloneSubTree, &RendererFunctions::CloneSubTree, true, true},
      /* 101 */ {kCFuncGetSystemInfo, &RendererFunctions::GetSystemInfo, true, true},
      /* 102 */ {kCFuncAddFallbackToDynamicComponent, &RendererFunctions::AddFallbackToDynamicComponent, true, true},
      /* 103 */ {kCFuncCreateGestureDetector, &RendererFunctions::CreateGestureDetector, true, true},
      /* 104 */ {kCFunctionElementAnimate, &RendererFunctions::ElementAnimate, true, true},
      /* 105 */ {kCFuncSetStaticStyleTo2, &RendererFunctions::SetStaticStyleTo2, true, true},
      /* 106 */ {kSetTimeout, &RendererFunctions::SetTimeout, true, true},
      /* 107 */ {kClearTimeout, &RendererFunctions::ClearTimeout, true, true},
      /* 108 */ {kSetInterval, &RendererFunctions::SetInterval, true, true},
      /* 109 */ {kClearTimeInterval, &RendererFunctions::ClearTimeInterval, true, true},
      /* 110 */ {kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame, true, true},
      /* 111 */ {kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame, true, true},
  };
  // clang-format on
  size = sizeof(kFuncs) / sizeof(kFuncs[0]);
  return kFuncs;
}

const lepus::RenderBindingFunction* Renderer::GetBuiltinFunctionsForFiber(
    int32_t& size) {
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  // clang-format off
  constexpr const static lepus::RenderBindingFunction kFuncs[] = {
    /* NO-ID */ {kCFuncIndexOf, &RendererFunctions::IndexOf, true, true},
    /* NO-ID */ {kCFuncGetLength, &RendererFunctions::GetLength, true, true},
    /* NO-ID */ {kCFuncSetValueToMap, &RendererFunctions::SetValueToMap, true, true},
    /* 001 */ {kCFunctionCreateElement, &RendererFunctions::FiberCreateElement, true, true},
    /* 002 */ {kCFunctionCreatePage, &RendererFunctions::FiberCreatePage, true, true},
    /* 003 */ {kCFunctionCreateComponent, &RendererFunctions::FiberCreateComponent, true, true},
    /* 004 */ {kCFunctionCreateView, &RendererFunctions::FiberCreateView, true, true},
    /* 005 */ {kCFunctionCreateList, &RendererFunctions::FiberCreateList, true, true},
    /* 006 */ {kCFunctionCreateScrollView, &RendererFunctions::FiberCreateScrollView, true, true},
    /* 007 */ {kCFunctionCreateText, &RendererFunctions::FiberCreateText, true, true},
    /* 008 */ {kCFunctionCreateImage, &RendererFunctions::FiberCreateImage, true, true},
    /* 009 */ {kCFunctionCreateRawText, &RendererFunctions::FiberCreateRawText, true, true},
    /* 010 */ {kCFunctionCreateNonElement, &RendererFunctions::FiberCreateNonElement, true, true},
    /* 011 */ {kCFunctionCreateWrapperElement, &RendererFunctions::FiberCreateWrapperElement, true, true},
    /* 012 */ {kCFunctionAppendElement, &RendererFunctions::FiberAppendElement, true, true},
    /* 013 */ {kCFunctionRemoveElement, &RendererFunctions::FiberRemoveElement, true, true},
    /* 014 */ {kCFunctionInsertElementBefore, &RendererFunctions::FiberInsertElementBefore, true, true},
    /* 015 */ {kCFunctionFirstElement, &RendererFunctions::FiberFirstElement, true, true},
    /* 016 */ {kCFunctionLastElement, &RendererFunctions::FiberLastElement, true, true},
    /* 017 */ {kCFunctionNextElement, &RendererFunctions::FiberNextElement, true, true},
    /* 018 */ {kCFunctionReplaceElement, &RendererFunctions::FiberReplaceElement, true, true},
    /* 019 */ {kCFunctionSwapElement, &RendererFunctions::FiberSwapElement, true, true},
    /* 020 */ {kCFunctionGetParent, &RendererFunctions::FiberGetParent, true, true},
    /* 021 */ {kCFunctionGetChildren, &RendererFunctions::FiberGetChildren, true, true},
    /* 022 */ {kCFunctionCloneElement, &RendererFunctions::FiberCloneElement, true, true},
    /* 023 */ {kCFunctionElementIsEqual, &RendererFunctions::FiberElementIsEqual, true, true},
    /* 024 */ {kCFunctionGetElementUniqueID, &RendererFunctions::FiberGetElementUniqueID, true, true},
    /* 025 */ {kCFunctionGetTag, &RendererFunctions::FiberGetTag, true, true},
    /* 026 */ {kCFunctionSetAttribute, &RendererFunctions::FiberSetAttribute, true, true},
    /* 027 */ {kCFunctionGetAttributes, &RendererFunctions::FiberGetAttributes, true, true},
    /* 028 */ {kCFunctionAddClass, &RendererFunctions::FiberAddClass, true, true},
    /* 029 */ {kCFunctionSetClasses, &RendererFunctions::FiberSetClasses, true, true},
    /* 030 */ {kCFunctionGetClasses, &RendererFunctions::FiberGetClasses, true, true},
    /* 031 */ {kCFunctionAddInlineStyle, &RendererFunctions::FiberAddInlineStyle, true, true},
    /* 032 */ {kCFunctionSetInlineStyles, &RendererFunctions::FiberSetInlineStyles, true, true},
    /* 033 */ {kCFunctionGetInlineStyles, &RendererFunctions::FiberGetInlineStyles, true, true},
    /* 034 */ {kCFunctionSetParsedStyles, &RendererFunctions::FiberSetParsedStyles, true, true},
    /* 035 */ {kCFunctionGetComputedStyles, &RendererFunctions::FiberGetComputedStyles, true, true},
    /* 036 */ {kCFunctionAddEvent, &RendererFunctions::FiberAddEvent, true, true},
    /* 037 */ {kCFunctionSetEvents, &RendererFunctions::FiberSetEvents, true, true},
    /* 038 */ {kCFunctionGetEvent, &RendererFunctions::FiberGetEvent, true, true},
    /* 039 */ {kCFunctionGetEvents, &RendererFunctions::FiberGetEvents, true, true},
    /* 040 */ {kCFunctionSetID, &RendererFunctions::FiberSetID, true, true},
    /* 041 */ {kCFunctionGetID, &RendererFunctions::FiberGetID, true, true},
    /* 042 */ {kCFunctionAddDataset, &RendererFunctions::FiberAddDataset, true, true},
    /* 043 */ {kCFunctionSetDataset, &RendererFunctions::FiberSetDataset, true, true},
    /* 044 */ {kCFunctionGetDataset, &RendererFunctions::FiberGetDataset, true, true},
    /* 045 */ {kCFunctionGetComponentID, &RendererFunctions::FiberGetComponentID, true, true},
    /* 046 */ {kCFunctionUpdateComponentID, &RendererFunctions::FiberUpdateComponentID, true, true},
    /* 047 */ {kCFunctionElementFromBinary, &RendererFunctions::FiberElementFromBinary, true, true},
    /* 048 */ {kCFunctionElementFromBinaryAsync, &RendererFunctions::FiberElementFromBinaryAsync, true, true},
    /* 049 */ {kCFunctionUpdateListCallbacks, &RendererFunctions::FiberUpdateListCallbacks, true, true},
    /* 050 */ {kCFunctionFlushElementTree, &RendererFunctions::FiberFlushElementTree, true, true},
    /* 051 */ {kCFunctionOnLifecycleEvent, &RendererFunctions::FiberOnLifecycleEvent, true, true},
    /* 052 */ {kCFunctionQueryComponent, &RendererFunctions::FiberQueryComponent, true, true},
    /* 053 */ {kCFunctionSetCSSId, &RendererFunctions::FiberSetCSSId, true, true},
    /* 054 */ {kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease, true, true},
    /* 055 */ {kCFuncAddEventListener, &RendererFunctions::AddEventListener, true, true},
    /* 056 */ {kCFuncI18nResourceTranslation, &RendererFunctions::I18nResourceTranslation, true, true},
    /* 057 */ {kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource, true, true},
    /* 058 */ {kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent, true, true},
    /* 059 */ {kCFunctionReportError, &RendererFunctions::ReportError, true, true},
    /* 060 */ {kCFunctionGetDataByKey, &RendererFunctions::FiberGetDataByKey, true, true},
    /* 061 */ {kCFunctionReplaceElements, &RendererFunctions::FiberReplaceElements, true, true},
    /* 062 */ {kCFunctionQuerySelector, &RendererFunctions::FiberQuerySelector, true, true},
    /* 063 */ {kCFunctionQuerySelectorAll, &RendererFunctions::FiberQuerySelectorAll, true, true},
    /* 064 */ {kCFunctionSetLepusInitData, &RendererFunctions::FiberSetLepusInitData, true, true},
    /* 065 */ {kCFunctionAddConfig, &RendererFunctions::FiberAddConfig, true, true},
    /* 066 */ {kCFunctionSetConfig, &RendererFunctions::FiberSetConfig, true, true},
    /* 067 */ {kCFunctionUpdateComponentInfo, &RendererFunctions::FiberUpdateComponentInfo, true, true},
    /* 068 */ {kCFunctionGetConfig, &RendererFunctions::FiberGetElementConfig, true, true},
    /* 069 */ {kCFunctionGetInlineStyle, &RendererFunctions::FiberGetInlineStyle, true, true},
    /* 070 */ {kCFuncSetGestureDetector, &RendererFunctions::FiberSetGestureDetector, true, true},
    /* 071 */ {kCFuncRemoveGestureDetector, &RendererFunctions::FiberRemoveGestureDetector, true, true},
    /* 072 */ {kCFunctionGetAttributeByName, &RendererFunctions::FiberGetAttributeByName, true, true},
    /* 073 */ {kCFunctionGetAttributeNames, &RendererFunctions::FiberGetAttributeNames, true, true},
    /* 074 */ {kCFunctionGetPageElement, &RendererFunctions::FiberGetPageElement, true, true},
    /* 075 */ {kCFunctionCreateIf, &RendererFunctions::FiberCreateIf, true, true},
    /* 076 */ {kCFunctionCreateFor, &RendererFunctions::FiberCreateFor, true, true},
    /* 077 */ {kCFunctionCreateBlock, &RendererFunctions::FiberCreateBlock, true, true},
    /* 078 */ {kCFunctionUpdateIfNodeIndex, &RendererFunctions::FiberUpdateIfNodeIndex, true, true},
    /* 079 */ {kCFunctionUpdateForChildCount, &RendererFunctions::FiberUpdateForChildCount, true, true},
    /* 080 */ {kCFunctionGetElementByUniqueID, &RendererFunctions::FiberGetElementByUniqueID, true, true},
    /* 081 */ {kCFunctionGetDiffData, &RendererFunctions::FiberGetDiffData, true, true},
    /* 082 */ {kCFunctionLoadLepusChunk, &RendererFunctions::LoadLepusChunk, true, true},
    /* 083 */ {kCFuncSetGestureState, &RendererFunctions::FiberSetGestureState, true, true},
    /* 084 */ {kCFunctionMarkTemplateElement, &RendererFunctions::FiberMarkTemplateElement, true, true},
    /* 085 */ {kCFunctionIsTemplateElement, &RendererFunctions::FiberIsTemplateElement, true, true},
    /* 086 */ {kCFunctionMarkPartElement, &RendererFunctions::FiberMarkPartElement, true, true},
    /* 087 */ {kCFunctionIsPartElement, &RendererFunctions::FiberIsPartElement, true, true},
    /* 088 */ {kCFunctionGetTemplateParts, &RendererFunctions::FiberGetTemplateParts, true, true},
    /* 089 */ {kCFunctionAsyncResolveElement, &RendererFunctions::FiberAsyncResolveElement, true, true},
    /* 090 */ {kCFuncConsumeGesture, &RendererFunctions::FiberConsumeGesture, true, true},
    /* 091 */ {kCFunctionCreateElementWithProperties, &RendererFunctions::FiberCreateElementWithProperties, true, true},
    /* 092 */ {kCFunctionCreateSignal, &RendererFunctions::FiberCreateSignal, true, true},
    /* 093 */ {kCFunctionWriteSignal, &RendererFunctions::FiberWriteSignal, true, true},
    /* 094 */ {kCFunctionReadSignal, &RendererFunctions::FiberReadSignal, true, true},
    /* 095 */ {kCFunctionCreateComputation, &RendererFunctions::FiberCreateComputation, true, true},
    /* 096 */ {kCFunctionCreateMemo, &RendererFunctions::FiberCreateMemo, true, true},
    /* 097 */ {kCFunctionCreateScope, &RendererFunctions::FiberCreateScope, true, true},
    /* 098 */ {kCFunctionGetScope, &RendererFunctions::FiberGetScope, true, true},
    /* 099 */ {kCFunctionCleanUp, &RendererFunctions::FiberCleanUp, true, true},
    /* 100 */ {kCFunctionOnCleanUp, &RendererFunctions::FiberOnCleanUp, true, true},
    /* 101 */ {kCFunctionUnTrack, &RendererFunctions::FiberUnTrack, true, true},
    /* 102 */ {kCFunctionCreateFrame, &RendererFunctions::FiberCreateFrame, true, true},
    /* 103 */ {kCFunctionRunUpdates, &RendererFunctions::FiberRunUpdates, true, true},
    /* 104 */ {kCFunctionCreateStyleObject, &RendererFunctions::CreateStyleObject, true, true},
    /* 105 */ {kCFunctionSetStyleObject, &RendererFunctions::SetStyleObject, true, true},
    /* 106 */ {kCFunctionUpdateStyleObject, &RendererFunctions::UpdateStyleObject, true, true},
    /* 107 */ {kCFunctionAsyncResolveSubtreeProperty, &RendererFunctions::FiberAsyncResolveSubtreeProperty, true, true},
    /* 108 */ {kCFunctionMarkAsyncFlushRoot, &RendererFunctions::FiberMarkAsyncResolveRoot, true, true},
    /* 109 */ {kCFunctionAddEventListener, &RendererFunctions::FiberAddEventListener, true, true},
    /* 110 */ {kCFunctionFiberRemoveEventListener, &RendererFunctions::FiberRemoveEventListener, true, true},
    /* 111 */ {kCFunctionCreateEvent, &RendererFunctions::FiberCreateEvent, true, true},
    /* 112 */ {kCFunctionDispatchEvent, &RendererFunctions::FiberDispatchEvent, true, true},
    /* 113 */ {kCFunctionStopPropagation, &RendererFunctions::FiberStopPropagation, true, true},
    /* 114 */ {kCFunctionStopImmediatePropagation, &RendererFunctions::FiberStopImmediatePropagation, true, true},
    /* 115 */ {kCFunctionInvokeUIMethod, &RendererFunctions::InvokeUIMethod, true, true},
    /* 116 */ {kCFunctionGetComputedStyleByKey, &RendererFunctions::FiberGetComputedStyleByKey, true, true},
    /* 117 */ {kSetTimeout, &RendererFunctions::SetTimeout, true, true},
    /* 118 */ {kClearTimeout, &RendererFunctions::ClearTimeout, true, true},
    /* 119 */ {kSetInterval, &RendererFunctions::SetInterval, true, true},
    /* 120 */ {kClearTimeInterval, &RendererFunctions::ClearTimeInterval, true, true},
    /* 121 */ {kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame, true, true},
    /* 122 */ {kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame, true, true},
    /* 123 */ {kCFunctionElementAnimate, &RendererFunctions::ElementAnimate, true, true},
    /* 124 */ {kCFuncLoadStyleSheet, &RendererFunctions::LoadStyleSheet, true, true},
    /* 125 */ {kCFuncAdoptStyleSheet, &RendererFunctions::AdoptStyleSheet, true, true},
    /* 126 */ {kCFuncReplaceStyleSheets, &RendererFunctions::ReplaceStyleSheets, true, true},
  };
  // clang-format on
  size = sizeof(kFuncs) / sizeof(kFuncs[0]);
  return kFuncs;
}

}  // namespace tasm
}  // namespace lynx
