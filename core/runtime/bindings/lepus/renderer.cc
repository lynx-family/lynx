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
#include "core/runtime/vm/lepus/builtin.h"

namespace lynx {
namespace tasm {
#define RENDERER_FUNCTION(name) static lepus::Value name(lepus::Context* ctx)
#define PREPARE_ARGS(name)
#define CALL_RUNTIME_AND_RETURN(name)                   \
  return RendererFunctions::name(ctx, ctx->GetParam(0), \
                                 (int)(ctx->GetParamsSize()))

#define CREATE_FUNCTION(name) \
  RENDERER_FUNCTION(name) { return lepus::Value(); }

#include "core/runtime/bindings/lepus/renderer_template.h"

#if defined(OS_WIN)
#ifdef SetProp
#undef SetProp
#endif  // SetProp
#endif  // OS_WIN

void Utils::RegisterBuiltin(lepus::Context* context) {
  lepus::RegisterCFunction(context, kCFuncIndexOf, &IndexOf);
  lepus::RegisterCFunction(context, kCFuncGetLength, &GetLength);
  lepus::RegisterCFunction(context, kCFuncSetValueToMap, &SetValueToMap);
}

void Utils::RegisterMethodToLynx(lepus::Context* context, lepus::Value& lynx) {
  if (lynx.IsTable()) {
    auto lynx_table = lynx.Table();
    lepus::RegisterTableFunction(context, lynx_table, kGetTextInfo,
                                 GetTextInfo);

    lepus::RegisterTableFunction(context, lynx_table, kSetTimeout, &SetTimeout);
    lepus::RegisterTableFunction(context, lynx_table, kClearTimeout,
                                 &ClearTimeout);
    lepus::RegisterTableFunction(context, lynx_table, kSetInterval,
                                 &SetInterval);
    lepus::RegisterTableFunction(context, lynx_table, kClearTimeInterval,
                                 &ClearTimeInterval);
    lepus::RegisterTableFunction(
        context, lynx_table, kCFunctionTriggerLepusBridge, &TriggerLepusBridge);
    lepus::RegisterTableFunction(context, lynx_table,
                                 kCFunctionTriggerComponentEvent,
                                 &TriggerComponentEvent);
    lepus::RegisterTableFunction(context, lynx_table,
                                 kCFunctionTriggerLepusBridgeSync,
                                 &TriggerLepusBridgeSync);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetDevTool,
                                 GetDevTool);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetCoreContext,
                                 GetCoreContext);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetJSContext,
                                 GetJSContext);
    lepus::RegisterTableFunction(context, lynx_table, runtime::kGetUIContext,
                                 GetUIContext);
    lepus::RegisterTableFunction(context, lynx_table, kRequestAnimationFrame,
                                 &RequestAnimationFrame);
    lepus::RegisterTableFunction(context, lynx_table, kCancelAnimationFrame,
                                 &CancelAnimationFrame);
    lepus::RegisterTableFunction(context, lynx_table,
                                 runtime::kGetCustomSectionSync,
                                 &GetCustomSectionSync);
    lepus::RegisterTableFunction(context, lynx_table, kSetSessionStorageItem,
                                 &SetSessionStorageItem);
    lepus::RegisterTableFunction(context, lynx_table, kGetSessionStorageItem,
                                 &GetSessionStorageItem);
    // Timing
    RegisterMethodToLynxPerformance(context, lynx);
  }
}

void Utils::RegisterMethodToLynxPerformance(lepus::Context* context,
                                            lepus::Value& lynx) {
  if (lynx.IsTable()) {
    lepus::Value perf_obj(lepus::Value::CreateObject(context));
    lynx.SetProperty(BASE_STATIC_STRING(runtime::kPerformanceObject), perf_obj);

    auto perf_table = perf_obj.Table();
    lepus::RegisterTableFunction(context, perf_table,
                                 runtime::kGeneratePipelineOptions,
                                 &GeneratePipelineOptions);
    lepus::RegisterTableFunction(context, perf_table, runtime::kOnPipelineStart,
                                 &OnPipelineStart);
    lepus::RegisterTableFunction(context, perf_table, runtime::kMarkTiming,
                                 &MarkTiming);
    lepus::RegisterTableFunction(context, perf_table,
                                 runtime::kBindPipelineIDWithTimingFlag,
                                 &BindPipelineIDWithTimingFlag);
    lepus::RegisterTableFunction(
        context, perf_table, runtime::kAddTimingListener, &AddTimingListener);

    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileStart,
                                 &ProfileStart);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileEnd,
                                 &ProfileEnd);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileMark,
                                 &ProfileMark);
    lepus::RegisterTableFunction(context, perf_table, runtime::kProfileFlowId,
                                 &ProfileFlowId);
    lepus::RegisterTableFunction(
        context, perf_table, runtime::kIsProfileRecording, &IsProfileRecording);
  }
};

