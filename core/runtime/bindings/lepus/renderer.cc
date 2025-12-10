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

static ALLOW_UNUSED_TYPE lepus::Value SlotFunction(lepus::Context* context,
                                                   lepus::Value*, int size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SLOT_FUNCTION);
  return lepus::Value();
}

void Renderer::RegisterCFunctionForRadon(lepus::Context* context) {
  context->RegisterGlobalCFunction(kCFuncIndexOf, &RendererFunctions::IndexOf);
  context->RegisterGlobalCFunction(kCFuncGetLength,
                                   &RendererFunctions::GetLength);
  context->RegisterGlobalCFunction(kCFuncSetValueToMap,
                                   &RendererFunctions::SetValueToMap);
  // clang-format off
  /* To add a RenderFunction, it needs to be registered first to avoid conflicts across different branches. */
  /* 001 */ context->RegisterGlobalCFunction(kCFuncCreatePage, &RendererFunctions::CreateVirtualPage);
  /* 002 */ context->RegisterGlobalCFunction(kCFuncAttachPage, &RendererFunctions::AttachPage);
  /* 003 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualComponent,
                           &RendererFunctions::CreateVirtualComponent);
  /* 004 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualNode,
                           &RendererFunctions::CreateVirtualNode);
  /* 005 */ context->RegisterGlobalCFunction(kCFuncAppendChild, &RendererFunctions::AppendChild);
  /* 006 */ context->RegisterGlobalCFunction(kCFuncSetClassTo, &RendererFunctions::SetClassTo);
  /* 007 */ context->RegisterGlobalCFunction(kCFuncSetStyleTo, &RendererFunctions::SetStyleTo);
  /* 008 */ context->RegisterGlobalCFunction(kCFuncSetEventTo, &RendererFunctions::SetEventTo);
  /* 009 */ context->RegisterGlobalCFunction(kCFuncSetAttributeTo, &RendererFunctions::SetAttributeTo);
  /* 010 */ context->RegisterGlobalCFunction(kCFuncSetStaticClassTo, &RendererFunctions::SetStaticClassTo);
  /* 011 */ context->RegisterGlobalCFunction(kCFuncSetStaticStyleTo, &RendererFunctions::SetStaticStyleTo);
  /* 012 */ context->RegisterGlobalCFunction(kCFuncSetStaticAttributeTo, &RendererFunctions::SetStaticAttrTo);
  /* 013 */ context->RegisterGlobalCFunction(kCFuncSetDataSetTo, &RendererFunctions::SetDataSetTo);
  /* 014 */ context->RegisterGlobalCFunction(kCFuncSetStaticEventTo, &RendererFunctions::SetStaticEventTo);
  /* 015 */ context->RegisterGlobalCFunction(kCFuncSetId, &RendererFunctions::SetId);
  /* 016 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualSlot, &RendererFunctions::CreateSlot);
  /* 017 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualPlug,
                           &RendererFunctions::CreateVirtualPlug);
  /* 018 */ context->RegisterGlobalCFunction(kCFuncMarkComponentHasRenderer,
                           &RendererFunctions::MarkComponentHasRenderer);
  /* 019 */ context->RegisterGlobalCFunction(kCFuncSetProp, &RendererFunctions::SetProp);
  /* 020 */ context->RegisterGlobalCFunction(kCFuncSetData, &RendererFunctions::SetData);
  /* 021 */ context->RegisterGlobalCFunction(kCFuncAddPlugToComponent,
                           &RendererFunctions::AddVirtualPlugToComponent);
  /* 022 */ context->RegisterGlobalCFunction(kCFuncGetComponentData, &RendererFunctions::GetComponentData);
  /* 023 */ context->RegisterGlobalCFunction(kCFuncGetComponentProps,
                           &RendererFunctions::GetComponentProps);
  /* 024 */ context->RegisterGlobalCFunction(kCFuncSetDynamicStyleTo,
                           &RendererFunctions::SetDynamicStyleTo);
  /* 025 */ context->RegisterGlobalCFunction(kCFuncGetLazyLoadCount, &RendererFunctions::ThemedTranslationLegacy);
  /* 026 */ context->RegisterGlobalCFunction(kCFuncUpdateComponentInfo,
                           &RendererFunctions::UpdateComponentInfo);
  /* 027 */ context->RegisterGlobalCFunction(kCFuncGetComponentInfo, &RendererFunctions::GetComponentInfo);
  /* 028 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualListNode,
                           &RendererFunctions::CreateVirtualListNode);
  /* 029 */ context->RegisterGlobalCFunction(kCFuncAppendListComponentInfo,
                           &RendererFunctions::AppendListComponentInfo);
  /* 030 */ context->RegisterGlobalCFunction(kCFuncSetListRefreshComponentInfo,
                           &SlotFunction);
  /* 031 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualComponentByName,
                           &RendererFunctions::CreateComponentByName);
  /* 032 */ context->RegisterGlobalCFunction(kCFuncCreateDynamicVirtualComponent,
                           &RendererFunctions::CreateDynamicVirtualComponent);
  /* 033 */ context->RegisterGlobalCFunction(kCFuncRenderDynamicComponent,
                           &RendererFunctions::RenderDynamicComponent);
  /* 034 */ context->RegisterGlobalCFunction(kCFuncThemedTranslation,
                           &RendererFunctions::ThemedTranslation);
  /* 035 */ context->RegisterGlobalCFunction(kCFuncRegisterDataProcessor,
                           &RendererFunctions::RegisterDataProcessor);
  /* 036 */ context->RegisterGlobalCFunction(kCFuncThemedLangTranslation,
                           &RendererFunctions::ThemedLanguageTranslation);
  /* 037 */ context->RegisterGlobalCFunction(kCFuncGetComponentContextData,
                               &RendererFunctions::GetComponentContextData);
  /* 038 */ context->RegisterGlobalCFunction(kCFuncProcessComponentData, &RendererFunctions::ProcessComponentData);
  /* 039 */ context->RegisterGlobalCFunction("__slot__39", &SlotFunction);
  /* 040 */ context->RegisterGlobalCFunction("__slot__40", &SlotFunction);
  /* 041 */ context->RegisterGlobalCFunction("__slot__41", &SlotFunction);
  /* 042 */ context->RegisterGlobalCFunction("__slot__42", &SlotFunction);
  /* 043 */ context->RegisterGlobalCFunction("__slot__43", &SlotFunction);
  /* 044 */ context->RegisterGlobalCFunction("__slot__44", &SlotFunction);
  /* 045 */ context->RegisterGlobalCFunction("__slot__45", &SlotFunction);
  /* 046 */ context->RegisterGlobalCFunction("__slot__46", &SlotFunction);
  /* 047 */ context->RegisterGlobalCFunction("__slot__47", &SlotFunction);
  /* 048 */ context->RegisterGlobalCFunction("__slot__48", &SlotFunction);
  /* 049 */ context->RegisterGlobalCFunction("__slot__49", &SlotFunction);
  /* 050 */ context->RegisterGlobalCFunction("__slot__50", &SlotFunction);
  /* 051 */ context->RegisterGlobalCFunction("__slot__51", &SlotFunction);
  // warning: double 51.
  /* 051 */ context->RegisterGlobalCFunction("__slot__51_1", &SlotFunction);
  /* 052 */ context->RegisterGlobalCFunction("__slot__52", &SlotFunction);
  /* 053 */ context->RegisterGlobalCFunction("__slot__53", &SlotFunction);
  /* 054 */ context->RegisterGlobalCFunction("__slot__54", &SlotFunction);
  /* 055 */ context->RegisterGlobalCFunction("__slot__55", &SlotFunction);
  /* 056 */ context->RegisterGlobalCFunction("__slot__56", &SlotFunction);
  /* 057 */ context->RegisterGlobalCFunction("__slot__57", &SlotFunction);
  /* 058 */ context->RegisterGlobalCFunction("__slot__58", &SlotFunction);
  /* 059 */ context->RegisterGlobalCFunction("__slot__59", &SlotFunction);
  /* 060 */ context->RegisterGlobalCFunction("__slot__60", &SlotFunction);
  /* 061 */ context->RegisterGlobalCFunction("__slot__61", &SlotFunction);
  /* 062 */ context->RegisterGlobalCFunction("__slot__62", &SlotFunction);
  /* 063 */ context->RegisterGlobalCFunction("__slot__63", &SlotFunction);
  /* 064 */ context->RegisterGlobalCFunction("__slot__64", &SlotFunction);
  /* 065 */ context->RegisterGlobalCFunction("__slot__65", &SlotFunction);
  /* 066 */ context->RegisterGlobalCFunction("__slot__66", &SlotFunction);
  /* 067 */ context->RegisterGlobalCFunction("__slot__67", &SlotFunction);
  /* 068 */ context->RegisterGlobalCFunction("__slot__68", &SlotFunction);
  /* 069 */ context->RegisterGlobalCFunction("__slot__69", &SlotFunction);
  /* 070 */ context->RegisterGlobalCFunction("__slot__70", &SlotFunction);
  /* 071 */ context->RegisterGlobalCFunction("__slot__71", &SlotFunction);
  /* 072 */ context->RegisterGlobalCFunction("__slot__72", &SlotFunction);
  /* 073 */ context->RegisterGlobalCFunction("__slot__73", &SlotFunction);
  /* 074 */ context->RegisterGlobalCFunction("__slot__74", &SlotFunction);
  /* 075 */ context->RegisterGlobalCFunction(kCFuncSetStaticStyleToByFiber, &RendererFunctions::SetStaticStyleTo2);
  /* 076 */ context->RegisterGlobalCFunction("__slot__76", &SlotFunction);
  /* 077 */ context->RegisterGlobalCFunction("__slot__77", &SlotFunction);
  /* 078 */ context->RegisterGlobalCFunction("__slot__78", &SlotFunction);
  /* 079 */ context->RegisterGlobalCFunction("__slot__79", &SlotFunction);
  /* 080 */ context->RegisterGlobalCFunction("__slot__80", &SlotFunction);
  /* 081 */ context->RegisterGlobalCFunction(kCFuncSetContextData, &RendererFunctions::SetContextData);
  /* 082 */ context->RegisterGlobalCFunction(kCFuncSetScriptEventTo, &RendererFunctions::SetScriptEventTo);
  /* 083 */ context->RegisterGlobalCFunction(kCFuncRegisterElementWorklet, &RendererFunctions::RegisterElementWorklet);
  /* 084 */ context->RegisterGlobalCFunction(kCFuncCreateVirtualPlugWithComponent, &RendererFunctions::CreateVirtualPlugWithComponent);
  /* 085 */ context->RegisterGlobalCFunction("__slot__85", &SlotFunction);
  /* 086 */ context->RegisterGlobalCFunction(kCFuncAddEventListener, &RendererFunctions::AddEventListener);
  /* 087 */ context->RegisterGlobalCFunction(kCFuncI18nResourceTranslation, &RendererFunctions::I18nResourceTranslation);
  /* 088 */ context->RegisterGlobalCFunction(kCFuncReFlushPage, &RendererFunctions::ReFlushPage);
  /* 089 */ context->RegisterGlobalCFunction(kCFuncSetComponent, &RendererFunctions::SetComponent);
  /* 090 */ context->RegisterGlobalCFunction(kCFuncGetGlobalProps, &RendererFunctions::GetGlobalProps);
  /* 091 */ context->RegisterGlobalCFunction("__slot__91", &SlotFunction);
  /* 092 */ context->RegisterGlobalCFunction(kCFuncAppendSubTree, &RendererFunctions::AppendSubTree);
  /* 093 */ context->RegisterGlobalCFunction(kCFuncHandleExceptionInLepus, &RendererFunctions::HandleExceptionInLepus);
  /* 094 */ context->RegisterGlobalCFunction(kCFuncAppendVirtualPlugToComponent, &RendererFunctions::AppendVirtualPlugToComponent);
  /* 095 */ context->RegisterGlobalCFunction(kCFuncMarkPageElement, &RendererFunctions::MarkPageElement);
  /* 096 */ context->RegisterGlobalCFunction(kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource);
  /* 097 */ context->RegisterGlobalCFunction(kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent);
  /* 098 */ context->RegisterGlobalCFunction(kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease);
  /* 099 */ context->RegisterGlobalCFunction(kCFuncCloneSubTree, &RendererFunctions::CloneSubTree);
  /* 100 */ context->RegisterGlobalCFunction(kCFuncGetSystemInfo, &RendererFunctions::GetSystemInfo);
  /* 101 */ context->RegisterGlobalCFunction(kCFuncAddFallbackToDynamicComponent, &RendererFunctions::AddFallbackToDynamicComponent);
  /* 102 */ context->RegisterGlobalCFunction(kCFuncCreateGestureDetector, &RendererFunctions::CreateGestureDetector);
  /* 103 */ context->RegisterGlobalCFunction(kCFunctionElementAnimate, &RendererFunctions::ElementAnimate);
  // clang-format on
}

