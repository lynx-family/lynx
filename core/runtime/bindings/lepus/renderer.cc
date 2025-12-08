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

void Utils::RegisterBuiltin(lepus::Context* context) {
  lepus::RegisterCFunction(context, kCFuncIndexOf, &RendererFunctions::IndexOf);
  lepus::RegisterCFunction(context, kCFuncGetLength,
                           &RendererFunctions::GetLength);
  lepus::RegisterCFunction(context, kCFuncSetValueToMap,
                           &RendererFunctions::SetValueToMap);
}

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

static ALLOW_UNUSED_TYPE lepus::Value SlotFunction(lepus::Context* context,
                                                   lepus::Value*, int size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SLOT_FUNCTION);
  return lepus::Value();
}

void Renderer::RegisterBuiltin(lepus::Context* context, ArchOption option) {
  switch (option) {
    case FIBER_ARCH:
      RegisterBuiltinForFiber(context);
      break;
    default:
      RegisterBuiltinForRadon(context);
  }
}

void Renderer::RegisterBuiltinForRadon(lepus::Context* context) {
  // clang-format off
  /* To add a RenderFunction, it needs to be registered first to avoid conflicts across different branches. */
  /* 001 */ lepus::RegisterCFunction(context, kCFuncCreatePage, &RendererFunctions::CreateVirtualPage);
  /* 002 */ lepus::RegisterCFunction(context, kCFuncAttachPage, &RendererFunctions::AttachPage);
  /* 003 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualComponent,
                           &RendererFunctions::CreateVirtualComponent);
  /* 004 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualNode,
                           &RendererFunctions::CreateVirtualNode);
  /* 005 */ lepus::RegisterCFunction(context, kCFuncAppendChild, &RendererFunctions::AppendChild);
  /* 006 */ lepus::RegisterCFunction(context, kCFuncSetClassTo, &RendererFunctions::SetClassTo);
  /* 007 */ lepus::RegisterCFunction(context, kCFuncSetStyleTo, &RendererFunctions::SetStyleTo);
  /* 008 */ lepus::RegisterCFunction(context, kCFuncSetEventTo, &RendererFunctions::SetEventTo);
  /* 009 */ lepus::RegisterCFunction(context, kCFuncSetAttributeTo,
                           &RendererFunctions::SetAttributeTo);
  /* 010 */ lepus::RegisterCFunction(context, kCFuncSetStaticClassTo,
                           &RendererFunctions::SetStaticClassTo);
  /* 011 */ lepus::RegisterCFunction(context, kCFuncSetStaticStyleTo,
                           &RendererFunctions::SetStaticStyleTo);
  /* 012 */ lepus::RegisterCFunction(context, kCFuncSetStaticAttributeTo,
                           &RendererFunctions::SetStaticAttrTo);
  /* 013 */ lepus::RegisterCFunction(context, kCFuncSetDataSetTo, &RendererFunctions::SetDataSetTo);
  /* 014 */ lepus::RegisterCFunction(context, kCFuncSetStaticEventTo,
                           &RendererFunctions::SetStaticEventTo);
  /* 015 */ lepus::RegisterCFunction(context, kCFuncSetId, &RendererFunctions::SetId);
  /* 016 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualSlot,
                           &RendererFunctions::CreateSlot);
  /* 017 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualPlug,
                           &RendererFunctions::CreateVirtualPlug);
  /* 018 */ lepus::RegisterCFunction(context, kCFuncMarkComponentHasRenderer,
                           &RendererFunctions::MarkComponentHasRenderer);
  /* 019 */ lepus::RegisterCFunction(context, kCFuncSetProp, &RendererFunctions::SetProp);
  /* 020 */ lepus::RegisterCFunction(context, kCFuncSetData, &RendererFunctions::SetData);
  /* 021 */ lepus::RegisterCFunction(context, kCFuncAddPlugToComponent,
                           &RendererFunctions::AddVirtualPlugToComponent);
  /* 022 */ lepus::RegisterCFunction(context, kCFuncGetComponentData, &RendererFunctions::GetComponentData);
  /* 023 */ lepus::RegisterCFunction(context, kCFuncGetComponentProps,
                           &RendererFunctions::GetComponentProps);
  /* 024 */ lepus::RegisterCFunction(context, kCFuncSetDynamicStyleTo,
                           &RendererFunctions::SetDynamicStyleTo);
  /* 025 */ lepus::RegisterCFunction(context, kCFuncGetLazyLoadCount, &RendererFunctions::ThemedTranslationLegacy);
  /* 026 */ lepus::RegisterCFunction(context, kCFuncUpdateComponentInfo,
                           &RendererFunctions::UpdateComponentInfo);
  /* 027 */ lepus::RegisterCFunction(context, kCFuncGetComponentInfo, &RendererFunctions::GetComponentInfo);
  /* 028 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualListNode,
                           &RendererFunctions::CreateVirtualListNode);
  /* 029 */ lepus::RegisterCFunction(context, kCFuncAppendListComponentInfo,
                           &RendererFunctions::AppendListComponentInfo);
  /* 030 */ lepus::RegisterCFunction(context, kCFuncSetListRefreshComponentInfo,
                           &SlotFunction);
  /* 031 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualComponentByName,
                           &RendererFunctions::CreateComponentByName);
  /* 032 */ lepus::RegisterCFunction(context, kCFuncCreateDynamicVirtualComponent,
                           &RendererFunctions::CreateDynamicVirtualComponent);
  /* 033 */ lepus::RegisterCFunction(context, kCFuncRenderDynamicComponent,
                           &RendererFunctions::RenderDynamicComponent);
  /* 034 */ lepus::RegisterCFunction(context, kCFuncThemedTranslation,
                           &RendererFunctions::ThemedTranslation);
  /* 035 */ lepus::RegisterCFunction(context, kCFuncRegisterDataProcessor,
                           &RendererFunctions::RegisterDataProcessor);
  /* 036 */ lepus::RegisterCFunction(context, kCFuncThemedLangTranslation,
                           &RendererFunctions::ThemedLanguageTranslation);
  /* 037 */ lepus::RegisterCFunction(context, kCFuncGetComponentContextData,
                               &RendererFunctions::GetComponentContextData);
  /* 038 */ lepus::RegisterCFunction(context, kCFuncProcessComponentData, &RendererFunctions::ProcessComponentData);
  /* 039 */ lepus::RegisterCFunction(context, "__slot__39", &SlotFunction);
  /* 040 */ lepus::RegisterCFunction(context, "__slot__40", &SlotFunction);
  /* 041 */ lepus::RegisterCFunction(context, "__slot__41", &SlotFunction);
  /* 042 */ lepus::RegisterCFunction(context, "__slot__42", &SlotFunction);
  /* 043 */ lepus::RegisterCFunction(context, "__slot__43", &SlotFunction);
  /* 044 */ lepus::RegisterCFunction(context, "__slot__44", &SlotFunction);
  /* 045 */ lepus::RegisterCFunction(context, "__slot__45", &SlotFunction);
  /* 046 */ lepus::RegisterCFunction(context, "__slot__46", &SlotFunction);
  /* 047 */ lepus::RegisterCFunction(context, "__slot__47", &SlotFunction);
  /* 048 */ lepus::RegisterCFunction(context, "__slot__48", &SlotFunction);
  /* 049 */ lepus::RegisterCFunction(context, "__slot__49", &SlotFunction);
  /* 050 */ lepus::RegisterCFunction(context, "__slot__50", &SlotFunction);
  /* 051 */ lepus::RegisterCFunction(context, "__slot__51", &SlotFunction);
  // warning: double 51.
  /* 051 */ lepus::RegisterCFunction(context, "__slot__51_1", &SlotFunction);
  /* 052 */ lepus::RegisterCFunction(context, "__slot__52", &SlotFunction);
  /* 053 */ lepus::RegisterCFunction(context, "__slot__53", &SlotFunction);
  /* 054 */ lepus::RegisterCFunction(context, "__slot__54", &SlotFunction);
  /* 055 */ lepus::RegisterCFunction(context, "__slot__55", &SlotFunction);
  /* 056 */ lepus::RegisterCFunction(context, "__slot__56", &SlotFunction);
  /* 057 */ lepus::RegisterCFunction(context, "__slot__57", &SlotFunction);
  /* 058 */ lepus::RegisterCFunction(context, "__slot__58", &SlotFunction);
  /* 059 */ lepus::RegisterCFunction(context, "__slot__59", &SlotFunction);
  /* 060 */ lepus::RegisterCFunction(context, "__slot__60", &SlotFunction);
  /* 061 */ lepus::RegisterCFunction(context, "__slot__61", &SlotFunction);
  /* 062 */ lepus::RegisterCFunction(context, "__slot__62", &SlotFunction);
  /* 063 */ lepus::RegisterCFunction(context, "__slot__63", &SlotFunction);
  /* 064 */ lepus::RegisterCFunction(context, "__slot__64", &SlotFunction);
  /* 065 */ lepus::RegisterCFunction(context, "__slot__65", &SlotFunction);
  /* 066 */ lepus::RegisterCFunction(context, "__slot__66", &SlotFunction);
  /* 067 */ lepus::RegisterCFunction(context, "__slot__67", &SlotFunction);
  /* 068 */ lepus::RegisterCFunction(context, "__slot__68", &SlotFunction);
  /* 069 */ lepus::RegisterCFunction(context, "__slot__69", &SlotFunction);
  /* 070 */ lepus::RegisterCFunction(context, "__slot__70", &SlotFunction);
  /* 071 */ lepus::RegisterCFunction(context, "__slot__71", &SlotFunction);
  /* 072 */ lepus::RegisterCFunction(context, "__slot__72", &SlotFunction);
  /* 073 */ lepus::RegisterCFunction(context, "__slot__73", &SlotFunction);
  /* 074 */ lepus::RegisterCFunction(context, "__slot__74", &SlotFunction);
  /* 075 */ lepus::RegisterCFunction(context, kCFuncSetStaticStyleToByFiber,
          &RendererFunctions::SetStaticStyleTo2);
  /* 076 */ lepus::RegisterCFunction(context, "__slot__76", &SlotFunction);
  /* 077 */ lepus::RegisterCFunction(context, "__slot__77", &SlotFunction);
  /* 078 */ lepus::RegisterCFunction(context, "__slot__78", &SlotFunction);
  /* 079 */ lepus::RegisterCFunction(context, "__slot__79", &SlotFunction);
  /* 080 */ lepus::RegisterCFunction(context, "__slot__80", &SlotFunction);
  /* 081 */ lepus::RegisterCFunction(context, kCFuncSetContextData, &RendererFunctions::SetContextData);
  /* 082 */ lepus::RegisterCFunction(context, kCFuncSetScriptEventTo, &RendererFunctions::SetScriptEventTo);
  /* 083 */ lepus::RegisterCFunction(context, kCFuncRegisterElementWorklet, &RendererFunctions::RegisterElementWorklet);
  /* 084 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualPlugWithComponent, &RendererFunctions::CreateVirtualPlugWithComponent);
  /* 085 */ lepus::RegisterCFunction(context, "__slot__85", &SlotFunction);
  /* 086 */ lepus::RegisterCFunction(context, kCFuncAddEventListener, &RendererFunctions::AddEventListener);
  /* 087 */ lepus::RegisterCFunction(context, kCFuncI18nResourceTranslation, &RendererFunctions::I18nResourceTranslation);
  /* 088 */ lepus::RegisterCFunction(context, kCFuncReFlushPage, &RendererFunctions::ReFlushPage);
  /* 089 */ lepus::RegisterCFunction(context, kCFuncSetComponent, &RendererFunctions::SetComponent);
  /* 090 */ lepus::RegisterCFunction(context, kCFuncGetGlobalProps, &RendererFunctions::GetGlobalProps);
  /* 091 */ lepus::RegisterCFunction(context, "__slot__91", &SlotFunction);
  /* 092 */ lepus::RegisterCFunction(context, kCFuncAppendSubTree, &RendererFunctions::AppendSubTree);
  /* 093 */ lepus::RegisterCFunction(context, kCFuncHandleExceptionInLepus, &RendererFunctions::HandleExceptionInLepus);
  /* 094 */ lepus::RegisterCFunction(context, kCFuncAppendVirtualPlugToComponent,
                                     &RendererFunctions::AppendVirtualPlugToComponent);
  /* 095 */ lepus::RegisterCFunction(context, kCFuncMarkPageElement, &RendererFunctions::MarkPageElement);
  /* 096 */ lepus::RegisterCFunction(context, kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource);
  /* 097 */ lepus::RegisterCFunction(context, kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent);
  /* 098 */ lepus::RegisterCFunction(context, kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease);
  /* 099 */ lepus::RegisterCFunction(context, kCFuncCloneSubTree, &RendererFunctions::CloneSubTree);
  /* 100 */ lepus::RegisterCFunction(context, kCFuncGetSystemInfo, &RendererFunctions::GetSystemInfo);
  /* 101 */ lepus::RegisterCFunction(context, kCFuncAddFallbackToDynamicComponent,
                           &RendererFunctions::AddFallbackToDynamicComponent);
  /* 102 */ lepus::RegisterCFunction(context, kCFuncCreateGestureDetector, &RendererFunctions::CreateGestureDetector);
  /* 103 */ lepus::RegisterCFunction(context, kCFunctionElementAnimate, &RendererFunctions::ElementAnimate);
  // clang-format on
}