void Utils::RegisterMethodToContextProxy(lepus::Context* context,
                                         lepus::Value& target,
                                         runtime::ContextProxy::Type type) {
  if (target.IsTable()) {
    auto target_table = target.Table();
    lepus::RegisterTableFunction(context, target_table, runtime::kPostMessage,
                                 PostMessage);
    lepus::RegisterTableFunction(context, target_table, runtime::kDispatchEvent,
                                 DispatchEvent);
    lepus::RegisterTableFunction(context, target_table,
                                 runtime::kAddEventListener,
                                 RuntimeAddEventListener);
    lepus::RegisterTableFunction(context, target_table,
                                 runtime::kRemoveEventListener,
                                 RuntimeRemoveEventListener);

    if (type == runtime::ContextProxy::Type::kDevTool) {
      RegisterTableFunction(context, target_table,
                            runtime::kReplaceStyleSheetByIdWithBase64,
                            &ReplaceStyleSheetByIdWithBase64);
      RegisterTableFunction(context, target_table,
                            runtime::kRemoveStyleSheetById,
                            &RemoveStyleSheetById);
    }
  }
}

#undef RENDERER_FUNCTION
#undef CALL_RUNTIME_AND_RETURN
#undef PREPARE_ARGS

static ALLOW_UNUSED_TYPE lepus::Value SlotFunction(lepus::Context* context) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SlotFunction");
  return lepus::Value();
}

void Renderer::RegisterBuiltin(lepus::Context* context, ArchOption option) {
  switch (option) {
    case FIBER_ARCH:
      RegisterBuiltinForFiber(context);
      break;
    case AIR_ARCH:
      RegisterBuiltinForAir(context);
      break;
    default:
      RegisterBuiltinForRadon(context);
  }
}