void Renderer::RegisterCFunctionForFiber(lepus::Context* context) {
  context->RegisterGlobalCFunction(kCFuncIndexOf, &RendererFunctions::IndexOf);
  context->RegisterGlobalCFunction(kCFuncGetLength,
                                   &RendererFunctions::GetLength);
  context->RegisterGlobalCFunction(kCFuncSetValueToMap,
                                   &RendererFunctions::SetValueToMap);
  // clang-format off
  // To add a RenderFunction, it needs to be registered first to avoid conflicts
  // across different branches.
  /* 001 */ context->RegisterGlobalCFunction(kCFunctionCreateElement, &RendererFunctions::FiberCreateElement);
  /* 002 */ context->RegisterGlobalCFunction(kCFunctionCreatePage, &RendererFunctions::FiberCreatePage);
  /* 003 */ context->RegisterGlobalCFunction(kCFunctionCreateComponent, &RendererFunctions::FiberCreateComponent);
  /* 004 */ context->RegisterGlobalCFunction(kCFunctionCreateView, &RendererFunctions::FiberCreateView);
  /* 005 */ context->RegisterGlobalCFunction(kCFunctionCreateList, &RendererFunctions::FiberCreateList);
  /* 006 */ context->RegisterGlobalCFunction(kCFunctionCreateScrollView, &RendererFunctions::FiberCreateScrollView);
  /* 007 */ context->RegisterGlobalCFunction(kCFunctionCreateText, &RendererFunctions::FiberCreateText);
  /* 008 */ context->RegisterGlobalCFunction(kCFunctionCreateImage, &RendererFunctions::FiberCreateImage);
  /* 009 */ context->RegisterGlobalCFunction(kCFunctionCreateRawText, &RendererFunctions::FiberCreateRawText);
  /* 010 */ context->RegisterGlobalCFunction(kCFunctionCreateNonElement, &RendererFunctions::FiberCreateNonElement);
  /* 011 */ context->RegisterGlobalCFunction(kCFunctionCreateWrapperElement, &RendererFunctions::FiberCreateWrapperElement);
  /* 012 */ context->RegisterGlobalCFunction(kCFunctionAppendElement, &RendererFunctions::FiberAppendElement);
  /* 013 */ context->RegisterGlobalCFunction(kCFunctionRemoveElement, &RendererFunctions::FiberRemoveElement);
  /* 014 */ context->RegisterGlobalCFunction(kCFunctionInsertElementBefore, &RendererFunctions::FiberInsertElementBefore);
  /* 015 */ context->RegisterGlobalCFunction(kCFunctionFirstElement, &RendererFunctions::FiberFirstElement);
  /* 016 */ context->RegisterGlobalCFunction(kCFunctionLastElement, &RendererFunctions::FiberLastElement);
  /* 017 */ context->RegisterGlobalCFunction(kCFunctionNextElement, &RendererFunctions::FiberNextElement);
  /* 018 */ context->RegisterGlobalCFunction(kCFunctionReplaceElement, &RendererFunctions::FiberReplaceElement);
  /* 019 */ context->RegisterGlobalCFunction(kCFunctionSwapElement, &RendererFunctions::FiberSwapElement);
  /* 020 */ context->RegisterGlobalCFunction(kCFunctionGetParent, &RendererFunctions::FiberGetParent);
  /* 021 */ context->RegisterGlobalCFunction(kCFunctionGetChildren, &RendererFunctions::FiberGetChildren);
  /* 022 */ context->RegisterGlobalCFunction(kCFunctionCloneElement, &RendererFunctions::FiberCloneElement);
  /* 023 */ context->RegisterGlobalCFunction(kCFunctionElementIsEqual, &RendererFunctions::FiberElementIsEqual);
  /* 024 */ context->RegisterGlobalCFunction(kCFunctionGetElementUniqueID, &RendererFunctions::FiberGetElementUniqueID);
  /* 025 */ context->RegisterGlobalCFunction(kCFunctionGetTag, &RendererFunctions::FiberGetTag);
  /* 026 */ context->RegisterGlobalCFunction(kCFunctionSetAttribute, &RendererFunctions::FiberSetAttribute);
  /* 027 */ context->RegisterGlobalCFunction(kCFunctionGetAttributes, &RendererFunctions::FiberGetAttributes);
  /* 028 */ context->RegisterGlobalCFunction(kCFunctionAddClass, &RendererFunctions::FiberAddClass);
  /* 029 */ context->RegisterGlobalCFunction(kCFunctionSetClasses, &RendererFunctions::FiberSetClasses);
  /* 030 */ context->RegisterGlobalCFunction(kCFunctionGetClasses, &RendererFunctions::FiberGetClasses);
  /* 031 */ context->RegisterGlobalCFunction(kCFunctionAddInlineStyle, &RendererFunctions::FiberAddInlineStyle);
  /* 032 */ context->RegisterGlobalCFunction(kCFunctionSetInlineStyles, &RendererFunctions::FiberSetInlineStyles);
  /* 033 */ context->RegisterGlobalCFunction(kCFunctionGetInlineStyles, &RendererFunctions::FiberGetInlineStyles);
  /* 034 */ context->RegisterGlobalCFunction(kCFunctionSetParsedStyles, &RendererFunctions::FiberSetParsedStyles);
  /* 035 */ context->RegisterGlobalCFunction(kCFunctionGetComputedStyles, &RendererFunctions::FiberGetComputedStyles);
  /* 036 */ context->RegisterGlobalCFunction(kCFunctionAddEvent, &RendererFunctions::FiberAddEvent);
  /* 037 */ context->RegisterGlobalCFunction(kCFunctionSetEvents, &RendererFunctions::FiberSetEvents);
  /* 038 */ context->RegisterGlobalCFunction(kCFunctionGetEvent, &RendererFunctions::FiberGetEvent);
  /* 039 */ context->RegisterGlobalCFunction(kCFunctionGetEvents, &RendererFunctions::FiberGetEvents);
  /* 040 */ context->RegisterGlobalCFunction(kCFunctionSetID, &RendererFunctions::FiberSetID);
  /* 041 */ context->RegisterGlobalCFunction(kCFunctionGetID, &RendererFunctions::FiberGetID);
  /* 042 */ context->RegisterGlobalCFunction(kCFunctionAddDataset, &RendererFunctions::FiberAddDataset);
  /* 043 */ context->RegisterGlobalCFunction(kCFunctionSetDataset, &RendererFunctions::FiberSetDataset);
  /* 044 */ context->RegisterGlobalCFunction(kCFunctionGetDataset, &RendererFunctions::FiberGetDataset);
  /* 045 */ context->RegisterGlobalCFunction(kCFunctionGetComponentID, &RendererFunctions::FiberGetComponentID);
  /* 046 */ context->RegisterGlobalCFunction(kCFunctionUpdateComponentID, &RendererFunctions::FiberUpdateComponentID);
  /* 047 */ context->RegisterGlobalCFunction(kCFunctionElementFromBinary, &RendererFunctions::FiberElementFromBinary);
  /* 048 */ context->RegisterGlobalCFunction(kCFunctionElementFromBinaryAsync, &RendererFunctions::FiberElementFromBinaryAsync);
  /* 049 */ context->RegisterGlobalCFunction(kCFunctionUpdateListCallbacks, &RendererFunctions::FiberUpdateListCallbacks);
  /* 050 */ context->RegisterGlobalCFunction(kCFunctionFlushElementTree, &RendererFunctions::FiberFlushElementTree);
  /* 051 */ context->RegisterGlobalCFunction(kCFunctionOnLifecycleEvent, &RendererFunctions::FiberOnLifecycleEvent);
  /* 052 */ context->RegisterGlobalCFunction(kCFunctionQueryComponent, &RendererFunctions::FiberQueryComponent);
  /* 053 */ context->RegisterGlobalCFunction(kCFunctionSetCSSId, &RendererFunctions::FiberSetCSSId);
  /* 054 */ context->RegisterGlobalCFunction(kCFunctionSetSourceMapRelease, &RendererFunctions::SetSourceMapRelease);
  /* 055 */ context->RegisterGlobalCFunction(kCFuncAddEventListener, &RendererFunctions::AddEventListener);
  /* 056 */ context->RegisterGlobalCFunction(kCFuncI18nResourceTranslation, &RendererFunctions::I18nResourceTranslation);
  /* 057 */ context->RegisterGlobalCFunction(kCFuncFilterI18nResource, &RendererFunctions::FilterI18nResource);
  /* 058 */ context->RegisterGlobalCFunction(kCFuncSendGlobalEvent, &RendererFunctions::SendGlobalEvent);
  /* 059 */ context->RegisterGlobalCFunction(kCFunctionReportError, &RendererFunctions::ReportError);
  /* 060 */ context->RegisterGlobalCFunction(kCFunctionGetDataByKey, &RendererFunctions::FiberGetDataByKey);
  /* 061 */ context->RegisterGlobalCFunction(kCFunctionReplaceElements, &RendererFunctions::FiberReplaceElements);
  /* 062 */ context->RegisterGlobalCFunction(kCFunctionQuerySelector, &RendererFunctions::FiberQuerySelector);
  /* 063 */ context->RegisterGlobalCFunction(kCFunctionQuerySelectorAll, &RendererFunctions::FiberQuerySelectorAll);
  /* 064 */ context->RegisterGlobalCFunction(kCFunctionSetLepusInitData, &RendererFunctions::FiberSetLepusInitData);
  /* 065 */ context->RegisterGlobalCFunction(kCFunctionAddConfig, &RendererFunctions::FiberAddConfig);
  /* 066 */ context->RegisterGlobalCFunction(kCFunctionSetConfig, &RendererFunctions::FiberSetConfig);
  /* 067 */ context->RegisterGlobalCFunction(kCFunctionUpdateComponentInfo, &RendererFunctions::FiberUpdateComponentInfo);
  /* 068 */ context->RegisterGlobalCFunction(kCFunctionGetConfig, &RendererFunctions::FiberGetElementConfig);
  /* 069 */ context->RegisterGlobalCFunction(kCFunctionGetInlineStyle, &RendererFunctions::FiberGetInlineStyle);
  /* 070 */ context->RegisterGlobalCFunction(kCFuncSetGestureDetector, &RendererFunctions::FiberSetGestureDetector);
  /* 071 */ context->RegisterGlobalCFunction(kCFuncRemoveGestureDetector, &RendererFunctions::FiberRemoveGestureDetector);
  /* 072 */ context->RegisterGlobalCFunction(kCFunctionGetAttributeByName, &RendererFunctions::FiberGetAttributeByName);
  /* 073 */ context->RegisterGlobalCFunction(kCFunctionGetAttributeNames, &RendererFunctions::FiberGetAttributeNames);
  /* 074 */ context->RegisterGlobalCFunction(kCFunctionGetPageElement, &RendererFunctions::FiberGetPageElement);
  /* 075 */ context->RegisterGlobalCFunction(kCFunctionCreateIf, &RendererFunctions::FiberCreateIf);
  /* 076 */ context->RegisterGlobalCFunction(kCFunctionCreateFor, &RendererFunctions::FiberCreateFor);
  /* 077 */ context->RegisterGlobalCFunction(kCFunctionCreateBlock, &RendererFunctions::FiberCreateBlock);
  /* 078 */ context->RegisterGlobalCFunction(kCFunctionUpdateIfNodeIndex, &RendererFunctions::FiberUpdateIfNodeIndex);
  /* 079 */ context->RegisterGlobalCFunction(kCFunctionUpdateForChildCount, &RendererFunctions::FiberUpdateForChildCount);
  /* 080 */ context->RegisterGlobalCFunction(kCFunctionGetElementByUniqueID, &RendererFunctions::FiberGetElementByUniqueID);
  /* 081 */ context->RegisterGlobalCFunction(kCFunctionGetDiffData, &RendererFunctions::FiberGetDiffData);
  /* 082 */ context->RegisterGlobalCFunction(kCFunctionLoadLepusChunk, &RendererFunctions::LoadLepusChunk);
  /* 083 */ context->RegisterGlobalCFunction(kCFuncSetGestureState, &RendererFunctions::FiberSetGestureState);
  /* 084 */ context->RegisterGlobalCFunction(kCFunctionMarkTemplateElement, &RendererFunctions::FiberMarkTemplateElement);
  /* 085 */ context->RegisterGlobalCFunction(kCFunctionIsTemplateElement, &RendererFunctions::FiberIsTemplateElement);
  /* 086 */ context->RegisterGlobalCFunction(kCFunctionMarkPartElement, &RendererFunctions::FiberMarkPartElement);
  /* 087 */ context->RegisterGlobalCFunction(kCFunctionIsPartElement, &RendererFunctions::FiberIsPartElement);
  /* 088 */ context->RegisterGlobalCFunction(kCFunctionGetTemplateParts, &RendererFunctions::FiberGetTemplateParts);
  /* 089 */ context->RegisterGlobalCFunction(kCFunctionAsyncResolveElement, &RendererFunctions::FiberAsyncResolveElement);
  /* 090 */ context->RegisterGlobalCFunction(kCFuncConsumeGesture, &RendererFunctions::FiberConsumeGesture);
  /* 091 */ context->RegisterGlobalCFunction(kCFunctionCreateElementWithProperties, &RendererFunctions::FiberCreateElementWithProperties);
  /* 092 */ context->RegisterGlobalCFunction(kCFunctionCreateSignal, &RendererFunctions::FiberCreateSignal);
  /* 093 */ context->RegisterGlobalCFunction(kCFunctionWriteSignal, &RendererFunctions::FiberWriteSignal);
  /* 094 */ context->RegisterGlobalCFunction(kCFunctionReadSignal, &RendererFunctions::FiberReadSignal);
  /* 095 */ context->RegisterGlobalCFunction(kCFunctionCreateComputation, &RendererFunctions::FiberCreateComputation);
  /* 096 */ context->RegisterGlobalCFunction(kCFunctionCreateMemo, &RendererFunctions::FiberCreateMemo);
  /* 097 */ context->RegisterGlobalCFunction(kCFunctionCreateScope, &RendererFunctions::FiberCreateScope);
  /* 098 */ context->RegisterGlobalCFunction(kCFunctionGetScope, &RendererFunctions::FiberGetScope);
  /* 099 */ context->RegisterGlobalCFunction(kCFunctionCleanUp, &RendererFunctions::FiberCleanUp);
  /* 100 */ context->RegisterGlobalCFunction(kCFunctionOnCleanUp, &RendererFunctions::FiberOnCleanUp);
  /* 101 */ context->RegisterGlobalCFunction(kCFunctionUnTrack, &RendererFunctions::FiberUnTrack);
  /* 102 */ context->RegisterGlobalCFunction(kCFunctionCreateFrame, &RendererFunctions::FiberCreateFrame);
  /* 103 */ context->RegisterGlobalCFunction(kCFunctionRunUpdates, &RendererFunctions::FiberRunUpdates);
  /* 104 */ context->RegisterGlobalCFunction(kCFunctionCreateStyleObject, &RendererFunctions::CreateStyleObject);
  /* 105 */ context->RegisterGlobalCFunction(kCFunctionSetStyleObject, &RendererFunctions::SetStyleObject);
  /* 106 */ context->RegisterGlobalCFunction(kCFunctionUpdateStyleObject, &RendererFunctions::UpdateStyleObject);
  /* 107 */ context->RegisterGlobalCFunction(kCFunctionAsyncResolveSubtreeProperty, &RendererFunctions::FiberAsyncResolveSubtreeProperty);
  /* 108 */ context->RegisterGlobalCFunction(kCFunctionMarkAsyncFlushRoot, &RendererFunctions::FiberMarkAsyncResolveRoot);
  /* 109 */ context->RegisterGlobalCFunction(kCFunctionAddEventListener, &RendererFunctions::FiberAddEventListener);
  /* 110 */ context->RegisterGlobalCFunction(kCFunctionFiberRemoveEventListener, &RendererFunctions::FiberRemoveEventListener);
  /* 111 */ context->RegisterGlobalCFunction(kCFunctionCreateEvent, &RendererFunctions::FiberCreateEvent);
  /* 112 */ context->RegisterGlobalCFunction(kCFunctionDispatchEvent, &RendererFunctions::FiberDispatchEvent);
  /* 113 */ context->RegisterGlobalCFunction(kCFunctionStopPropagation, &RendererFunctions::FiberStopPropagation);
  /* 114 */ context->RegisterGlobalCFunction(kCFunctionStopImmediatePropagation, &RendererFunctions::FiberStopImmediatePropagation);
  /* 115 */ context->RegisterGlobalCFunction(kCFunctionInvokeUIMethod, &RendererFunctions::InvokeUIMethod);
  /* 116 */ context->RegisterGlobalCFunction(kCFunctionGetComputedStyleByKey, &RendererFunctions::FiberGetComputedStyleByKey);
  // Added in Lynx:3.0
  /* 117 */ context->RegisterGlobalCFunction(kSetTimeout, &RendererFunctions::SetTimeout);
  // Added in Lynx:3.0
  /* 118 */ context->RegisterGlobalCFunction(kClearTimeout, &RendererFunctions::ClearTimeout);
  // Added in Lynx:3.0
  /* 119 */ context->RegisterGlobalCFunction(kSetInterval, &RendererFunctions::SetInterval);
  // Added in Lynx:3.0
  /* 120 */ context->RegisterGlobalCFunction(kClearTimeInterval, &RendererFunctions::ClearTimeInterval);
  // Added in Lynx:3.0
  /* 121 */ context->RegisterGlobalCFunction(kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame);
  // Added in Lynx:3.0
  /* 122 */ context->RegisterGlobalCFunction(kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame);
  // clang-format on
}

