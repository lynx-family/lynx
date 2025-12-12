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
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/runtime/vm/lepus/builtin.h"

namespace lynx {
namespace tasm {

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
        {runtime::kGetNative, &RendererFunctions::GetNative},
        {runtime::kGetEngine, &RendererFunctions::GetEngine},
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
        {kLoadScript, &RendererFunctions::LoadScript},
        {kFetchBundle, &RendererFunctions::FetchBundle},
        {kGetModule, &RendererFunctions::GetModule},
        // exposure
        {tasm::kStopExposure, &RendererFunctions::StopExposure},
        {tasm::kResumeExposure, &RendererFunctions::ResumeExposure},
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
    lepus::Value perf_obj(lepus::LEPUSValueHelper::CreateObject(context));
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

void Utils::RegisterNGMethodToResponseHandler(lepus::Context* context,
                                              lepus::Value& response_handler) {
  if (response_handler.IsJSValue()) {
    const static lepus::RenderBindingFunction funcs[] = {
        {runtime::kWait, &RendererFunctions::WaitingForResponse},
        {runtime::kThen, &RendererFunctions::AddListenerForResponse},
    };
    lepus::RegisterObjectNGCFunction(context, response_handler, funcs,
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

void Utils::RegisterNGMethodToLepusModule(lepus::Context* context,
                                          lepus::Value& lepus_module) {
  if (lepus_module.IsJSValue()) {
    // Register function for invoke module method
    const static lepus::RenderBindingFunction funcs[] = {
        {runtime::kInvoke, &RendererFunctions::InvokeModuleMethod},
    };
    lepus::RegisterObjectNGCFunction(context, lepus_module, funcs,
                                     sizeof(funcs) / sizeof(funcs[0]));
  }
}

lepus::Value Renderer::SlotFunction(lepus::Context* context, lepus::Value*,
                                    int size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SLOT_FUNCTION);
  return lepus::Value();
}

void Renderer::RegisterNGBuiltin(lepus::Context* context, ArchOption option) {
  switch (option) {
    case ArchOption::FIBER_ARCH:
      RegisterNGBuiltinForFiber(context);
      break;
    default:
      RegisterNGBuiltinForRadon(context);
  }
}

void Renderer::RegisterNGBuiltinForRadon(lepus::Context* context) {
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
      {kCFunctionElementAnimate, &RendererFunctions::ElementAnimate},
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
}

void Renderer::RegisterNGBuiltinForFiber(lepus::Context* context) {
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
      {kCFunctionCreateFrame, &RendererFunctions::FiberCreateFrame},
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
      {kCFunctionAsyncResolveSubtreeProperty,
       &RendererFunctions::FiberAsyncResolveSubtreeProperty},
      {kCFunctionMarkAsyncFlushRoot,
       &RendererFunctions::FiberMarkAsyncResolveRoot},
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
      {kCFunctionRunUpdates, &RendererFunctions::FiberRunUpdates},
      {kCFunctionElementAnimate, &RendererFunctions::ElementAnimate},
      {kCFunctionCreateStyleObject, &RendererFunctions::CreateStyleObject},
      {kCFunctionSetStyleObject, &RendererFunctions::SetStyleObject},
      {kCFunctionUpdateStyleObject, &RendererFunctions::UpdateStyleObject},
      {kCFunctionAddEventListener, &RendererFunctions::FiberAddEventListener},
      {kCFunctionFiberRemoveEventListener,
       &RendererFunctions::FiberRemoveEventListener},
      {kCFunctionCreateEvent, &RendererFunctions::FiberCreateEvent},
      {kCFunctionDispatchEvent, &RendererFunctions::FiberDispatchEvent},
      {kCFunctionStopPropagation, &RendererFunctions::FiberStopPropagation},
      {kCFunctionStopImmediatePropagation,
       &RendererFunctions::FiberStopImmediatePropagation},
      {kCFunctionGetComputedStyleByKey,
       &RendererFunctions::FiberGetComputedStyleByKey},
      /* Element API END */
  };
  lepus::RegisterNGCFunction(context, funcs, sizeof(funcs) / sizeof(funcs[0]));
  return;
}

}  // namespace tasm
}  // namespace lynx