void Renderer::RegisterBuiltinForRadon(lepus::Context* context) {
  // clang-format off
  /* To add a RenderFunction, it needs to be registered first to avoid conflicts across different branches. */
  /* 001 */ lepus::RegisterCFunction(context, kCFuncCreatePage, &CreateVirtualPage);
  /* 002 */ lepus::RegisterCFunction(context, kCFuncAttachPage, &AttachPage);
  /* 003 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualComponent,
                           &CreateVirtualComponent);
  /* 004 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualNode,
                           &CreateVirtualNode);
  /* 005 */ lepus::RegisterCFunction(context, kCFuncAppendChild, AppendChild);
  /* 006 */ lepus::RegisterCFunction(context, kCFuncSetClassTo, &SetClassTo);
  /* 007 */ lepus::RegisterCFunction(context, kCFuncSetStyleTo, &SetStyleTo);
  /* 008 */ lepus::RegisterCFunction(context, kCFuncSetEventTo, &SetEventTo);
  /* 009 */ lepus::RegisterCFunction(context, kCFuncSetAttributeTo,
                           SetAttributeTo);
  /* 010 */ lepus::RegisterCFunction(context, kCFuncSetStaticClassTo,
                           &SetStaticClassTo);
  /* 011 */ lepus::RegisterCFunction(context, kCFuncSetStaticStyleTo,
                           &SetStaticStyleTo);
  /* 012 */ lepus::RegisterCFunction(context, kCFuncSetStaticAttributeTo,
                           &SetStaticAttrTo);
  /* 013 */ lepus::RegisterCFunction(context, kCFuncSetDataSetTo, &SetDataSetTo);
  /* 014 */ lepus::RegisterCFunction(context, kCFuncSetStaticEventTo,
                           &SetStaticEventTo);
  /* 015 */ lepus::RegisterCFunction(context, kCFuncSetId, &SetId);
  /* 016 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualSlot,
                           &CreateSlot);
  /* 017 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualPlug,
                           &CreateVirtualPlug);
  /* 018 */ lepus::RegisterCFunction(context, kCFuncMarkComponentHasRenderer,
                           &MarkComponentHasRenderer);
  /* 019 */ lepus::RegisterCFunction(context, kCFuncSetProp, &SetProp);
  /* 020 */ lepus::RegisterCFunction(context, kCFuncSetData, &SetData);
  /* 021 */ lepus::RegisterCFunction(context, kCFuncAddPlugToComponent,
                           AddVirtualPlugToComponent);
  /* 022 */ lepus::RegisterCFunction(context, kCFuncGetComponentData, &GetComponentData);
  /* 023 */ lepus::RegisterCFunction(context, kCFuncGetComponentProps,
                           &GetComponentProps);
  /* 024 */ lepus::RegisterCFunction(context, kCFuncSetDynamicStyleTo,
                           &SetDynamicStyleTo);
  /* 025 */ lepus::RegisterCFunction(context, kCFuncGetLazyLoadCount, &ThemedTranslationLegacy);
  /* 026 */ lepus::RegisterCFunction(context, kCFuncUpdateComponentInfo,
                           &UpdateComponentInfo);
  /* 027 */ lepus::RegisterCFunction(context, kCFuncGetComponentInfo, &GetComponentInfo);
  /* 028 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualListNode,
                           &CreateVirtualListNode);
  /* 029 */ lepus::RegisterCFunction(context, kCFuncAppendListComponentInfo,
                           &AppendListComponentInfo);
  /* 030 */ lepus::RegisterCFunction(context, kCFuncSetListRefreshComponentInfo,
                           &SlotFunction);
  /* 031 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualComponentByName,
                           &CreateComponentByName);
  /* 032 */ lepus::RegisterCFunction(context, kCFuncCreateDynamicVirtualComponent,
                           &CreateDynamicVirtualComponent);
  /* 033 */ lepus::RegisterCFunction(context, kCFuncRenderDynamicComponent,
                           &RenderDynamicComponent);
  /* 034 */ lepus::RegisterCFunction(context, kCFuncThemedTranslation,
                           &ThemedTranslation);
  /* 035 */ lepus::RegisterCFunction(context, kCFuncRegisterDataProcessor,
                           &RegisterDataProcessor);
  /* 036 */ lepus::RegisterCFunction(context, kCFuncThemedLangTranslation,
                           &ThemedLanguageTranslation);
  /* 037 */ lepus::RegisterCFunction(context, kCFuncGetComponentContextData,
                               &GetComponentContextData);
  /* 038 */ lepus::RegisterCFunction(context, kCFuncProcessComponentData, &ProcessComponentData);
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
          &SetStaticStyleTo2);
  /* 076 */ lepus::RegisterCFunction(context, "__slot__76", &SlotFunction);
  /* 077 */ lepus::RegisterCFunction(context, "__slot__77", &SlotFunction);
  /* 078 */ lepus::RegisterCFunction(context, "__slot__78", &SlotFunction);
  /* 079 */ lepus::RegisterCFunction(context, "__slot__79", &SlotFunction);
  /* 080 */ lepus::RegisterCFunction(context, "__slot__80", &SlotFunction);
  /* 081 */ lepus::RegisterCFunction(context, kCFuncSetContextData, &SetContextData);
  /* 082 */ lepus::RegisterCFunction(context, kCFuncSetScriptEventTo, &SetScriptEventTo);
  /* 083 */ lepus::RegisterCFunction(context, kCFuncRegisterElementWorklet, &RegisterElementWorklet);
  /* 084 */ lepus::RegisterCFunction(context, kCFuncCreateVirtualPlugWithComponent, &CreateVirtualPlugWithComponent);
  /* 085 */ lepus::RegisterCFunction(context, "__slot__85", &SlotFunction);
  /* 086 */ lepus::RegisterCFunction(context, kCFuncAddEventListener, &AddEventListener);
  /* 087 */ lepus::RegisterCFunction(context, kCFuncI18nResourceTranslation, &I18nResourceTranslation);
  /* 088 */ lepus::RegisterCFunction(context, kCFuncReFlushPage, &ReFlushPage);
  /* 089 */ lepus::RegisterCFunction(context, kCFuncSetComponent, &SetComponent);
  /* 090 */ lepus::RegisterCFunction(context, kCFuncGetGlobalProps, &GetGlobalProps);
  /* 091 */ lepus::RegisterCFunction(context, "__slot__91", &SlotFunction);
  /* 092 */ lepus::RegisterCFunction(context, kCFuncAppendSubTree, &AppendSubTree);
  /* 093 */ lepus::RegisterCFunction(context, kCFuncHandleExceptionInLepus, &HandleExceptionInLepus);
  /* 094 */ lepus::RegisterCFunction(context, kCFuncAppendVirtualPlugToComponent,
                                     AppendVirtualPlugToComponent);
  /* 095 */ lepus::RegisterCFunction(context, kCFuncMarkPageElement, &MarkPageElement);
  /* 096 */ lepus::RegisterCFunction(context, kCFuncFilterI18nResource, &FilterI18nResource);
  /* 097 */ lepus::RegisterCFunction(context, kCFuncSendGlobalEvent, &SendGlobalEvent);
  /* 098 */ lepus::RegisterCFunction(context, kCFunctionSetSourceMapRelease, &SetSourceMapRelease);
  /* 099 */ lepus::RegisterCFunction(context, kCFuncCloneSubTree, &CloneSubTree);
  /* 100 */ lepus::RegisterCFunction(context, kCFuncGetSystemInfo, &GetSystemInfo);
  /* 101 */ lepus::RegisterCFunction(context, kCFuncAddFallbackToDynamicComponent,
                           &AddFallbackToDynamicComponent);
  /* 102 */ lepus::RegisterCFunction(context, kCFuncCreateGestureDetector, &CreateGestureDetector);
  // clang-format on
}