void Renderer::RegisterMethodToLynx(lepus::Context* context, lepus::Value& lynx,
                                    const std::string& targetSdkVersion) {
  if (!lynx.IsJSValue() && !lynx.IsTable()) {
    return;
  }

  // clang-format off
  context->RegisterObjectCFunction(lynx, tasm::kGetTextInfo, &RendererFunctions::GetTextInfo);
  context->RegisterObjectCFunction(lynx, kSetTimeout, &RendererFunctions::SetTimeout);
  context->RegisterObjectCFunction(lynx, kClearTimeout, &RendererFunctions::ClearTimeout);
  context->RegisterObjectCFunction(lynx, kSetInterval, &RendererFunctions::SetInterval);
  context->RegisterObjectCFunction(lynx, kClearTimeInterval, &RendererFunctions::ClearTimeInterval);
  context->RegisterObjectCFunction(lynx, kCFunctionTriggerLepusBridge, &RendererFunctions::TriggerLepusBridge);
  context->RegisterObjectCFunction(lynx, kCFunctionTriggerLepusBridgeSync, &RendererFunctions::TriggerLepusBridgeSync);
  context->RegisterObjectCFunction(lynx, kCFunctionTriggerComponentEvent, &RendererFunctions::TriggerComponentEvent);
  context->RegisterObjectCFunction(lynx, runtime::kGetDevTool, &RendererFunctions::GetDevTool);
  context->RegisterObjectCFunction(lynx, runtime::kGetCoreContext, &RendererFunctions::GetCoreContext);
  context->RegisterObjectCFunction(lynx, runtime::kGetJSContext, &RendererFunctions::GetJSContext);
  context->RegisterObjectCFunction(lynx, runtime::kGetUIContext, &RendererFunctions::GetUIContext);
  context->RegisterObjectCFunction(lynx, runtime::kGetNative, &RendererFunctions::GetNative);
  context->RegisterObjectCFunction(lynx, runtime::kGetEngine, &RendererFunctions::GetEngine);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kRequestAnimationFrame, &RendererFunctions::RequestAnimationFrame);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kCancelAnimationFrame, &RendererFunctions::CancelAnimationFrame);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, runtime::kGetCustomSectionSync, &RendererFunctions::GetCustomSectionSync);
  // shared data.
  context->RegisterObjectCFunction(lynx, tasm::kSetSessionStorageItem, &RendererFunctions::SetSessionStorageItem);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, tasm::kGetSessionStorageItem, &RendererFunctions::GetSessionStorageItem);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, runtime::kAddReporterCustomInfo, &RendererFunctions::LynxAddReporterCustomInfo);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kReportError, &RendererFunctions::ReportError);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kLoadScript, &RendererFunctions::LoadScript);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kFetchBundle, &RendererFunctions::FetchBundle);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, kGetModule, &RendererFunctions::GetModule);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, tasm::kStopExposure, &RendererFunctions::StopExposure);
  // Reserved to ensure compatibility.Use global's instead.
  context->RegisterObjectCFunction(lynx, tasm::kResumeExposure, &RendererFunctions::ResumeExposure);
  // clang-format on

  // Timing
  RegisterMethodToLynxPerformance(context, lynx);

  // engine version
  if (!targetSdkVersion.empty()) {
    lynx.SetProperty(BASE_STATIC_STRING(runtime::kTargetSdkVersion),
                     lepus::Value(targetSdkVersion));
  }
}

