// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_H_

#include <string>

#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {

namespace lepus {
class Context;
class Value;
struct RenderBindingFunction;
}  // namespace lepus

namespace tasm {

// Pointer of TempateAssembler
constexpr char kTemplateAssembler[] = "$kTemplateAssembler";

constexpr static const char* kCFuncGetLength = "_GetLength";
constexpr static const char* kCFuncIndexOf = "_IndexOf";
constexpr static const char* kCFuncSetValueToMap = "_SetValueToMap";

constexpr static const char* kCFuncAttachPage = "_AttachPage";
constexpr static const char* kCFuncAppendChild = "_AppendChild";
constexpr static const char* kCFuncAppendSubTree = "_AppendSubTree";
constexpr static const char* kCFuncCloneSubTree = "_CloneSubTree";
constexpr static const char* kCFuncCreateVirtualNode = "_CreateVirtualNode";
constexpr static const char* kCFuncCreateVirtualComponent =
    "_CreateVirtualComponent";
constexpr static const char* kCFuncCreatePage = "_CreatePage";
constexpr static const char* kCFuncSetAttributeTo = "_SetAttributeTo";
constexpr static const char* kCFuncSetStaticAttributeTo =
    "_SetStaticAttributeTo";
constexpr static const char* kCFuncSetStyleTo = "_SetStyleTo";
constexpr static const char* kCFuncSetStaticStyleTo = "_SetStaticStyleTo";
constexpr static const char* kCFuncSetDynamicStyleTo = "_SetDynamicStyleTo";
constexpr static const char* kCFuncSetClassTo = "_SetClassTo";
constexpr static const char* kCFuncSetStaticClassTo = "_SetStaticClassTo";
constexpr static const char* kCFuncSetStaticEventTo = "_SetStaticEventTo";
constexpr static const char* kCFuncSetDataSetTo = "_SetDataSetTo";
constexpr static const char* kCFuncSetEventTo = "_SetEventTo";
constexpr static const char* kCFuncSetId = "_SetId";
constexpr static const char* kCFuncSetStaticStyleTo2 = "_SetStaticStyleTo2";
constexpr static const char* kCFuncSetStaticStyleToByFiber =
    "_SetStaticStyleToByFiber";
constexpr static const char* kCFuncGetGlobalProps = "_GetGlobalProps";
constexpr static const char* kCFuncHandleExceptionInLepus =
    "_HandleExceptionInLepus";
constexpr static const char* kCFuncMarkPageElement = "_MarkPageElement";
constexpr static const char* kCFuncGetSystemInfo = "_GetSystemInfo";

// Set Element Worklet
constexpr static const char* kCFuncSetScriptEventTo = "_SetScriptEventTo";

// Set Gesture Worklet
constexpr static const char* kCFuncCreateGestureDetector =
    "_CreateGestureDetector";
// Set Gesture Worklet in Fiber
constexpr static const char* kCFuncSetGestureDetector = "__SetGestureDetector";
constexpr static const char* kCFuncRemoveGestureDetector =
    "__RemoveGestureDetector";
constexpr static const char* kCFuncSetGestureState = "__SetGestureState";
constexpr static const char* kCFuncConsumeGesture = "__ConsumeGesture";

constexpr static const char* kCFuncCreateVirtualSlot = "_CreateVirtualSlot";
constexpr static const char* kCFuncCreateVirtualPlug = "_CreateVirtualPlug";
constexpr static const char* kCFuncCreateVirtualPlugWithComponent =
    "_CreateVirtualPlugWithComponent";
constexpr static const char* kCFuncMarkComponentHasRenderer =
    "_MarkComponentHasRenderer";

// Relevant to component
constexpr static const char* kCFuncSetProp = "_SetProp";
constexpr static const char* kCFuncSetData = "_SetData";
constexpr static const char* kCFuncProcessComponentData = "_ProcessData";
constexpr static const char* kCFuncAddPlugToComponent = "_AddPlugToComponent";
constexpr static const char* kCFuncAppendVirtualPlugToComponent =
    "_AppendPlugToComponent";
constexpr static const char* kCFuncGetComponentData = "_GetComponentData";
constexpr static const char* kCFuncGetComponentProps = "_GetComponentProps";
constexpr static const char* kCFuncUpdateComponentInfo = "_UpdateComponentInfo";
constexpr static const char* kCFuncGetComponentInfo = "_GetComponentInfo";
constexpr static const char* kCFuncCreateVirtualListNode =
    "_CreateVirtualListNode";
constexpr static const char* kCFuncAppendListComponentInfo =
    "_AppendListComponentInfo";
constexpr static const char* kCFuncSetListRefreshComponentInfo =
    "_SetListRefreshComponentInfo";
constexpr static const char* kCFuncCreateVirtualComponentByName =
    "_CreateVirtualComponentByName";
constexpr static const char* kCFuncCreateDynamicVirtualComponent =
    "_CreateDynamicVirtualComponent";
constexpr static const char* kCFuncRenderDynamicComponent =
    "_RenderDynamicComponent";
constexpr static const char* kCFuncAddFallbackToDynamicComponent =
    "_AddFallbackToDynamicComponent";
constexpr static const char* kCFuncSetComponent = "_SetComponent";

constexpr static const char* kCFuncGetLazyLoadCount = "_GetLazyLoadCount";
constexpr static const char* kCFuncThemedTranslation = "_sysTheme";
constexpr static const char* kCFuncThemedLangTranslation = "_sysLang";

// Template API
constexpr static const char* kCFuncRegisterDataProcessor =
    "registerDataProcessor";
constexpr static const char* kCFuncAddEventListener = "_AddEventListener";
constexpr static const char* kCFuncSendGlobalEvent = "_SendGlobalEvent";

// Element Worklet
constexpr static const char* kCFuncRegisterElementWorklet =
    "registerElementWorklet";

// i18n
constexpr static const char* kCFuncFilterI18nResource = "_FilterI18nResource";
constexpr static const char* kCFuncI18nResourceTranslation =
    "_I18nResourceTranslation";
constexpr static const char* kCFuncReFlushPage = "_ReFlushPage";

// Radon Component
constexpr static const char* kCFuncGetComponentContextData = "_context";
constexpr static const char* kCFuncSetContextData = "_SetContextData";

// Element API
// Create Element
constexpr static const char* kCFunctionCreateElement = "__CreateElement";
constexpr static const char* kCFunctionCreateView = "__CreateView";
constexpr static const char* kCFunctionCreateText = "__CreateText";
constexpr static const char* kCFunctionCreateImage = "__CreateImage";
constexpr static const char* kCFunctionCreateRawText = "__CreateRawText";
constexpr static const char* kCFunctionCreateNonElement = "__CreateNonElement";
constexpr static const char* kCFunctionCreateWrapperElement =
    "__CreateWrapperElement";
constexpr static const char* kCFunctionCreateList = "__CreateList";
constexpr static const char* kCFunctionCreateScrollView = "__CreateScrollView";
constexpr static const char* kCFunctionCreatePage = "__CreatePage";
constexpr static const char* kCFunctionGetPageElement = "__GetPageElement";
constexpr static const char* kCFunctionCreateComponent = "__CreateComponent";
constexpr static const char* kCFunctionQuerySelector = "__QuerySelector";
constexpr static const char* kCFunctionQuerySelectorAll = "__QuerySelectorAll";
constexpr static const char* kCFunctionCreateIf = "__CreateIf";
constexpr static const char* kCFunctionCreateFor = "__CreateFor";
constexpr static const char* kCFunctionCreateBlock = "__CreateBlock";
constexpr static const char* kCFunctionCreateFrame = "__CreateFrame";
// Element Tree
constexpr static const char* kCFunctionAppendElement = "__AppendElement";
constexpr static const char* kCFunctionRemoveElement = "__RemoveElement";
constexpr static const char* kCFunctionInsertElementBefore =
    "__InsertElementBefore";
constexpr static const char* kCFunctionFirstElement = "__FirstElement";
constexpr static const char* kCFunctionLastElement = "__LastElement";
constexpr static const char* kCFunctionNextElement = "__NextElement";
constexpr static const char* kCFunctionReplaceElement = "__ReplaceElement";
constexpr static const char* kCFunctionReplaceElements = "__ReplaceElements";
constexpr static const char* kCFunctionSwapElement = "__SwapElement";
constexpr static const char* kCFunctionGetParent = "__GetParent";
constexpr static const char* kCFunctionGetChildren = "__GetChildren";
constexpr static const char* kCFunctionCloneElement = "__CloneElement";
constexpr static const char* kCFunctionElementIsEqual = "__ElementIsEqual";
constexpr static const char* kCFunctionUpdateIfNodeIndex =
    "__UpdateIfNodeIndex";
constexpr static const char* kCFunctionUpdateForChildCount =
    "__UpdateForChildCount";
constexpr static const char* kCFunctionMarkTemplateElement =
    "__MarkTemplateElement";
constexpr static const char* kCFunctionIsTemplateElement =
    "__IsTemplateElement";
constexpr static const char* kCFunctionMarkPartElement = "__MarkPartElement";
constexpr static const char* kCFunctionIsPartElement = "__IsPartElement";
constexpr static const char* kCFunctionGetTemplateParts = "__GetTemplateParts";
constexpr static const char* kCFunctionCreateElementWithProperties =
    "__CreateElementWithProperties";

#pragma region simple styling api
constexpr const char* kCFunctionCreateStyleObject = "__CreateStyleObject";
constexpr const char* kCFunctionSetStyleObject = "__SetStyleObject";
constexpr const char* kCFunctionUpdateStyleObject = "__UpdateStyleObject";
#pragma endregion  // simple styling api

// Singal API
// TODO(songshourui.null): Based on the discussion results of the Element API,
// it will be decided whether to clearly distinguish between the Element API and
// the Signal API in the naming of the Signal API.
constexpr static const char* kCFunctionCreateSignal = "__CreateSignal";
constexpr static const char* kCFunctionWriteSignal = "__WriteSignal";
constexpr static const char* kCFunctionReadSignal = "__ReadSignal";
constexpr static const char* kCFunctionCreateComputation =
    "__CreateComputation";
constexpr static const char* kCFunctionCreateMemo = "__CreateMemo";
constexpr static const char* kCFunctionUnTrack = "__UnTrack";
constexpr static const char* kCFunctionCreateScope = "__CreateScope";
constexpr static const char* kCFunctionGetScope = "__GetScope";
constexpr static const char* kCFunctionCleanUp = "__CleanUp";
constexpr static const char* kCFunctionOnCleanUp = "__OnCleanUp";
constexpr static const char* kCFunctionRunUpdates = "__RunUpdates";

// Element Info
constexpr static const char* kCFunctionGetElementUniqueID =
    "__GetElementUniqueID";
constexpr static const char* kCFunctionGetElementByUniqueID =
    "__GetElementByUniqueID";
constexpr static const char* kCFunctionGetTag = "__GetTag";
constexpr static const char* kCFunctionSetAttribute = "__SetAttribute";
constexpr static const char* kCFunctionGetAttributes = "__GetAttributes";
constexpr static const char* kCFunctionGetAttributeByName =
    "__GetAttributeByName";
constexpr static const char* kCFunctionGetAttributeNames =
    "__GetAttributeNames";
constexpr static const char* kCFunctionAddClass = "__AddClass";
constexpr static const char* kCFunctionSetClasses = "__SetClasses";
constexpr static const char* kCFunctionGetClasses = "__GetClasses";
constexpr static const char* kCFunctionAddInlineStyle = "__AddInlineStyle";
constexpr static const char* kCFunctionSetInlineStyles = "__SetInlineStyles";
constexpr static const char* kCFunctionGetInlineStyles = "__GetInlineStyles";
constexpr static const char* kCFunctionSetParsedStyles = "__SetParsedStyles";
constexpr static const char* kCFunctionGetInlineStyle = "__GetInlineStyle";
constexpr static const char* kCFunctionGetComputedStyles =
    "__GetComputedStyles";
constexpr static const char* kCFunctionAddEvent = "__AddEvent";
constexpr static const char* kCFunctionSetEvents = "__SetEvents";
constexpr static const char* kCFunctionGetEvent = "__GetEvent";
constexpr static const char* kCFunctionGetEvents = "__GetEvents";
constexpr static const char* kCFunctionSetID = "__SetID";
constexpr static const char* kCFunctionGetID = "__GetID";
constexpr static const char* kCFunctionAddDataset = "__AddDataset";
constexpr static const char* kCFunctionSetDataset = "__SetDataset";
constexpr static const char* kCFunctionGetDataset = "__GetDataset";
constexpr static const char* kCFunctionGetDataByKey = "__GetDataByKey";
constexpr static const char* kCFunctionSetCSSId = "__SetCSSId";
constexpr static const char* kCFunctionSetConfig = "__SetConfig";
constexpr static const char* kCFunctionAddConfig = "__AddConfig";
constexpr static const char* kCFunctionGetConfig = "__GetConfig";
constexpr static const char* kCFunctionAsyncResolveElement =
    "__AsyncResolveElement";
constexpr static const char* kCFunctionAddEventListener = "__AddEventListener";
constexpr static const char* kCFunctionFiberRemoveEventListener =
    "__RemoveEventListener";
constexpr static const char* kCFunctionCreateEvent = "__CreateEvent";
constexpr static const char* kCFunctionDispatchEvent = "__DispatchEvent";
constexpr static const char* kCFunctionStopPropagation = "__StopPropagation";
constexpr static const char* kCFunctionStopImmediatePropagation =
    "__StopImmediatePropagation";
// Element Component Info
constexpr static const char* kCFunctionGetComponentID = "__GetComponentID";
constexpr static const char* kCFunctionUpdateComponentID =
    "__UpdateComponentID";
constexpr static const char* kCFunctionUpdateComponentInfo =
    "__UpdateComponentInfo";
// List Info
constexpr static const char* kCFunctionUpdateListCallbacks =
    "__UpdateListCallbacks";
// Other RenderFunctions
constexpr static const char* kCFunctionFlushElementTree = "__FlushElementTree";
constexpr static const char* kCFunctionAsyncResolveSubtreeProperty =
    "__AsyncResolveSubtree";
constexpr static const char* kCFunctionMarkAsyncFlushRoot =
    "__MarkAsyncFlushRoot";
constexpr static const char* kCFunctionOnLifecycleEvent = "__OnLifecycleEvent";
constexpr static const char* kCFunctionElementFromBinary =
    "__ElementFromBinary";
constexpr static const char* kCFunctionElementFromBinaryAsync =
    "__ElementFromBinaryAsync";
constexpr static const char* kCFuncLoadStyleSheet = "__LoadStyleSheet";
constexpr static const char* kCFuncAdoptStyleSheet = "__AdoptStyleSheet";
constexpr static const char* kCFuncReplaceStyleSheets = "__ReplaceStyleSheets";
constexpr static const char* kCFunctionQueryComponent = "__QueryComponent";
constexpr static const char* kCFunctionSetLepusInitData = "__SetLepusInitData";
constexpr static const char* kCFunctionGetDiffData = "__GetDiffData";
constexpr static const char* kCFunctionInvokeUIMethod = "__InvokeUIMethod";
constexpr static const char* kCFunctionGetComputedStyleByKey =
    "__GetComputedStyleByKey";

// air strict mode
// Element SetAttribute
constexpr static const char* kCFunctionTriggerLepusBridge =
    "_TriggerLepusBridge";
constexpr static const char* kCFunctionTriggerLepusBridgeSync =
    "_TriggerLepusBridgeSync";
constexpr static const char* kCFunctionTriggerComponentEvent =
    "_TriggerComponentEvent";

// lepusNg sourceMap
constexpr static const char* kCFunctionSetSourceMapRelease =
    "_SetSourceMapRelease";
constexpr static const char* kCFunctionReportError = "_ReportError";

// Worklet
constexpr static const char* kCFunctionLoadLepusChunk = "__LoadLepusChunk";
constexpr static const char* kCFunctionElementAnimate = "__ElementAnimate";

class Utils {
 public:
  static lepus::Value CreateLynx(lepus::Context* context,
                                 const std::string& version);
  static lepus::Value CreateLynxPerformance(lepus::Context* context);
  static lepus::Value CreateResponseHandler(lepus::Context* context,
                                            const lepus::Value& handler_impl);
  static lepus::Value CreateContextProxy(lepus::Context* context,
                                         runtime::ContextProxy::Type type,
                                         const lepus::Value& proxy_impl);
  static lepus::Value CreateGestureManager(lepus::Context* context);
  static lepus::Value CreateLepusModule(lepus::Context* context,
                                        const lepus::Value& module_impl);
};

class Renderer {
 public:
  static void RegisterBuiltin(lepus::Context* context, ArchOption option);

 private:
  static lepus::Value SlotFunction(lepus::Context* context, lepus::Value* args,
                                   int size);
  static const lepus::RenderBindingFunction* GetBuiltinFunctionsForRadon(
      int32_t& size);
  static const lepus::RenderBindingFunction* GetBuiltinFunctionsForFiber(
      int32_t& size);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_H_