void Renderer::RegisterBuiltinForFiber(lepus::Context* context) {
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  /* 001 */ lepus::RegisterCFunction(context, kCFunctionCreateElement,
                                     &FiberCreateElement);
  /* 002 */ lepus::RegisterCFunction(context, kCFunctionCreatePage,
                                     &FiberCreatePage);
  /* 003 */ lepus::RegisterCFunction(context, kCFunctionCreateComponent,
                                     &FiberCreateComponent);
  /* 004 */ lepus::RegisterCFunction(context, kCFunctionCreateView,
                                     &FiberCreateView);
  /* 005 */ lepus::RegisterCFunction(context, kCFunctionCreateList,
                                     &FiberCreateList);
  /* 006 */ lepus::RegisterCFunction(context, kCFunctionCreateScrollView,
                                     &FiberCreateScrollView);
  /* 007 */ lepus::RegisterCFunction(context, kCFunctionCreateText,
                                     &FiberCreateText);
  /* 008 */ lepus::RegisterCFunction(context, kCFunctionCreateImage,
                                     &FiberCreateImage);
  /* 009 */ lepus::RegisterCFunction(context, kCFunctionCreateRawText,
                                     &FiberCreateRawText);
  /* 010 */ lepus::RegisterCFunction(context, kCFunctionCreateNonElement,
                                     &FiberCreateNonElement);
  /* 011 */ lepus::RegisterCFunction(context, kCFunctionCreateWrapperElement,
                                     &FiberCreateWrapperElement);
  /* 012 */ lepus::RegisterCFunction(context, kCFunctionAppendElement,
                                     &FiberAppendElement);
  /* 013 */ lepus::RegisterCFunction(context, kCFunctionRemoveElement,
                                     &FiberRemoveElement);
  /* 014 */ lepus::RegisterCFunction(context, kCFunctionInsertElementBefore,
                                     &FiberInsertElementBefore);
  /* 015 */ lepus::RegisterCFunction(context, kCFunctionFirstElement,
                                     &FiberFirstElement);
  /* 016 */ lepus::RegisterCFunction(context, kCFunctionLastElement,
                                     &FiberLastElement);
  /* 017 */ lepus::RegisterCFunction(context, kCFunctionNextElement,
                                     &FiberNextElement);
  /* 018 */ lepus::RegisterCFunction(context, kCFunctionReplaceElement,
                                     &FiberReplaceElement);
  /* 019 */ lepus::RegisterCFunction(context, kCFunctionSwapElement,
                                     &FiberSwapElement);
  /* 020 */ lepus::RegisterCFunction(context, kCFunctionGetParent,
                                     &FiberGetParent);
  /* 021 */ lepus::RegisterCFunction(context, kCFunctionGetChildren,
                                     &FiberGetChildren);
  /* 022 */ lepus::RegisterCFunction(context, kCFunctionCloneElement,
                                     &FiberCloneElement);
  /* 023 */ lepus::RegisterCFunction(context, kCFunctionElementIsEqual,
                                     &FiberElementIsEqual);
  /* 024 */ lepus::RegisterCFunction(context, kCFunctionGetElementUniqueID,
                                     &FiberGetElementUniqueID);
  /* 025 */ lepus::RegisterCFunction(context, kCFunctionGetTag, &FiberGetTag);
  /* 026 */ lepus::RegisterCFunction(context, kCFunctionSetAttribute,
                                     &FiberSetAttribute);
  /* 027 */ lepus::RegisterCFunction(context, kCFunctionGetAttributes,
                                     &FiberGetAttributes);
  /* 028 */ lepus::RegisterCFunction(context, kCFunctionAddClass,
                                     &FiberAddClass);
  /* 029 */ lepus::RegisterCFunction(context, kCFunctionSetClasses,
                                     &FiberSetClasses);
  /* 030 */ lepus::RegisterCFunction(context, kCFunctionGetClasses,
                                     &FiberGetClasses);
  /* 031 */ lepus::RegisterCFunction(context, kCFunctionAddInlineStyle,
                                     &FiberAddInlineStyle);
  /* 032 */ lepus::RegisterCFunction(context, kCFunctionSetInlineStyles,
                                     &FiberSetInlineStyles);
  /* 033 */ lepus::RegisterCFunction(context, kCFunctionGetInlineStyles,
                                     &FiberGetInlineStyles);
  /* 034 */ lepus::RegisterCFunction(context, kCFunctionSetParsedStyles,
                                     &FiberSetParsedStyles);
  /* 035 */ lepus::RegisterCFunction(context, kCFunctionGetComputedStyles,
                                     &FiberGetComputedStyles);
  /* 036 */ lepus::RegisterCFunction(context, kCFunctionAddEvent,
                                     &FiberAddEvent);
  /* 037 */ lepus::RegisterCFunction(context, kCFunctionSetEvents,
                                     &FiberSetEvents);
  /* 038 */ lepus::RegisterCFunction(context, kCFunctionGetEvent,
                                     &FiberGetEvent);
  /* 039 */ lepus::RegisterCFunction(context, kCFunctionGetEvents,
                                     &FiberGetEvents);
  /* 040 */ lepus::RegisterCFunction(context, kCFunctionSetID, &FiberSetID);
  /* 041 */ lepus::RegisterCFunction(context, kCFunctionGetID, &FiberGetID);
  /* 042 */ lepus::RegisterCFunction(context, kCFunctionAddDataset,
                                     &FiberAddDataset);
  /* 043 */ lepus::RegisterCFunction(context, kCFunctionSetDataset,
                                     &FiberSetDataset);
  /* 044 */ lepus::RegisterCFunction(context, kCFunctionGetDataset,
                                     &FiberGetDataset);
  /* 045 */ lepus::RegisterCFunction(context, kCFunctionGetComponentID,
                                     &FiberGetComponentID);
  /* 046 */ lepus::RegisterCFunction(context, kCFunctionUpdateComponentID,
                                     &FiberUpdateComponentID);
  /* 047 */ lepus::RegisterCFunction(context, kCFunctionElementFromBinary,
                                     &FiberElementFromBinary);
  /* 048 */ lepus::RegisterCFunction(context, kCFunctionElementFromBinaryAsync,
                                     &FiberElementFromBinaryAsync);
  /* 049 */ lepus::RegisterCFunction(context, kCFunctionUpdateListCallbacks,
                                     &FiberUpdateListCallbacks);
  /* 050 */ lepus::RegisterCFunction(context, kCFunctionFlushElementTree,
                                     &FiberFlushElementTree);
  /* 051 */ lepus::RegisterCFunction(context, kCFunctionOnLifecycleEvent,
                                     &FiberOnLifecycleEvent);
  /* 052 */ lepus::RegisterCFunction(context, kCFunctionQueryComponent,
                                     &FiberQueryComponent);
  /* 053 */ lepus::RegisterCFunction(context, kCFunctionSetCSSId,
                                     &FiberSetCSSId);
  /* 054 */ lepus::RegisterCFunction(context, kCFunctionSetSourceMapRelease,
                                     &SetSourceMapRelease);
  /* 055 */ lepus::RegisterCFunction(context, kCFuncAddEventListener,
                                     &AddEventListener);
  /* 056 */ lepus::RegisterCFunction(context, kCFuncI18nResourceTranslation,
                                     &I18nResourceTranslation);
  /* 057 */ lepus::RegisterCFunction(context, kCFuncFilterI18nResource,
                                     &FilterI18nResource);
  /* 058 */ lepus::RegisterCFunction(context, kCFuncSendGlobalEvent,
                                     &SendGlobalEvent);
  /* 059 */ lepus::RegisterCFunction(context, kCFunctionReportError,
                                     &ReportError);
  /* 060 */ lepus::RegisterCFunction(context, kCFunctionGetDataByKey,
                                     &FiberGetDataByKey);
  /* 061 */ lepus::RegisterCFunction(context, kCFunctionReplaceElements,
                                     &FiberReplaceElements);
  /* 062 */ lepus::RegisterCFunction(context, kCFunctionQuerySelector,
                                     &FiberQuerySelector);
  /* 063 */ lepus::RegisterCFunction(context, kCFunctionQuerySelectorAll,
                                     &FiberQuerySelectorAll);
  /* 064 */ lepus::RegisterCFunction(context, kCFunctionSetLepusInitData,
                                     &FiberSetLepusInitData);
  /* 065 */ lepus::RegisterCFunction(context, kCFunctionAddConfig,
                                     &FiberAddConfig);
  /* 066 */ lepus::RegisterCFunction(context, kCFunctionSetConfig,
                                     &FiberSetConfig);
  /* 067 */ lepus::RegisterCFunction(context, kCFunctionUpdateComponentInfo,
                                     &FiberUpdateComponentInfo);
  /* 068 */ lepus::RegisterCFunction(context, kCFunctionGetConfig,
                                     &FiberGetElementConfig);
  /* 069 */ lepus::RegisterCFunction(context, kCFunctionGetInlineStyle,
                                     &FiberGetInlineStyle);
  /* 070 */ lepus::RegisterCFunction(context, kCFuncSetGestureDetector,
                                     &FiberSetGestureDetector);
  /* 071 */ lepus::RegisterCFunction(context, kCFuncRemoveGestureDetector,
                                     &FiberRemoveGestureDetector);
  /* 072 */ lepus::RegisterCFunction(context, kCFunctionGetAttributeByName,
                                     &FiberGetAttributeByName);
  /* 073 */ lepus::RegisterCFunction(context, kCFunctionGetAttributeNames,
                                     &FiberGetAttributeNames);
  /* 074 */ lepus::RegisterCFunction(context, kCFunctionGetPageElement,
                                     &FiberGetPageElement);
  /* 075 */ lepus::RegisterCFunction(context, kCFunctionCreateIf,
                                     &FiberCreateIf);
  /* 076 */ lepus::RegisterCFunction(context, kCFunctionCreateFor,
                                     &FiberCreateFor);
  /* 077 */ lepus::RegisterCFunction(context, kCFunctionCreateBlock,
                                     &FiberCreateBlock);
  /* 078 */ lepus::RegisterCFunction(context, kCFunctionUpdateIfNodeIndex,
                                     &FiberUpdateIfNodeIndex);
  /* 079 */ lepus::RegisterCFunction(context, kCFunctionUpdateForChildCount,
                                     &FiberUpdateForChildCount);
  /* 080 */ lepus::RegisterCFunction(context, kCFunctionGetElementByUniqueID,
                                     &FiberGetElementByUniqueID);
  /* 081 */ lepus::RegisterCFunction(context, kCFunctionGetDiffData,
                                     &FiberGetDiffData);
  /* 082 */ lepus::RegisterCFunction(context, kCFunctionLoadLepusChunk,
                                     &LoadLepusChunk);
  /* 083 */ lepus::RegisterCFunction(context, kCFuncSetGestureState,
                                     &FiberSetGestureState);
  /* 084 */ lepus::RegisterCFunction(context, kCFunctionMarkTemplateElement,
                                     &FiberMarkTemplateElement);
  /* 085 */ lepus::RegisterCFunction(context, kCFunctionIsTemplateElement,
                                     &FiberIsTemplateElement);
  /* 086 */ lepus::RegisterCFunction(context, kCFunctionMarkPartElement,
                                     &FiberMarkPartElement);
  /* 087 */ lepus::RegisterCFunction(context, kCFunctionIsPartElement,
                                     &FiberIsPartElement);
  /* 088 */ lepus::RegisterCFunction(context, kCFunctionGetTemplateParts,
                                     &FiberGetTemplateParts);
  /* 089 */ lepus::RegisterCFunction(context, kCFunctionAsyncResolveElement,
                                     &FiberAsyncResolveElement);
  /* 090 */ lepus::RegisterCFunction(context, kCFuncConsumeGesture,
                                     &FiberConsumeGesture);
  /* 091 */ lepus::RegisterCFunction(context,
                                     kCFunctionCreateElementWithProperties,
                                     &FiberCreateElementWithProperties);
  /* 092 */ lepus::RegisterCFunction(context, kCFunctionCreateSignal,
                                     &FiberCreateSignal);
  /* 093 */ lepus::RegisterCFunction(context, kCFunctionWriteSignal,
                                     &FiberWriteSignal);
  /* 094 */ lepus::RegisterCFunction(context, kCFunctionReadSignal,
                                     &FiberReadSignal);
  /* 095 */ lepus::RegisterCFunction(context, kCFunctionCreateComputation,
                                     &FiberCreateComputation);
  /* 096 */ lepus::RegisterCFunction(context, kCFunctionCreateMemo,
                                     &FiberCreateMemo);
  /* 097 */ lepus::RegisterCFunction(context, kCFunctionCreateScope,
                                     &FiberCreateScope);
  /* 098 */ lepus::RegisterCFunction(context, kCFunctionGetScope,
                                     &FiberGetScope);
  /* 099 */ lepus::RegisterCFunction(context, kCFunctionCleanUp, &FiberCleanUp);
  /* 100 */ lepus::RegisterCFunction(context, kCFunctionOnCleanUp,
                                     &FiberOnCleanUp);
  /* 101 */ lepus::RegisterCFunction(context, kCFunctionUnTrack, &FiberUnTrack);
}