void Renderer::RegisterMethodToLynxPerformance(lepus::Context* context,
                                               lepus::Value& lynx) {
  lepus::Value perf_obj(lepus::LEPUSValueHelper::CreateObject(context));
  lynx.SetProperty(BASE_STATIC_STRING(runtime::kPerformanceObject), perf_obj);

  context->RegisterObjectCFunction(perf_obj, runtime::kGeneratePipelineOptions,
                                   &RendererFunctions::GeneratePipelineOptions);
  context->RegisterObjectCFunction(perf_obj, runtime::kOnPipelineStart,
                                   &RendererFunctions::OnPipelineStart);
  context->RegisterObjectCFunction(perf_obj, runtime::kMarkTiming,
                                   &RendererFunctions::MarkTiming);
  context->RegisterObjectCFunction(
      perf_obj, runtime::kBindPipelineIDWithTimingFlag,
      &RendererFunctions::BindPipelineIDWithTimingFlag);
  context->RegisterObjectCFunction(perf_obj, runtime::kAddTimingListener,
                                   &RendererFunctions::AddTimingListener);

  context->RegisterObjectCFunction(perf_obj, runtime::kProfileStart,
                                   &RendererFunctions::ProfileStart);
  context->RegisterObjectCFunction(perf_obj, runtime::kProfileEnd,
                                   &RendererFunctions::ProfileEnd);
  context->RegisterObjectCFunction(perf_obj, runtime::kProfileMark,
                                   &RendererFunctions::ProfileMark);
  context->RegisterObjectCFunction(perf_obj, runtime::kProfileFlowId,
                                   &RendererFunctions::ProfileFlowId);
  context->RegisterObjectCFunction(perf_obj, runtime::kIsProfileRecording,
                                   &RendererFunctions::IsProfileRecording);
}