void Renderer::RegisterBuiltinForFiber(lepus::Context* context) {
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  /* 001 */ lepus::RegisterCFunction(context, kCFunctionCreateElement,
                                     &RendererFunctions::FiberCreateElement);
  /* 002 */ lepus::RegisterCFunction(context, kCFunctionCreatePage,
                                     &RendererFunctions::FiberCreatePage);
  /* 003 */ lepus::RegisterCFunction(context, kCFunctionCreateComponent,
                                     &RendererFunctions::FiberCreateComponent);
  /* 004 */ lepus::RegisterCFunction(context, kCFunctionCreateView,
                                     &RendererFunctions::FiberCreateView);
  /* 005 */ lepus::RegisterCFunction(context, kCFunctionCreateList,
                                     &RendererFunctions::FiberCreateList);
  /* 006 */ lepus::RegisterCFunction(context, kCFunctionCreateScrollView,
                                     &RendererFunctions::FiberCreateScrollView);
  /* 007 */ lepus::RegisterCFunction(context, kCFunctionCreateText,
                                     &RendererFunctions::FiberCreateText);
  /* 008 */ lepus::RegisterCFunction(context, kCFunctionCreateImage,
                                     &RendererFunctions::FiberCreateImage);
  /* 009 */ lepus::RegisterCFunction(context, kCFunctionCreateRawText,
                                     &RendererFunctions::FiberCreateRawText);
  /* 010 */ lepus::RegisterCFunction(context, kCFunctionCreateNonElement,
                                     &RendererFunctions::FiberCreateNonElement);
  /* 011 */ lepus::RegisterCFunction(
      context, kCFunctionCreateWrapperElement,
      &RendererFunctions::FiberCreateWrapperElement);
  /* 012 */ lepus::RegisterCFunction(context, kCFunctionAppendElement,
                                     &RendererFunctions::FiberAppendElement);
  /* 013 */ lepus::RegisterCFunction(context, kCFunctionRemoveElement,
                                     &RendererFunctions::FiberRemoveElement);
  /* 014 */ lepus::RegisterCFunction(
      context, kCFunctionInsertElementBefore,
      &RendererFunctions::FiberInsertElementBefore);
  /* 015 */ lepus::RegisterCFunction(context, kCFunctionFirstElement,
                                     &RendererFunctions::FiberFirstElement);
  /* 016 */ lepus::RegisterCFunction(context, kCFunctionLastElement,
                                     &RendererFunctions::FiberLastElement);
  /* 017 */ lepus::RegisterCFunction(context, kCFunctionNextElement,
                                     &RendererFunctions::FiberNextElement);
  /* 018 */ lepus::RegisterCFunction(context, kCFunctionReplaceElement,
                                     &RendererFunctions::FiberReplaceElement);
  /* 019 */ lepus::RegisterCFunction(context, kCFunctionSwapElement,
                                     &RendererFunctions::FiberSwapElement);
  /* 020 */ lepus::RegisterCFunction(context, kCFunctionGetParent,
                                     &RendererFunctions::FiberGetParent);
  /* 021 */ lepus::RegisterCFunction(context, kCFunctionGetChildren,
                                     &RendererFunctions::FiberGetChildren);
  /* 022 */ lepus::RegisterCFunction(context, kCFunctionCloneElement,
                                     &RendererFunctions::FiberCloneElement);
  /* 023 */ lepus::RegisterCFunction(context, kCFunctionElementIsEqual,
                                     &RendererFunctions::FiberElementIsEqual);
  /* 024 */ lepus::RegisterCFunction(
      context, kCFunctionGetElementUniqueID,
      &RendererFunctions::FiberGetElementUniqueID);
  /* 025 */ lepus::RegisterCFunction(context, kCFunctionGetTag,
                                     &RendererFunctions::FiberGetTag);
  /* 026 */ lepus::RegisterCFunction(context, kCFunctionSetAttribute,
                                     &RendererFunctions::FiberSetAttribute);
  /* 027 */ lepus::RegisterCFunction(context, kCFunctionGetAttributes,
                                     &RendererFunctions::FiberGetAttributes);
  /* 028 */ lepus::RegisterCFunction(context, kCFunctionAddClass,
                                     &RendererFunctions::FiberAddClass);
  /* 029 */ lepus::RegisterCFunction(context, kCFunctionSetClasses,
                                     &RendererFunctions::FiberSetClasses);
  /* 030 */ lepus::RegisterCFunction(context, kCFunctionGetClasses,
                                     &RendererFunctions::FiberGetClasses);
  /* 031 */ lepus::RegisterCFunction(context, kCFunctionAddInlineStyle,
                                     &RendererFunctions::FiberAddInlineStyle);
  /* 032 */ lepus::RegisterCFunction(context, kCFunctionSetInlineStyles,
                                     &RendererFunctions::FiberSetInlineStyles);
  /* 033 */ lepus::RegisterCFunction(context, kCFunctionGetInlineStyles,
                                     &RendererFunctions::FiberGetInlineStyles);
  /* 034 */ lepus::RegisterCFunction(context, kCFunctionSetParsedStyles,
                                     &RendererFunctions::FiberSetParsedStyles);
  /* 035 */ lepus::RegisterCFunction(
      context, kCFunctionGetComputedStyles,
      &RendererFunctions::FiberGetComputedStyles);
  /* 036 */ lepus::RegisterCFunction(context, kCFunctionAddEvent,
                                     &RendererFunctions::FiberAddEvent);
  /* 037 */ lepus::RegisterCFunction(context, kCFunctionSetEvents,
                                     &RendererFunctions::FiberSetEvents);
  /* 038 */ lepus::RegisterCFunction(context, kCFunctionGetEvent,
                                     &RendererFunctions::FiberGetEvent);
  /* 039 */ lepus::RegisterCFunction(context, kCFunctionGetEvents,
                                     &RendererFunctions::FiberGetEvents);
  /* 040 */ lepus::RegisterCFunction(context, kCFunctionSetID,
                                     &RendererFunctions::FiberSetID);
  /* 041 */ lepus::RegisterCFunction(context, kCFunctionGetID,
                                     &RendererFunctions::FiberGetID);
  /* 042 */ lepus::RegisterCFunction(context, kCFunctionAddDataset,
                                     &RendererFunctions::FiberAddDataset);
  /* 043 */ lepus::RegisterCFunction(context, kCFunctionSetDataset,
                                     &RendererFunctions::FiberSetDataset);
  /* 044 */ lepus::RegisterCFunction(context, kCFunctionGetDataset,
                                     &RendererFunctions::FiberGetDataset);
  /* 045 */ lepus::RegisterCFunction(context, kCFunctionGetComponentID,
                                     &RendererFunctions::FiberGetComponentID);
  /* 046 */ lepus::RegisterCFunction(
      context, kCFunctionUpdateComponentID,
      &RendererFunctions::FiberUpdateComponentID);
  /* 047 */ lepus::RegisterCFunction(
      context, kCFunctionElementFromBinary,
      &RendererFunctions::FiberElementFromBinary);
  /* 048 */ lepus::RegisterCFunction(
      context, kCFunctionElementFromBinaryAsync,
      &RendererFunctions::FiberElementFromBinaryAsync);
  /* 049 */ lepus::RegisterCFunction(
      context, kCFunctionUpdateListCallbacks,
      &RendererFunctions::FiberUpdateListCallbacks);
  /* 050 */ lepus::RegisterCFunction(context, kCFunctionFlushElementTree,
                                     &RendererFunctions::FiberFlushElementTree);
  /* 051 */ lepus::RegisterCFunction(context, kCFunctionOnLifecycleEvent,
                                     &RendererFunctions::FiberOnLifecycleEvent);
  /* 052 */ lepus::RegisterCFunction(context, kCFunctionQueryComponent,
                                     &RendererFunctions::FiberQueryComponent);
  /* 053 */ lepus::RegisterCFunction(context, kCFunctionSetCSSId,
                                     &RendererFunctions::FiberSetCSSId);
  /* 054 */ lepus::RegisterCFunction(context, kCFunctionSetSourceMapRelease,
                                     &RendererFunctions::SetSourceMapRelease);
  /* 055 */ lepus::RegisterCFunction(context, kCFuncAddEventListener,
                                     &RendererFunctions::AddEventListener);
  /* 056 */ lepus::RegisterCFunction(
      context, kCFuncI18nResourceTranslation,
      &RendererFunctions::I18nResourceTranslation);
  /* 057 */ lepus::RegisterCFunction(context, kCFuncFilterI18nResource,
                                     &RendererFunctions::FilterI18nResource);
  /* 058 */ lepus::RegisterCFunction(context, kCFuncSendGlobalEvent,
                                     &RendererFunctions::SendGlobalEvent);
  /* 059 */ lepus::RegisterCFunction(context, kCFunctionReportError,
                                     &RendererFunctions::ReportError);
  /* 060 */ lepus::RegisterCFunction(context, kCFunctionGetDataByKey,
                                     &RendererFunctions::FiberGetDataByKey);
  /* 061 */ lepus::RegisterCFunction(context, kCFunctionReplaceElements,
                                     &RendererFunctions::FiberReplaceElements);
  /* 062 */ lepus::RegisterCFunction(context, kCFunctionQuerySelector,
                                     &RendererFunctions::FiberQuerySelector);
  /* 063 */ lepus::RegisterCFunction(context, kCFunctionQuerySelectorAll,
                                     &RendererFunctions::FiberQuerySelectorAll);
  /* 064 */ lepus::RegisterCFunction(context, kCFunctionSetLepusInitData,
                                     &RendererFunctions::FiberSetLepusInitData);
  /* 065 */ lepus::RegisterCFunction(context, kCFunctionAddConfig,
                                     &RendererFunctions::FiberAddConfig);
  /* 066 */ lepus::RegisterCFunction(context, kCFunctionSetConfig,
                                     &RendererFunctions::FiberSetConfig);
  /* 067 */ lepus::RegisterCFunction(
      context, kCFunctionUpdateComponentInfo,
      &RendererFunctions::FiberUpdateComponentInfo);
  /* 068 */ lepus::RegisterCFunction(context, kCFunctionGetConfig,
                                     &RendererFunctions::FiberGetElementConfig);
  /* 069 */ lepus::RegisterCFunction(context, kCFunctionGetInlineStyle,
                                     &RendererFunctions::FiberGetInlineStyle);
  /* 070 */ lepus::RegisterCFunction(
      context, kCFuncSetGestureDetector,
      &RendererFunctions::FiberSetGestureDetector);
  /* 071 */ lepus::RegisterCFunction(
      context, kCFuncRemoveGestureDetector,
      &RendererFunctions::FiberRemoveGestureDetector);
  /* 072 */ lepus::RegisterCFunction(
      context, kCFunctionGetAttributeByName,
      &RendererFunctions::FiberGetAttributeByName);
  /* 073 */ lepus::RegisterCFunction(
      context, kCFunctionGetAttributeNames,
      &RendererFunctions::FiberGetAttributeNames);
  /* 074 */ lepus::RegisterCFunction(context, kCFunctionGetPageElement,
                                     &RendererFunctions::FiberGetPageElement);
  /* 075 */ lepus::RegisterCFunction(context, kCFunctionCreateIf,
                                     &RendererFunctions::FiberCreateIf);
  /* 076 */ lepus::RegisterCFunction(context, kCFunctionCreateFor,
                                     &RendererFunctions::FiberCreateFor);
  /* 077 */ lepus::RegisterCFunction(context, kCFunctionCreateBlock,
                                     &RendererFunctions::FiberCreateBlock);
  /* 078 */ lepus::RegisterCFunction(
      context, kCFunctionUpdateIfNodeIndex,
      &RendererFunctions::FiberUpdateIfNodeIndex);
  /* 079 */ lepus::RegisterCFunction(
      context, kCFunctionUpdateForChildCount,
      &RendererFunctions::FiberUpdateForChildCount);
  /* 080 */ lepus::RegisterCFunction(
      context, kCFunctionGetElementByUniqueID,
      &RendererFunctions::FiberGetElementByUniqueID);
  /* 081 */ lepus::RegisterCFunction(context, kCFunctionGetDiffData,
                                     &RendererFunctions::FiberGetDiffData);
  /* 082 */ lepus::RegisterCFunction(context, kCFunctionLoadLepusChunk,
                                     &RendererFunctions::LoadLepusChunk);
  /* 083 */ lepus::RegisterCFunction(context, kCFuncSetGestureState,
                                     &RendererFunctions::FiberSetGestureState);
  /* 084 */ lepus::RegisterCFunction(
      context, kCFunctionMarkTemplateElement,
      &RendererFunctions::FiberMarkTemplateElement);
  /* 085 */ lepus::RegisterCFunction(
      context, kCFunctionIsTemplateElement,
      &RendererFunctions::FiberIsTemplateElement);
  /* 086 */ lepus::RegisterCFunction(context, kCFunctionMarkPartElement,
                                     &RendererFunctions::FiberMarkPartElement);
  /* 087 */ lepus::RegisterCFunction(context, kCFunctionIsPartElement,
                                     &RendererFunctions::FiberIsPartElement);
  /* 088 */ lepus::RegisterCFunction(context, kCFunctionGetTemplateParts,
                                     &RendererFunctions::FiberGetTemplateParts);
  /* 089 */ lepus::RegisterCFunction(
      context, kCFunctionAsyncResolveElement,
      &RendererFunctions::FiberAsyncResolveElement);
  /* 090 */ lepus::RegisterCFunction(context, kCFuncConsumeGesture,
                                     &RendererFunctions::FiberConsumeGesture);
  /* 091 */ lepus::RegisterCFunction(
      context, kCFunctionCreateElementWithProperties,
      &RendererFunctions::FiberCreateElementWithProperties);
  /* 092 */ lepus::RegisterCFunction(context, kCFunctionCreateSignal,
                                     &RendererFunctions::FiberCreateSignal);
  /* 093 */ lepus::RegisterCFunction(context, kCFunctionWriteSignal,
                                     &RendererFunctions::FiberWriteSignal);
  /* 094 */ lepus::RegisterCFunction(context, kCFunctionReadSignal,
                                     &RendererFunctions::FiberReadSignal);
  /* 095 */ lepus::RegisterCFunction(
      context, kCFunctionCreateComputation,
      &RendererFunctions::FiberCreateComputation);
  /* 096 */ lepus::RegisterCFunction(context, kCFunctionCreateMemo,
                                     &RendererFunctions::FiberCreateMemo);
  /* 097 */ lepus::RegisterCFunction(context, kCFunctionCreateScope,
                                     &RendererFunctions::FiberCreateScope);
  /* 098 */ lepus::RegisterCFunction(context, kCFunctionGetScope,
                                     &RendererFunctions::FiberGetScope);
  /* 099 */ lepus::RegisterCFunction(context, kCFunctionCleanUp,
                                     &RendererFunctions::FiberCleanUp);
  /* 100 */ lepus::RegisterCFunction(context, kCFunctionOnCleanUp,
                                     &RendererFunctions::FiberOnCleanUp);
  /* 101 */ lepus::RegisterCFunction(context, kCFunctionUnTrack,
                                     &RendererFunctions::FiberUnTrack);
  /* 102 */ lepus::RegisterCFunction(context, kCFunctionCreateFrame,
                                     &RendererFunctions::FiberCreateFrame);
  /* 103 */ lepus::RegisterCFunction(context, kCFunctionRunUpdates,
                                     &RendererFunctions::FiberRunUpdates);
  /* 104 */ lepus::RegisterCFunction(context, kCFunctionCreateStyleObject,
                                     &RendererFunctions::CreateStyleObject);
  /* 105 */ lepus::RegisterCFunction(context, kCFunctionSetStyleObject,
                                     &RendererFunctions::SetStyleObject);
  /* 106 */ lepus::RegisterCFunction(context, kCFunctionUpdateStyleObject,
                                     &RendererFunctions::UpdateStyleObject);
  /* 107 */ lepus::RegisterCFunction(
      context, kCFunctionAsyncResolveSubtreeProperty,
      &RendererFunctions::FiberAsyncResolveSubtreeProperty);
  /* 108 */ lepus::RegisterCFunction(
      context, kCFunctionMarkAsyncFlushRoot,
      &RendererFunctions::FiberMarkAsyncResolveRoot);
  /* 109 */ lepus::RegisterCFunction(context, kCFunctionAddEventListener,
                                     &RendererFunctions::FiberAddEventListener);
  /* 110 */ lepus::RegisterCFunction(
      context, kCFunctionFiberRemoveEventListener,
      &RendererFunctions::FiberRemoveEventListener);
  /* 111 */ lepus::RegisterCFunction(context, kCFunctionCreateEvent,
                                     &RendererFunctions::FiberCreateEvent);
  /* 112 */ lepus::RegisterCFunction(context, kCFunctionDispatchEvent,
                                     &RendererFunctions::FiberDispatchEvent);
  /* 113 */ lepus::RegisterCFunction(context, kCFunctionStopPropagation,
                                     &RendererFunctions::FiberStopPropagation);
  /* 114 */ lepus::RegisterCFunction(
      context, kCFunctionStopImmediatePropagation,
      &RendererFunctions::FiberStopImmediatePropagation);
  /* 115 */ lepus::RegisterCFunction(context, kCFunctionInvokeUIMethod,
                                     &RendererFunctions::InvokeUIMethod);
  /* 116 */ lepus::RegisterCFunction(
      context, kCFunctionGetComputedStyleByKey,
      &RendererFunctions::FiberGetComputedStyleByKey);
}
}  // namespace tasm
}  // namespace lynx