void Renderer::RegisterBuiltinForAir(lepus::Context* context) {
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  /* 001 */ lepus::RegisterCFunction(context, kCFunctionAirCreateElement,
                                     &AirCreateElement);
  /* 002 */ lepus::RegisterCFunction(context, kCFunctionAirGetElement,
                                     &AirGetElement);
  /* 003 */ lepus::RegisterCFunction(context, kCFunctionAirCreatePage,
                                     &AirCreatePage);
  /* 004 */ lepus::RegisterCFunction(context, kCFunctionAirCreateComponent,
                                     &AirCreateComponent);
  /* 005 */ lepus::RegisterCFunction(context, kCFunctionAirCreateBlock,
                                     &AirCreateBlock);
  /* 006 */ lepus::RegisterCFunction(context, kCFunctionAirCreateIf,
                                     &AirCreateIf);
  /* 007 */ lepus::RegisterCFunction(context, kCFunctionAirCreateRadonIf,
                                     &AirCreateRadonIf);
  /* 008 */ lepus::RegisterCFunction(context, kCFunctionAirCreateFor,
                                     &AirCreateFor);
  /* 009 */ lepus::RegisterCFunction(context, kCFunctionAirCreatePlug,
                                     &AirCreatePlug);
  /* 010 */ lepus::RegisterCFunction(context, kCFunctionAirCreateSlot,
                                     &AirCreateSlot);
  /* 011 */ lepus::RegisterCFunction(context, kCFunctionAirAppendElement,
                                     &AirAppendElement);
  /* 012 */ lepus::RegisterCFunction(context, kCFunctionAirRemoveElement,
                                     &AirRemoveElement);
  /* 013 */ lepus::RegisterCFunction(context, kCFunctionAirInsertElementBefore,
                                     &AirInsertElementBefore);
  /* 014 */ lepus::RegisterCFunction(context, kCFunctionAirGetElementUniqueID,
                                     &AirGetElementUniqueID);
  /* 015 */ lepus::RegisterCFunction(context, kCFunctionAirGetTag,
                                     &AirGetElementTag);
  /* 016 */ lepus::RegisterCFunction(context, kCFunctionAirSetAttribute,
                                     &AirSetAttribute);
  /* 017 */ lepus::RegisterCFunction(context, kCFunctionAirSetInlineStyles,
                                     &AirSetInlineStyles);
  /* 018 */ lepus::RegisterCFunction(context, kCFunctionAirSetEvent,
                                     &AirSetEvent);
  /* 019 */ lepus::RegisterCFunction(context, kCFunctionAirSetID, &AirSetID);
  /* 020 */ lepus::RegisterCFunction(context, kCFunctionAirGetElementByID,
                                     &AirGetElementByID);
  /* 021 */ lepus::RegisterCFunction(context, kCFunctionAirGetElementByLepusID,
                                     &AirGetElementByLepusID);
  /* 022 */ lepus::RegisterCFunction(context, kCFunctionAirUpdateIfNodeIndex,
                                     &AirUpdateIfNodeIndex);
  /* 023 */ lepus::RegisterCFunction(context, kCFunctionAirUpdateForNodeIndex,
                                     &AirUpdateForNodeIndex);
  /* 024 */ lepus::RegisterCFunction(context, kCFunctionAirUpdateForChildCount,
                                     &AirUpdateForChildCount);
  /* 025 */ lepus::RegisterCFunction(context,
                                     kCFunctionAirGetForNodeChildWithIndex,
                                     &AirGetForNodeChildWithIndex);
  /* 026 */ lepus::RegisterCFunction(context, kCFunctionAirPushForNode,
                                     &AirPushForNode);
  /* 027 */ lepus::RegisterCFunction(context, kCFunctionAirPopForNode,
                                     &AirPopForNode);
  /* 028 */ lepus::RegisterCFunction(
      context, kCFunctionAirGetChildElementByIndex, &AirGetChildElementByIndex);
  /* 029 */ lepus::RegisterCFunction(context, kCFunctionAirPushAirDynamicNode,
                                     &AirPushDynamicNode);
  /* 030 */ lepus::RegisterCFunction(context, kCFunctionAirGetAirDynamicNode,
                                     &AirGetDynamicNode);
  /* 031 */ lepus::RegisterCFunction(context, kCFunctionAirSetAirComponentProp,
                                     &AirSetComponentProp);
  /* 032 */ lepus::RegisterCFunction(
      context, kCFunctionAirRenderComponentInLepus, &AirRenderComponentInLepus);
  /* 033 */ lepus::RegisterCFunction(
      context, kCFunctionAirUpdateComponentInLepus, &AirUpdateComponentInLepus);
  /* 034 */ lepus::RegisterCFunction(context, kCFunctionAirGetComponentInfo,
                                     &AirGetComponentInfo);
  /* 035 */ lepus::RegisterCFunction(context, kCFunctionAirUpdateComponentInfo,
                                     &AirUpdateComponentInfo);
  /* 036 */ lepus::RegisterCFunction(context, kCFunctionAirGetData,
                                     &AirGetData);
  /* 037 */ lepus::RegisterCFunction(context, kCFunctionAirGetProps,
                                     &AirGetProps);
  /* 038 */ lepus::RegisterCFunction(context, kCFunctionAirSetData,
                                     &AirSetData);
  /* 039 */ lepus::RegisterCFunction(context, kCFunctionAirFlushElement,
                                     &AirFlushElement);
  /* 040 */ lepus::RegisterCFunction(context, kCFunctionAirFlushElementTree,
                                     &AirFlushElementTree);
  /* 041 */ lepus::RegisterCFunction(context, kCFunctionTriggerLepusBridge,
                                     &TriggerLepusBridge);
  /* 042 */ lepus::RegisterCFunction(context, kCFunctionTriggerLepusBridgeSync,
                                     &TriggerLepusBridgeSync);
  /* 043 */ lepus::RegisterCFunction(context, kCFunctionAirSetDataSet,
                                     &AirSetDataSet);
  /* 044 */ lepus::RegisterCFunction(context, kCFunctionAirSendGlobalEvent,
                                     &AirSendGlobalEvent);
  /* 045 */ lepus::RegisterCFunction(context, kCFunctionSetTimeout,
                                     &SetTimeout);
  /* 046 */ lepus::RegisterCFunction(context, kCFunctionClearTimeout,
                                     &ClearTimeout);
  /* 047 */ lepus::RegisterCFunction(context, kCFunctionSetTimeInterval,
                                     &SetInterval);
  /* 048 */ lepus::RegisterCFunction(context, kCFunctionClearTimeInterval,
                                     &ClearTimeInterval);
  /* 049 */ lepus::RegisterCFunction(context, kCFuncAddEventListener,
                                     &AddEventListener);
  /* 050 */ lepus::RegisterCFunction(context, kCFuncRegisterDataProcessor,
                                     &RegisterDataProcessor);
  /* 051 */ lepus::RegisterCFunction(context, kCFunctionAirGetElementByUniqueID,
                                     &AirGetElementByUniqueID);
  /* 052 */ lepus::RegisterCFunction(context, kCFunctionAirGetRootElement,
                                     &AirGetRootElement);
  /* 053 */ lepus::RegisterCFunction(context, kCFunctionRemoveEventListener,
                                     &RemoveEventListener);
  /* 054 */ lepus::RegisterCFunction(context, kCFunctionTriggerComponentEvent,
                                     &TriggerComponentEvent);
  /* 055 */ lepus::RegisterCFunction(context, kCFunctionAirCreateRawText,
                                     &AirCreateRawText);
  /* 056 */ lepus::RegisterCFunction(context, kCFunctionAirSetClasses,
                                     &AirSetClasses);
  /* 057 */ lepus::RegisterCFunction(context, kCFunctionAirPushComponentNode,
                                     &AirPushComponentNode);
  /* 058 */ lepus::RegisterCFunction(context, kCFunctionAirPopComponentNode,
                                     &AirPopComponentNode);
  /* 059 */ lepus::RegisterCFunction(context, kCFunctionAirGetParentForNode,
                                     &AirGetParentForNode);
  /* 060 */ lepus::RegisterCFunction(context, kCFunctionReportError,
                                     &ReportError);
  /* 061 */ lepus::RegisterCFunction(context, kCFunctionAirFlushTree,
                                     &AirFlushTree);
  /* 062 */ lepus::RegisterCFunction(context, kCFunctionInvokeUIMethod,
                                     &InvokeUIMethod);
  /* 063 */ lepus::RegisterCFunction(context, runtime::kAddReporterCustomInfo,
                                     &LynxAddReporterCustomInfo);
}

}  // namespace tasm
}  // namespace lynx