void Utils::RegisterMethodToResponseHandler(lepus::Context* context,
                                            lepus::Value& response_handler) {
  if (!response_handler.IsJSValue() && !response_handler.IsTable()) {
    return;
  }

  context->RegisterObjectCFunction(response_handler, runtime::kWait,
                                   &RendererFunctions::WaitingForResponse);
  context->RegisterObjectCFunction(response_handler, runtime::kThen,
                                   &RendererFunctions::AddListenerForResponse);
}

void Utils::RegisterMethodToContextProxy(lepus::Context* context,
                                         lepus::Value& target,
                                         runtime::ContextProxy::Type type) {
  if (!target.IsJSValue() && !target.IsTable()) {
    return;
  }

  context->RegisterObjectCFunction(target, runtime::kPostMessage,
                                   &RendererFunctions::PostMessage);

  context->RegisterObjectCFunction(target, runtime::kDispatchEvent,
                                   &RendererFunctions::DispatchEvent);
  context->RegisterObjectCFunction(target, runtime::kAddEventListener,
                                   &RendererFunctions::RuntimeAddEventListener);
  context->RegisterObjectCFunction(
      target, runtime::kRemoveEventListener,
      &RendererFunctions::RuntimeRemoveEventListener);

  if (type == runtime::ContextProxy::Type::kDevTool) {
    context->RegisterObjectCFunction(
        target, runtime::kReplaceStyleSheetByIdWithBase64,
        &RendererFunctions::ReplaceStyleSheetByIdWithBase64);
    context->RegisterObjectCFunction(target, runtime::kRemoveStyleSheetById,
                                     &RendererFunctions::RemoveStyleSheetById);
  }
}

void Utils::RegisterMethodToLepusModule(lepus::Context* context,
                                        lepus::Value& lepus_module) {
  if (!lepus_module.IsJSValue() && !lepus_module.IsTable()) {
    return;
  }

  if (lepus_module.IsTable()) {
    context->RegisterObjectCFunction(lepus_module, runtime::kInvoke,
                                     &RendererFunctions::InvokeModuleMethod);
  }
}

void Utils::RegisterMethodToGestureManager(lepus::Context* context,
                                           lepus::Value& gesture_manager) {
  if (!gesture_manager.IsJSValue() && !gesture_manager.IsTable()) {
    return;
  }

  context->RegisterObjectCFunction(gesture_manager, kCFuncSetGestureState,
                                   &RendererFunctions::FiberSetGestureState);
  context->RegisterObjectCFunction(gesture_manager, kCFuncConsumeGesture,
                                   &RendererFunctions::FiberConsumeGesture);
}

}  // namespace tasm
}  // namespace lynx
