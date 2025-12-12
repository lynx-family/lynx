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
  lepus::RegisterCFunction(context, kCFuncIndexOf, &RendererFunctions::IndexOf);
  lepus::RegisterCFunction(context, kCFuncGetLength,
                           &RendererFunctions::GetLength);
  lepus::RegisterCFunction(context, kCFuncSetValueToMap,
                           &RendererFunctions::SetValueToMap);
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
  /* 104 */  lepus::RegisterCFunction(context, kCFuncSetStaticStyleTo2, &RendererFunctions::SetStaticStyleTo2);
  /* 105 */ lepus::RegisterCFunction(context, kSetTimeout, &RendererFunctions::SetTimeout);
  /* 106 */ lepus::RegisterCFunction(context, kClearTimeout, &RendererFunctions::ClearTimeout);
  /* 107 */ lepus::RegisterCFunction(context, kSetInterval, &RendererFunctions::SetInterval);
  /* 108 */ lepus::RegisterCFunction(context, kClearTimeInterval, &RendererFunctions::ClearTimeInterval);
  /* 109 */ lepus::RegisterCFunction(context, kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame);
  /* 110 */ lepus::RegisterCFunction(context, kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame);
  // clang-format on
}

void Renderer::RegisterBuiltinForFiber(lepus::Context* context) {
  lepus::RegisterCFunction(context, kCFuncIndexOf, &RendererFunctions::IndexOf);
  lepus::RegisterCFunction(context, kCFuncGetLength,
                           &RendererFunctions::GetLength);
  lepus::RegisterCFunction(context, kCFuncSetValueToMap,
                           &RendererFunctions::SetValueToMap);
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
  /*117*/ lepus::RegisterCFunction(context, kSetTimeout,
                                   &RendererFunctions::SetTimeout);
  /*118*/ lepus::RegisterCFunction(context, kClearTimeout,
                                   &RendererFunctions::ClearTimeout);
  /*119*/ lepus::RegisterCFunction(context, kSetInterval,
                                   &RendererFunctions::SetInterval);
  /*120*/ lepus::RegisterCFunction(context, kClearTimeInterval,
                                   &RendererFunctions::ClearTimeInterval);
  /*121*/ lepus::RegisterCFunction(context, kRequestAnimationFrame,
                                   &RendererFunctions::RequestAnimationFrame);
  /*122*/ lepus::RegisterCFunction(context, kCancelAnimationFrame,
                                   &RendererFunctions::CancelAnimationFrame);
  /*123*/ lepus::RegisterCFunction(context, kCFunctionElementAnimate,
                                   &RendererFunctions::ElementAnimate);
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
  };
  // clang-format on
  size = sizeof(kFuncs) / sizeof(kFuncs[0]);
  return kFuncs;
}

}  // namespace tasm
}  // namespace lynx
