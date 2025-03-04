// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <assert.h>

#include <sstream>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
#define CREATE_FUNCTION(name) \
  RENDERER_FUNCTION(name) { return LEPUS_UNDEFINED; }

#define RenderFatal(expression, ...) \
  LynxFatal(expression, LYNX_ERROR_CODE_DATA_BINDING, __VA_ARGS__)

#define CHECK_ARGC_EQ(name, count) \
  RenderFatal(argc == count, #name " params size should == " #count);

#define CHECK_ARGC_GE(name, count) \
  RenderFatal(argc >= count, #name " params size should == " #count);

void Utils::RegisterNGBuiltin(lepus::Context* context) {
  static const lepus::RenderBindingFunction funcs[] = {
      {kCFuncIndexOf, &RendererFunctions::IndexOf},
      {kCFuncGetLength, &RendererFunctions::GetLength},
      {kCFuncSetValueToMap, &RendererFunctions::SetValueToMap},
      // Added in Lynx:3.0
      {kSetTimeout, &RendererFunctions::SetTimeout},
      // Added in Lynx:3.0
      {kClearTimeout, &RendererFunctions::ClearTimeout},
      // Added in Lynx:3.0
      {kSetInterval, &RendererFunctions::SetInterval},
      // Added in Lynx:3.0
      {kClearTimeInterval, &RendererFunctions::ClearTimeInterval},
      // Added in Lynx:3.0
      {kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame},
      // Added in Lynx:3.0
      {kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame},
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
}

void Utils::RegisterNGMethodToLynx(lepus::Context* context, lepus::Value& lynx,
                                   const std::string& targetSdkVersion) {
  if (lynx.IsJSValue()) {
    const static lepus::RenderBindingFunction funcs[] = {
        {tasm::kGetTextInfo, &RendererFunctions::GetTextInfo},

        {kSetTimeout, &RendererFunctions::SetTimeout},
        {kClearTimeout, &RendererFunctions::ClearTimeout},
        {kSetInterval, &RendererFunctions::SetInterval},
        {kClearTimeInterval, &RendererFunctions::ClearTimeInterval},
        {kCFunctionTriggerLepusBridge, &RendererFunctions::TriggerLepusBridge},
        {kCFunctionTriggerLepusBridgeSync,
         &RendererFunctions::TriggerLepusBridgeSync},
        {kCFunctionTriggerComponentEvent,
         &RendererFunctions::TriggerComponentEvent},
        {runtime::kGetDevTool, &RendererFunctions::GetDevTool},
        {runtime::kGetCoreContext, &RendererFunctions::GetCoreContext},
        {runtime::kGetJSContext, &RendererFunctions::GetJSContext},
        {runtime::kGetUIContext, &RendererFunctions::GetUIContext},
        // Reserved to ensure compatibility.Use global's instead.
        {kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame},
        // Reserved to ensure compatibility.Use global's instead.
        {kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame},
        {runtime::kGetCustomSectionSync,
         &RendererFunctions::GetCustomSectionSync},
        // shared data.
        {tasm::kSetSessionStorageItem,
         &RendererFunctions::SetSessionStorageItem},
        {tasm::kGetSessionStorageItem,
         &RendererFunctions::GetSessionStorageItem},
        {runtime::kAddReporterCustomInfo,
         // reportError
         &RendererFunctions::LynxAddReporterCustomInfo},
        {kReportError, &RendererFunctions::ReportError},
    };
    lepus::RegisterObjectNGCFunction(context, lynx, funcs,
                                     sizeof(funcs) / sizeof(funcs[0]));
    // Timing
    RegisterNGMethodToLynxPerformance(context, lynx);

    // engine version
    if (!targetSdkVersion.empty()) {
      lynx.SetProperty(BASE_STATIC_STRING(runtime::kTargetSdkVersion),
                       lepus::Value(targetSdkVersion));
    }
  }
  return;
}

void Utils::RegisterNGMethodToLynxPerformance(lepus::Context* context,
                                              lepus::Value& lynx) {
  if (lynx.IsJSValue()) {
    lepus::Value perf_obj(lepus::Value::CreateObject(context));
    lynx.SetProperty(BASE_STATIC_STRING(runtime::kPerformanceObject), perf_obj);
    static const lepus::RenderBindingFunction funcs[] = {
        {runtime::kGeneratePipelineOptions,
         &RendererFunctions::GeneratePipelineOptions},
        {runtime::kOnPipelineStart, &RendererFunctions::OnPipelineStart},
        {runtime::kMarkTiming, &RendererFunctions::MarkTiming},
        {runtime::kBindPipelineIDWithTimingFlag,
         &RendererFunctions::BindPipelineIDWithTimingFlag},
        {runtime::kAddTimingListener, &RendererFunctions::AddTimingListener},
        {runtime::kProfileStart, &RendererFunctions::ProfileStart},
        {runtime::kProfileEnd, &RendererFunctions::ProfileEnd},
        {runtime::kProfileMark, &RendererFunctions::ProfileMark},
        {runtime::kProfileFlowId, &RendererFunctions::ProfileFlowId},
        {runtime::kIsProfileRecording, &RendererFunctions::IsProfileRecording},
    };
    lepus::RegisterObjectNGCFunction(context, perf_obj, funcs,
                                     sizeof(funcs) / sizeof(funcs[0]));
  }
}

void Utils::RegisterNGMethodToContextProxy(lepus::Context* context,
                                           lepus::Value& target,
                                           runtime::ContextProxy::Type type) {
  if (target.IsJSValue()) {
    const static lepus::RenderBindingFunction funcs[] = {
        {runtime::kPostMessage, &RendererFunctions::PostMessage},
        {runtime::kDispatchEvent, &RendererFunctions::DispatchEvent},
        {runtime::kAddEventListener,
         &RendererFunctions::RuntimeAddEventListener},
        {runtime::kRemoveEventListener,
         &RendererFunctions::RuntimeRemoveEventListener},
    };
    lepus::RegisterObjectNGCFunction(context, target, funcs,
                                     sizeof(funcs) / sizeof(funcs[0]));
    if (type == runtime::ContextProxy::Type::kDevTool) {
      const static lepus::RenderBindingFunction funcs[] = {
          {runtime::kReplaceStyleSheetByIdWithBase64,
           &RendererFunctions::ReplaceStyleSheetByIdWithBase64},
          {runtime::kRemoveStyleSheetById,
           &RendererFunctions::RemoveStyleSheetById},
      };
      lepus::RegisterObjectNGCFunction(context, target, funcs,
                                       sizeof(funcs) / sizeof(funcs[0]));
    }
  }
}

void Utils::RegisterNGMethodToGestureManager(lepus::Context* context,
                                             lepus::Value& gesture_manager) {
  // Check if gesture_manager is a JSValue
  if (gesture_manager.IsJSValue()) {
    const static lepus::RenderBindingFunction funcs[] = {
        {tasm::kCFuncSetGestureState, &RendererFunctions::FiberSetGestureState},
        {tasm::kCFuncConsumeGesture, &RendererFunctions::FiberConsumeGesture},
    };
    // Register function for setting gesture state
    lepus::RegisterObjectNGCFunction(context, gesture_manager, funcs,
                                     sizeof(funcs) / sizeof(funcs[0]));
  }
}

static lepus::Value SlotFunction(lepus::Context* context, lepus::Value* argv,
                                 int32_t argc) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SlotFunction");
  return lepus::Value();
}

void Renderer::RegisterNGBuiltin(lepus::Context* context, ArchOption option) {
  switch (option) {
    case ArchOption::FIBER_ARCH:
      RegisterNGBuiltinForFiber(context);
      break;
    case ArchOption::AIR_ARCH:
      RegisterNGBuiltinForAir(context);
      break;
    default:
      RegisterNGBuiltinForRadon(context);
  }
}

void Renderer::RegisterNGBuiltinForRadon(lepus::Context* context) {
  static const lepus::RenderBindingFunction funcs[] = {
      {kCFuncCreatePage, &RendererFunctions::CreateVirtualPage},
      {kCFuncAttachPage, &RendererFunctions::AttachPage},
      {kCFuncCreateVirtualComponent,
       &RendererFunctions::CreateVirtualComponent},
      {kCFuncCreateVirtualNode, &RendererFunctions::CreateVirtualNode},
      {kCFuncAppendChild, &RendererFunctions::AppendChild},
      {kCFuncAppendSubTree, &RendererFunctions::AppendSubTree},
      {kCFuncCloneSubTree, &RendererFunctions::CloneSubTree},
      {kCFuncSetClassTo, &RendererFunctions::SetClassTo},
      {kCFuncSetStyleTo, &RendererFunctions::SetStyleTo},
      {kCFuncSetEventTo, &RendererFunctions::SetEventTo},
      {kCFuncSetAttributeTo, &RendererFunctions::SetAttributeTo},
      {kCFuncSetStaticClassTo, &RendererFunctions::SetStaticClassTo},
      {kCFuncSetStaticStyleTo, &RendererFunctions::SetStaticStyleTo},
      {kCFuncSetStaticAttributeTo, &RendererFunctions::SetStaticAttrTo},
      {kCFuncSetDataSetTo, &RendererFunctions::SetDataSetTo},
      {kCFuncSetStaticEventTo, &RendererFunctions::SetStaticEventTo},
      {kCFuncSetId, &RendererFunctions::SetId},
      {kCFuncCreateVirtualSlot, &RendererFunctions::CreateSlot},
      {kCFuncCreateVirtualPlug, &RendererFunctions::CreateVirtualPlug},
      {kCFuncMarkComponentHasRenderer,
       &RendererFunctions::MarkComponentHasRenderer},
      {kCFuncSetProp, &RendererFunctions::SetProp},
      {kCFuncSetData, &RendererFunctions::SetData},
      {kCFuncAddPlugToComponent, &RendererFunctions::AddVirtualPlugToComponent},
      {kCFuncAppendVirtualPlugToComponent,
       &RendererFunctions::AppendVirtualPlugToComponent},
      {kCFuncGetComponentData, &RendererFunctions::GetComponentData},
      {kCFuncGetComponentProps, &RendererFunctions::GetComponentProps},
      {kCFuncSetDynamicStyleTo, &RendererFunctions::SetDynamicStyleTo},
      {kCFuncGetLazyLoadCount, &RendererFunctions::ThemedTranslationLegacy},
      {kCFuncUpdateComponentInfo, &RendererFunctions::UpdateComponentInfo},
      {kCFuncGetComponentInfo, &RendererFunctions::GetComponentInfo},
      {kCFuncCreateVirtualListNode, &RendererFunctions::CreateVirtualListNode},
      {kCFuncAppendListComponentInfo,
       &RendererFunctions::AppendListComponentInfo},
      {kCFuncSetListRefreshComponentInfo, &SlotFunction},
      {kCFuncCreateVirtualComponentByName,
       &RendererFunctions::CreateComponentByName},
      {kCFuncCreateDynamicVirtualComponent,
       &RendererFunctions::CreateDynamicVirtualComponent},
      {kCFuncRenderDynamicComponent,
       &RendererFunctions::RenderDynamicComponent},
      {kCFuncThemedTranslation, &RendererFunctions::ThemedTranslation},
      {kCFuncRegisterDataProcessor, &RendererFunctions::RegisterDataProcessor},
      {kCFuncThemedLangTranslation,
       &RendererFunctions::ThemedLanguageTranslation},
      {kCFuncGetComponentContextData,
       &RendererFunctions::GetComponentContextData},
      {kCFuncProcessComponentData, &RendererFunctions::ProcessComponentData},
      {kCFuncSetStaticStyleTo2, &RendererFunctions::SetStaticStyleTo2},
      {kCFuncSetStaticStyleToByFiber, &RendererFunctions::SetStaticStyleTo2},
      {kCFuncSetScriptEventTo, &RendererFunctions::SetScriptEventTo},
      {kCFuncRegisterElementWorklet,
       &RendererFunctions::RegisterElementWorklet},
      {kCFuncSetContextData, &RendererFunctions::SetContextData},
      {kCFuncCreateVirtualPlugWithComponent,
       &RendererFunctions::CreateVirtualPlugWithComponent},
      {kCFuncAddEventListener, &RendererFunctions::AddEventListener},
      {kCFuncI18nResourceTranslation,
       &RendererFunctions::I18nResourceTranslation},
      {kCFuncReFlushPage, &RendererFunctions::ReFlushPage},
      {kCFuncSetComponent, &RendererFunctions::SetComponent},
      {kCFuncGetGlobalProps, &RendererFunctions::GetGlobalProps},
      {kCFuncHandleExceptionInLepus,
       &RendererFunctions::HandleExceptionInLepus},
      {kCFuncMarkPageElement, &RendererFunctions::MarkPageElement},
      {kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource},
      {kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent},
      {kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease},
      {kCFuncGetSystemInfo, &RendererFunctions::GetSystemInfo},
      {kCFuncAddFallbackToDynamicComponent,
       &RendererFunctions::AddFallbackToDynamicComponent},
      {kCFuncCreateGestureDetector, &RendererFunctions::CreateGestureDetector},
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
}

void Renderer::RegisterNGBuiltinForFiber(lepus::Context* context) {
  static const lepus::RenderBindingFunction funcs[] = {
      /* Element API BEGIN */
      {kCFunctionCreateElement, &RendererFunctions::FiberCreateElement},
      {kCFunctionCreatePage, &RendererFunctions::FiberCreatePage},
      {kCFunctionGetPageElement, &RendererFunctions::FiberGetPageElement},
      {kCFunctionCreateComponent, &RendererFunctions::FiberCreateComponent},
      {kCFunctionCreateView, &RendererFunctions::FiberCreateView},
      {kCFunctionCreateList, &RendererFunctions::FiberCreateList},
      {kCFunctionCreateScrollView, &RendererFunctions::FiberCreateScrollView},
      {kCFunctionCreateText, &RendererFunctions::FiberCreateText},
      {kCFunctionCreateImage, &RendererFunctions::FiberCreateImage},
      {kCFunctionCreateRawText, &RendererFunctions::FiberCreateRawText},
      {kCFunctionCreateNonElement, &RendererFunctions::FiberCreateNonElement},
      {kCFunctionCreateWrapperElement,
       &RendererFunctions::FiberCreateWrapperElement},
      {kCFunctionAppendElement, &RendererFunctions::FiberAppendElement},
      {kCFunctionRemoveElement, &RendererFunctions::FiberRemoveElement},
      {kCFunctionInsertElementBefore,
       &RendererFunctions::FiberInsertElementBefore},
      {kCFunctionFirstElement, &RendererFunctions::FiberFirstElement},
      {kCFunctionLastElement, &RendererFunctions::FiberLastElement},
      {kCFunctionNextElement, &RendererFunctions::FiberNextElement},
      {kCFunctionReplaceElement, &RendererFunctions::FiberReplaceElement},
      {kCFunctionReplaceElements, &RendererFunctions::FiberReplaceElements},
      {kCFunctionSwapElement, &RendererFunctions::FiberSwapElement},
      {kCFunctionGetParent, &RendererFunctions::FiberGetParent},
      {kCFunctionGetChildren, &RendererFunctions::FiberGetChildren},
      {kCFunctionCloneElement, &RendererFunctions::FiberCloneElement},
      {kCFunctionMarkTemplateElement,
       &RendererFunctions::FiberMarkTemplateElement},
      {kCFunctionIsTemplateElement, &RendererFunctions::FiberIsTemplateElement},
      {kCFunctionMarkPartElement, &RendererFunctions::FiberMarkPartElement},
      {kCFunctionIsPartElement, &RendererFunctions::FiberIsPartElement},
      {kCFunctionGetTemplateParts, &RendererFunctions::FiberGetTemplateParts},
      {kCFunctionElementIsEqual, &RendererFunctions::FiberElementIsEqual},
      {kCFunctionGetElementUniqueID,
       &RendererFunctions::FiberGetElementUniqueID},
      {kCFunctionAddConfig, &RendererFunctions::FiberAddConfig},
      {kCFunctionSetConfig, &RendererFunctions::FiberSetConfig},
      {kCFunctionGetConfig, &RendererFunctions::FiberGetElementConfig},
      {kCFunctionGetTag, &RendererFunctions::FiberGetTag},
      {kCFunctionSetAttribute, &RendererFunctions::FiberSetAttribute},
      {kCFunctionGetAttributes, &RendererFunctions::FiberGetAttributes},
      {kCFunctionGetAttributeByName,
       &RendererFunctions::FiberGetAttributeByName},
      {kCFunctionGetAttributeNames, &RendererFunctions::FiberGetAttributeNames},
      {kCFunctionAddClass, &RendererFunctions::FiberAddClass},
      {kCFunctionSetClasses, &RendererFunctions::FiberSetClasses},
      {kCFunctionGetClasses, &RendererFunctions::FiberGetClasses},
      {kCFunctionAddInlineStyle, &RendererFunctions::FiberAddInlineStyle},
      {kCFunctionSetInlineStyles, &RendererFunctions::FiberSetInlineStyles},
      {kCFunctionGetInlineStyles, &RendererFunctions::FiberGetInlineStyles},
      {kCFunctionGetInlineStyle, &RendererFunctions::FiberGetInlineStyle},
      {kCFunctionSetParsedStyles, &RendererFunctions::FiberSetParsedStyles},
      {kCFunctionGetComputedStyles, &RendererFunctions::FiberGetComputedStyles},
      {kCFunctionAddEvent, &RendererFunctions::FiberAddEvent},
      {kCFunctionSetEvents, &RendererFunctions::FiberSetEvents},
      {kCFunctionGetEvent, &RendererFunctions::FiberGetEvent},
      {kCFunctionGetEvents, &RendererFunctions::FiberGetEvents},
      {kCFunctionSetID, &RendererFunctions::FiberSetID},
      {kCFunctionGetID, &RendererFunctions::FiberGetID},
      {kCFunctionAddDataset, &RendererFunctions::FiberAddDataset},
      {kCFunctionSetDataset, &RendererFunctions::FiberSetDataset},
      {kCFunctionGetDataset, &RendererFunctions::FiberGetDataset},
      {kCFunctionGetDataByKey, &RendererFunctions::FiberGetDataByKey},
      {kCFunctionGetComponentID, &RendererFunctions::FiberGetComponentID},
      {kCFunctionUpdateComponentID, &RendererFunctions::FiberUpdateComponentID},
      {kCFunctionUpdateComponentInfo,
       &RendererFunctions::FiberUpdateComponentInfo},
      {kCFunctionUpdateListCallbacks,
       &RendererFunctions::FiberUpdateListCallbacks},
      {kCFunctionFlushElementTree, &RendererFunctions::FiberFlushElementTree},
      {kCFunctionOnLifecycleEvent, &RendererFunctions::FiberOnLifecycleEvent},
      {kCFunctionElementFromBinary, &RendererFunctions::FiberElementFromBinary},
      {kCFunctionElementFromBinaryAsync,
       &RendererFunctions::FiberElementFromBinaryAsync},
      {kCFunctionQueryComponent, &RendererFunctions::FiberQueryComponent},
      {kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease},
      {kCFunctionSetCSSId, &RendererFunctions::FiberSetCSSId},
      {kCFuncAddEventListener, &RendererFunctions::AddEventListener},
      {kCFuncI18nResourceTranslation,
       &RendererFunctions::I18nResourceTranslation},
      {kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource},
      {kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent},
      {kCFunctionReportError, &RendererFunctions::ReportError},

      {kCFunctionQuerySelector, &RendererFunctions::FiberQuerySelector},
      {kCFunctionQuerySelectorAll, &RendererFunctions::FiberQuerySelectorAll},
      {kCFunctionSetLepusInitData, &RendererFunctions::FiberSetLepusInitData},
      {kCFuncSetGestureDetector, &RendererFunctions::FiberSetGestureDetector},
      {kCFuncRemoveGestureDetector,
       &RendererFunctions::FiberRemoveGestureDetector},
      {kCFuncSetGestureState, &RendererFunctions::FiberSetGestureState},
      {kCFuncConsumeGesture, &RendererFunctions::FiberConsumeGesture},

      {kCFunctionCreateIf, &RendererFunctions::FiberCreateIf},
      {kCFunctionCreateFor, &RendererFunctions::FiberCreateFor},
      {kCFunctionCreateBlock, &RendererFunctions::FiberCreateBlock},
      {kCFunctionUpdateIfNodeIndex, &RendererFunctions::FiberUpdateIfNodeIndex},
      {kCFunctionUpdateForChildCount,
       &RendererFunctions::FiberUpdateForChildCount},
      {kCFunctionGetElementByUniqueID,
       &RendererFunctions::FiberGetElementByUniqueID},
      {kCFunctionGetDiffData, &RendererFunctions::FiberGetDiffData},

      {kCFunctionLoadLepusChunk, &RendererFunctions::LoadLepusChunk},
      {kCFunctionInvokeUIMethod, &RendererFunctions::InvokeUIMethod},
      {kCFunctionAsyncResolveElement,
       &RendererFunctions::FiberAsyncResolveElement},
      {kCFunctionCreateElementWithProperties,
       &RendererFunctions::FiberCreateElementWithProperties},
      {kCFunctionCreateSignal, &RendererFunctions::FiberCreateSignal},
      {kCFunctionWriteSignal, &RendererFunctions::FiberWriteSignal},
      {kCFunctionReadSignal, &RendererFunctions::FiberReadSignal},
      {kCFunctionCreateComputation, &RendererFunctions::FiberCreateComputation},
      {kCFunctionCreateMemo, &RendererFunctions::FiberCreateMemo},
      {kCFunctionCreateScope, &RendererFunctions::FiberCreateScope},
      {kCFunctionGetScope, &RendererFunctions::FiberGetScope},
      {kCFunctionCleanUp, &RendererFunctions::FiberCleanUp},
      {kCFunctionOnCleanUp, &RendererFunctions::FiberOnCleanUp},
      {kCFunctionUnTrack, &RendererFunctions::FiberUnTrack},
      /* Element API END */
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
  return;
}

void Renderer::RegisterNGBuiltinForAir(lepus::Context* context) {
  static const lepus::RenderBindingFunction funcs[] = {
      {kCFunctionAirCreateElement, &RendererFunctions::AirCreateElement},
      {kCFunctionAirGetElement, &RendererFunctions::AirGetElement},
      {kCFunctionAirCreatePage, &RendererFunctions::AirCreatePage},
      {kCFunctionAirCreateComponent, &RendererFunctions::AirCreateComponent},
      {kCFunctionAirCreateBlock, &RendererFunctions::AirCreateBlock},
      {kCFunctionAirCreateIf, &RendererFunctions::AirCreateIf},
      {kCFunctionAirCreateRadonIf, &RendererFunctions::AirCreateRadonIf},
      {kCFunctionAirCreateFor, &RendererFunctions::AirCreateFor},
      {kCFunctionAirCreatePlug, &RendererFunctions::AirCreatePlug},
      {kCFunctionAirCreateSlot, &RendererFunctions::AirCreateSlot},
      {kCFunctionAirAppendElement, &RendererFunctions::AirAppendElement},
      {kCFunctionAirRemoveElement, &RendererFunctions::AirRemoveElement},
      {kCFunctionAirInsertElementBefore,
       &RendererFunctions::AirInsertElementBefore},
      {kCFunctionAirGetElementUniqueID,
       &RendererFunctions::AirGetElementUniqueID},
      {kCFunctionAirGetTag, &RendererFunctions::AirGetElementTag},
      {kCFunctionAirSetAttribute, &RendererFunctions::AirSetAttribute},
      {kCFunctionAirSetInlineStyles, &RendererFunctions::AirSetInlineStyles},
      {kCFunctionAirSetEvent, &RendererFunctions::AirSetEvent},
      {kCFunctionAirSetID, &RendererFunctions::AirSetID},
      {kCFunctionAirGetElementByID, &RendererFunctions::AirGetElementByID},
      {kCFunctionAirGetElementByLepusID,
       &RendererFunctions::AirGetElementByLepusID},
      {kCFunctionAirUpdateIfNodeIndex,
       &RendererFunctions::AirUpdateIfNodeIndex},
      {kCFunctionAirUpdateForNodeIndex,
       &RendererFunctions::AirUpdateForNodeIndex},
      {kCFunctionAirUpdateForChildCount,
       &RendererFunctions::AirUpdateForChildCount},
      {kCFunctionAirGetForNodeChildWithIndex,
       &RendererFunctions::AirGetForNodeChildWithIndex},
      {kCFunctionAirPushForNode, &RendererFunctions::AirPushForNode},
      {kCFunctionAirPopForNode, &RendererFunctions::AirPopForNode},
      {kCFunctionAirGetChildElementByIndex,
       &RendererFunctions::AirGetChildElementByIndex},
      {kCFunctionAirPushAirDynamicNode, &RendererFunctions::AirPushDynamicNode},
      {kCFunctionAirGetAirDynamicNode, &RendererFunctions::AirGetDynamicNode},
      {kCFunctionAirSetAirComponentProp,
       &RendererFunctions::AirSetComponentProp},
      {kCFunctionAirRenderComponentInLepus,
       &RendererFunctions::AirRenderComponentInLepus},
      {kCFunctionAirUpdateComponentInLepus,
       &RendererFunctions::AirUpdateComponentInLepus},
      {kCFunctionAirGetComponentInfo, &RendererFunctions::AirGetComponentInfo},
      {kCFunctionAirUpdateComponentInfo,
       &RendererFunctions::AirUpdateComponentInfo},
      {kCFunctionAirGetData, &RendererFunctions::AirGetData},
      {kCFunctionAirGetProps, &RendererFunctions::AirGetProps},
      {kCFunctionAirSetData, &RendererFunctions::AirSetData},
      {kCFunctionAirFlushElement, &RendererFunctions::AirFlushElement},
      {kCFunctionAirFlushElementTree, &RendererFunctions::AirFlushElementTree},
      {kCFunctionTriggerLepusBridge, &RendererFunctions::TriggerLepusBridge},
      {kCFunctionTriggerLepusBridgeSync,
       &RendererFunctions::TriggerLepusBridgeSync},
      {kCFunctionAirSetDataSet, &RendererFunctions::AirSetDataSet},
      {kCFunctionAirSendGlobalEvent, &RendererFunctions::AirSendGlobalEvent},
      {kCFunctionSetTimeout, &RendererFunctions::SetTimeout},
      {kCFunctionClearTimeout, &RendererFunctions::ClearTimeout},
      {kCFunctionSetTimeInterval, &RendererFunctions::SetInterval},
      {kCFunctionClearTimeInterval, &RendererFunctions::ClearTimeInterval},
      {kCFuncAddEventListener, &RendererFunctions::AddEventListener},
      {kCFuncRegisterDataProcessor, &RendererFunctions::RegisterDataProcessor},
      {kCFunctionAirGetElementByUniqueID,
       &RendererFunctions::AirGetElementByUniqueID},
      {kCFunctionAirGetRootElement, &RendererFunctions::AirGetRootElement},
      {kCFunctionRemoveEventListener, &RendererFunctions::RemoveEventListener},
      {kCFunctionTriggerComponentEvent,
       &RendererFunctions::TriggerComponentEvent},
      {kCFunctionAirCreateRawText, &RendererFunctions::AirCreateRawText},
      {kCFunctionAirSetClasses, &RendererFunctions::AirSetClasses},
      {kCFunctionAirPushComponentNode,
       &RendererFunctions::AirPushComponentNode},
      {kCFunctionAirPopComponentNode, &RendererFunctions::AirPopComponentNode},
      {kCFunctionAirGetParentForNode, &RendererFunctions::AirGetParentForNode},
      {kCFunctionReportError, &RendererFunctions::ReportError},
      {kCFunctionAirFlushTree, &RendererFunctions::AirFlushTree},
      {kCFunctionAirInvokeUIMethod, &RendererFunctions::InvokeUIMethod},
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
}

}  // namespace tasm
}  // namespace lynx
