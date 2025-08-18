// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TRACE_RENDERER_TRACE_EVENT_DEF_H_
#define CORE_RENDERER_TRACE_RENDERER_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

/**
 * @trace_description: Update component Data. Component name is
 * @args{componentName}. Updated Keys is @args{Keys}. `Keys` represent the state
 * keys updated in this update.
 */
inline constexpr const char* const RADON_DIFF_UPDATE_COMPONENT_DARA =
    "UpdateComponentData";
/**
 * @trace_description: RootComponent update triggered by background scripting
 * thread(historically known as "JS Thread").
 */
inline constexpr const char* const LYNX_UPDATE_DATA_BY_JS =
    "LynxUpdateDataByJS";
/**
 * @trace_description: Root Component update. Updated Keys is @args{Keys}. Keys
 * represent the keys of the state that were updated in this update. defaultData
 * represents the current state keys of the root component.
 */
inline constexpr const char* const LYNX_UPDATE_DATA = "LynxUpdateData";
/**
 * @trace_description: Component update triggered by background scripting thread
 * (historically known as "JS Thread"). A component update can be triggered for
 * multiple reasons.
 */
inline constexpr const char* const LYNX_UPDATE_COMPONENT_DATA_BY_JS =
    "LynxUpdateComponentDataByJS";
/**
 * @trace_description: Update the attributes of the RadonNode and recursively
 * execute DispatchChildrenForDiff process of its child.
 */
inline constexpr const char* const RADON_DIFF_DISPATCH_CHILDREN =
    "DispatchChildrenForDiff";
/**
 * @trace_description: Update the data of the @args{componentName} component.
 */
inline constexpr const char* const RADON_COMPONENT_UPDATE_COMPONENT =
    "RadonComponent::UpdateRadonComponent";
/**
 * @trace_description: Create child nodes and update component attributes, then
 * execute DispatchForDiff process for child nodes. Finally, send the
 * "__OnReactComponentDidUpdate" to trigger the component's componentDidUpdate
 * lifecycle method.
 */
inline constexpr const char* const RADON_COMPONENT_DISPATCH_FOR_DIFF =
    "RadonComponent::DispatchForDiff";
/**
 * @trace_description: Diff process of the virtual node tree in Lynx using the
 * MyersDiff algorithm.
 */
inline constexpr const char* const RADON_MYERS_DIFF =
    "RadonBase::RadonMyersDiff";
/**
 * @trace_description: Create element and update attributes of the node if
 * needed.
 */
inline constexpr const char* const RADON_DISPATCH_SELF = "RadonDispatchSelf";
/**
 * @trace_description: No changes happened in the update pipeline; it is
 * considered a useless update.
 */
inline constexpr const char* const ELEMENT_MANAGER_ON_PATCH_FINISH_NO_PATCH =
    "ElementManager::OnPatchFinishNoPatch";
/**
 * @trace_description: Execute tasks such as the layout of elements and the
 * creation of platform UI views after the virtual nodes diff process is
 * completed.
 */
inline constexpr const char* const ELEMENT_MANAGER_ON_PATCH_FINISH =
    "ElementManager::OnPatchFinish";

/**
 * @trace_description: Layout stage. This stage is based on layout node tree to
 * complete the layout stage, and finally synchronize the layout results to
 * element. Element adjusts the layout results and generates UI layout op.
 */
inline constexpr const char* const LAYOUT_CONTEXT_LAYOUT =
    "LayoutContext.Layout";
/**
 * @trace_description: Request to trigger the Layout stage when layout related
 * computed styles, such as width, height, are updated.
 */
inline constexpr const char* const LAYOUT_CONTEXT_REQUEST_LAYOUT =
    "LayoutContext.RequestLayout";

/**
 * @trace_description: Load the Lynx bundle, which includes four parts: load,
 * parse, framework rendering, and pixel pipeline.
 * 1. Load: request the bundle;
 * 2. Parse: parse bundle for subsequent pipeline process;
 * 3. Framework rendering: create and synchronize its internal representation of
 * the UI with the actual element tree in the engine through element
 * manipulation.
 * @link{https://lynxjs.org/guide/spec.html#frameworkrendering-framework-rendering};
 * 4. Pixel pipeline: processes element tree into pixels that are displayed on
 * the users' screen.
 * @link{https://lynxjs.org/guide/spec.html#enginepixeling-pixel-pipeline};
 */
inline constexpr const char* const LYNX_LOAD_TEMPLATE = "LynxLoadTemplate";

/**
 * @trace_description: Parse diff result and list child component info in
 * ReactLynx2 or TTMLRadonDiff for c++ list.
 */
inline constexpr const char* const LIST_ADAPTER_UPDATE_DATA_SOURCE =
    "ListAdapter::UpdateDataSource";

/**
 * @trace_description:  Parse diff result and list child component info in
 * ReactLynx3 or TTMLNoDiff for c++ list.
 */
inline constexpr const char* const LIST_ADAPTER_UPDATE_FIVER_DATA_SOURCE =
    "ListAdapter::UpdateFiberDataSource";
/**
 * @trace_description: Pre-render off-screen list child components for c++ list.
 * @history_name{Preload}
 */
inline constexpr const char* const LIST_PRELOAD =
    "LinearLayoutManager::Preload";

/**
 * @trace_description: Prerender of off-screen upper elements in the <list>
 * component.
 */
inline constexpr const char* const LIST_PRE_RENDER_OFF_SCREEN_UPPER_ELEMENT =
    "LinearLayoutManager::PreloadToStart";

/**
 * @trace_description: Update content size and offset and flush content size and
 * scroll info to platform ListContainerView.
 */
inline constexpr const char* const FLUSH_CONTENT_SIZE_AND_OFFSET_TO_PLATFORM =
    "FlushContentSizeAndOffsetToPlatform";

/**
 * @trace_description: Render one list child component in ReactLynx2 or
 * TTMLRadonDiff.
 * @history_name{List::RenderComponent}
 */
inline constexpr const char* const RADON_LIST_RENDER_COMPONENT =
    "RadonDiffListNode2::ComponentAtIndex";

/**
 * @trace_description: Render one list child component in ReactLynx3 or
 * TTMLNoDiff.
 */
inline constexpr const char* const LIST_ELEMENT_RENDER_COMPONENT =
    "ListElement::ComponentAtIndex";

/**
 * @trace_description: Send a request of LazyBundle on Main Thread Script(MTS).
 * It is a necessary step to render a LazyBundle. However, it only occurs when
 * rendering LazyBundle on IFR in ReactLynx3.
 * @link{https://lynxjs.org/guide/interaction/ifr.html}
 */
inline constexpr const char* const LAZY_BUNDLE_REQUIRE_TEMPLATE =
    "LazyBundle::RequireTemplateEntry";

inline constexpr const char* const LYNX_ENV_CONFIG_UPDATE_SCREEN_SIZE =
    "UpdateScreenSize";
inline constexpr const char* const PAGE_PROXY_UPDATE_IN_LOAD_TEMPLATE =
    "UpdateInLoadTemplate";
inline constexpr const char* const PAGE_PROXY_FORCE_UPDATE = "ForceUpdate";
inline constexpr const char* const PAGE_PROXY_ON_COMPONENT_ADDED =
    "OnComponentAdded";
inline constexpr const char* const PAGE_PROXY_ON_COMPONENT_REMOVED =
    "OnComponentRemoved";
inline constexpr const char* const PAGE_PROXY_ON_COMPONENT_MOVED =
    "OnComponentMoved";
inline constexpr const char* const PAGE_PROXY_ON_REACT_COMPONENT_CREATED =
    "OnReactComponentCreated";
inline constexpr const char* const PAGE_PROXY_ON_REACT_COMPONENT_RENDER =
    "OnReactComponentRender";
inline constexpr const char* const PAGE_PROXY_ON_REACT_COMPONENT_DID_UPDATE =
    "OnReactComponentDidUpdate";
inline constexpr const char* const PAGE_PROXY_ON_REACT_COMPONENT_DID_CATCH =
    "OnReactComponentDidCATCH";
inline constexpr const char* const PAGE_PROXY_ON_REACT_COMPONENT_UNMOUNT =
    "OnReactComponentUnmount";
inline constexpr const char* const PAGE_PROXY_ON_REACT_CARD_RENDER =
    "OnReactCardRender";
inline constexpr const char* const PAGE_PROXY_ON_REACT_CARD_DID_UPDATE =
    "OnReactCardDidUpdate";
inline constexpr const char* const PAGE_PROXY_GET_PATH_INFO =
    "PageProxy::GetPathInfo";
inline constexpr const char* const PAGE_PROXY_GET_FIELDS =
    "PageProxy::GetFields";
inline constexpr const char* const PAGE_PROXY_UPDATE_DATA_FOR_SSR =
    "UpdateDataForSsr";
inline constexpr const char* const PAGE_PROXY_SSR_CREATE_DOM = "SSR::CreateDom";
inline constexpr const char* const PAGE_PROXY_SSR_DISPATCH = "SSR::Dispatch";
inline constexpr const char* const PAGE_PROXY_SSR_PROCESS_SCRIPT =
    "SSR::ProcessScript";

inline constexpr const char* const
    TEMPLATE_ASSEMBLER_EXECUTE_ON_LAYOUT_READY_HOOKS =
        "TemplateAssembler::ExecuteOnLayoutReadyHooks";
inline constexpr const char* const
    TEMPLATE_ASSEMBLER_ASYNC_EXECUTE_ON_LAYOUT_READY_HOOKS =
        "TemplateAssembler::AsyncExecuteOnLayoutReadyHooks";
inline constexpr const char* const
    TEMPLATE_ASSEMBLER_ENSURE_ON_LAYOUT_READY_HOOKS_FINISH =
        "TemplateAssembler::EnsureOnLayoutReadyHooksFinish";
inline constexpr const char* const TEMPLATE_ASSEMBLER_ENSURE_ON_LAYOUT_AFTER =
    "TemplateAssembler::OnLayoutAfter";

inline constexpr const char* const TEMPLATE_ASSEMBLER_SCOPE_CONSTRUCTOR =
    "TemplateAssembler::Scope::Scope";
inline constexpr const char* const TEMPLATE_ASSEMBLER_CONSTRUCTOR =
    "TemplateAssembler::TemplateAssembler";
inline constexpr const char* const TEMPLATE_ASSEMBLER_ON_JS_PREPARED =
    "TemplateAssembler::OnJSPrepared";
inline constexpr const char* const TEMPLATE_ASSEMBLER_SET_NATIVE_PROPS =
    "TemplateAssembler::SetNativeProps";
inline constexpr const char* const SEND_LAZY_BUNDLE_GLOBAL_EVENT =
    "TemplateAssembler::SendLazyBundleGlobalEvent";
inline constexpr const char* const TEMPLATE_ASSEMBLER_CALL_LEPUS_METHOD =
    "TemplateAssembler::CallLepusMethod";
inline constexpr const char* const TEMPLATE_ASSEMBLER_FETCH_BUNDLE =
    "TemplateAssembler::FetchBundle";
inline constexpr const char* const TEMPLATE_ASSEMBLER_UPDATE_DATA =
    "UpdateData";
inline constexpr const char* const LYNX_UPDATE_GLOBAL_PROPS =
    "LynxUpdateGlobalProps";
inline constexpr const char* const TEMPLATE_ENTRY_SET_INIT_DATA =
    "TemplateEntry::SetInitData";
inline constexpr const char* const PATCH_FINISH_INNER_FOR_AIR =
    "OnPatchFinishInnerForAir";
inline constexpr const char* const LYNX_DECODE = "LynxDecode";
inline constexpr const char* const VM_EXECUTE = "VMExecute";
inline constexpr const char* const LYNX_DOM_READY = "LynxDomReady";
inline constexpr const char* const LYNX_RELOAD_TEMPLATE = "LynxReloadTemplate";
inline constexpr const char* const LYNX_RELOAD_TEMPLATE_WITH_GLOBAL_PROPS =
    "LynxReloadTemplateWithGlobalProps";
inline constexpr const char* const RELOAD_FROM_JS = "ReloadFromJS";
inline constexpr const char* const LAZY_BUNDLE_LOAD = "LazyBundle.Load";
inline constexpr const char* const LAZY_BUNDLE_BUILD_TEMPLATE_ENTRY =
    "LazyBundle::BuildTemplateEntry";
inline constexpr const char* const LAZY_BUNDLE_PRELOAD = "LazyBundle::Preload";
inline constexpr const char* const CONVERT_VALUE_WITH_READ_ONLY =
    "ConvertValueWithReadOnly";
inline constexpr const char* const UPDATE_META_DATA = "UpdateMetaData";
inline constexpr const char* const FROM_BINARY = "FromBinary";
inline constexpr const char* const DATA_PROCESSOR = "dataProcessor";

/** PIPELINE_START  */
inline constexpr const char* const LYNX_PIPELINE_RUN_PIXEL = "RunPixelPipeline";
inline constexpr const char* const LYNX_PIPELINE_ON_LAYOUT_AFTER =
    "OnLayoutAfter";
inline constexpr const char* const LYNX_PIPELINE_TRIGGER_RESOLVE =
    "TriggerResolve";
inline constexpr const char* const LYNX_PIPELINE_TRIGGER_LAYOUT =
    "TriggerLayout";
inline constexpr const char* const LYNX_PIPELINE_FLUSH_UI_OPERATION =
    "FlushUiOperation";

/** PIPELINE_END */

inline constexpr const char* const TEMPLATE_ENTRY_HOLDER_FIND_ENTRY =
    "TemplateEntryHolder::FindEntry";
inline constexpr const char* const TEMPLATE_ENTRY_HOLDER_FIND_TEMPLATE_ENTRY =
    "TemplateEntryHolder::FindTemplateEntry";
inline constexpr const char* const TEMPLATE_ENTRY_HOLDER_FOR_EACH_ENTRY =
    "TemplateEntryHolder::ForEachEntry";
inline constexpr const char* const TEMPLATE_ENTRY_CONSTRUCTOR =
    "TemplateEntry::TemplateEntry";
inline constexpr const char* const TEMPLATE_ENTRY_INIT_WITH_PAGE_CONFIG =
    "TemplateEntry::InitWithPageConfigger";
inline constexpr const char* const TEMPLATE_ENTRY_INIT_CARD_ENV = "InitCardEnv";
inline constexpr const char* const TEMPLATE_ENTRY_REGISTER_BUILD_IN =
    "TemplateEntry::RegisterBuiltin";
inline constexpr const char* const TEMPLATE_ENTRY_SET_TEMPLATE_ASSEMBLER =
    "TemplateEntry::SetTemplateAssembler";
inline constexpr const char* const TEMPLATE_ENTRY_GET_ELEMENT_TEMPLATE_INFO =
    "TemplateEntry::GetElementTemplateInfo";
inline constexpr const char* const TEMPLATE_ENTRY_GET_PARSED_STYLES =
    "TemplateEntry::GetParsedStyles";
inline constexpr const char* const TEMPLATE_ENTRY_LOAD_LEPUS_CHUNK =
    "TemplateEntry::LoadLepusChunk";

inline constexpr const char* const CSS_PATCH_PROCESS_RAW =
    "CSSPatching::ProcessRaw";
inline constexpr const char* const STYLE_SHEET_MANAGER_GET_STYLE_SHEET =
    "CSSStyleSheetManager::GetCSSStyleSheet";
inline constexpr const char* const CSS_HANDLER_GET_VARIABLE_BY_RULE =
    "CSSVariableHandler::GetCSSVariableByRule";
inline constexpr const char* const DYNAMIC_STYLE_MANAGER_ADOPT_SHEET =
    "DynamicCSSStylesManager::AdoptStyle";
inline constexpr const char* const
    DYNAMIC_STYLE_MANAGER_UPDATE_RESOLVING_STATUS =
        "DynamicCSSStylesManager::UpdateWithResolvingStatus";
inline constexpr const char* const SHARED_FRAGMENT_INIT_PSEUDO_NOT_STYLE =
    "SharedCSSFragment::InitPseudoNotStyle";
inline constexpr const char* const UNIT_HANDLER_PROCESS =
    "UnitHandler::Process";
inline constexpr const char* const CSS_PATCH_RESOLVE_STYLE =
    "CSSPatching::ResolveStyle";
inline constexpr const char* const CSS_PATCH_APPLY_PSEUDO_NOT_STYLE =
    "CSSPatching::ApplyPseudoNotCSSStyle";
inline constexpr const char* const
    CSS_PATCH_APPLY_PSEUDO_CLASS_CHILD_SELECTOR_STYLE =
        "CSSPatching::ApplyPseudoClassChildSelectorStyle";
inline constexpr const char* const CSS_PATCH_GET_CSS_BY_RULE =
    "CSSPatching::GetCSSByRule";
inline constexpr const char* const CSS_PATCH_APPLY_CASCADE_STYLES =
    "CSSPatching::ApplyCascadeStyles";

inline constexpr const char* const ELEMENT_CONTAINER_FIND_PARENT =
    "ElementContainer::FindParentForChild";
inline constexpr const char* const ELEMENT_CONTAINER_TRANSITION =
    "ElementContainer::TransitionToNativeView";
inline constexpr const char* const ELEMENT_CONTAINER_STYLE_CHANGED =
    "ElementContainer::StyleChanged";
inline constexpr const char* const ELEMENT_CONTAINER_Z_INDEX_CHANGED =
    "ElementContainer::ZIndexChanged";
inline constexpr const char* const ELEMENT_CONTAINER_UPDATE_Z_INDEX_LIST =
    "ElementContainer::UpdateZIndexList";

inline constexpr const char* const ELEMENT_MANAGER_CREATE_NODE =
    "ElementManager::CreateNode";
inline constexpr const char* const ELEMENT_MANAGER_REQUEST_LAYOUT =
    "ElementManager::RequestLayout";
inline constexpr const char* const ELEMENT_MANAGER_ON_PATCH_FINISH_INNER =
    "ElementManager::OnPatchFinishInner";
inline constexpr const char* const ELEMENT_MANAGER_ON_PATCH_FINISH_FOR_AIR =
    "ElementManager::OnPatchFinishInnerForAir";
inline constexpr const char* const ELEMENT_MANAGER_ON_PATCH_FINISH_FOR_FIBER =
    "ElementManager::OnPatchFinishInnerForFiber";
inline constexpr const char* const
    ELEMENT_MANAGER_ON_PATCH_FINISH_FIBER_NO_PATCH =
        "ElementManager::OnPatchFinishForFiberNoPatch";
inline constexpr const char* const ELEMENT_MANAGER_UPDATE_VIEWPORT =
    "ElementManager::UpdateViewport";
inline constexpr const char* const ELEMENT_MANAGER_TICK_ALL_ELEMENT =
    "ElementManager::TickAllElement";
inline constexpr const char* const ELEMENT_MANAGER_SORT_Z_INDEX =
    "ElementManager sort z-index";
inline constexpr const char* const ELEMENT_MANAGER_UPDATE_Z_INDEX_LIST =
    "ElementManager::UpdateZIndexList";
inline constexpr const char* const ELEMENT_VSYNC_PROXY_FRAME_TIME =
    "ElementVsyncProxy::VsyncFrameTime";
inline constexpr const char* const RESOLVE_ATTRIBUTES_AND_STYLE =
    "ResolveAttributesAndStyle";
inline constexpr const char* const ELEMENT_UPDATE_LAYOUT =
    "Element::UpdateLayout";
inline constexpr const char* const ELEMENT_SET_STYLE_INTERNAL =
    "Element::SetStyleInternal";
inline constexpr const char* const ELEMENT_CONSUME_ATTRIBUTE =
    "Element::WillConsumeAttribute";
inline constexpr const char* const ELEMENT_SET_KEYFRAMES_BY_NAMES =
    "Element::SetKeyframesByNames";
inline constexpr const char* const ELEMENT_PUSH_KEYFRAMES_TO_BUNDLE =
    "Element::PushKeyframesToBundle";
inline constexpr const char* const ELEMENT_RESOLVE_KEYFRAMES_BY_NAMES =
    "Element::ResolveCSSKeyframesByNames";
inline constexpr const char* const ELEMENT_TICK_ALL_ANIMATION =
    "Element::TickAllAnimation";
inline constexpr const char* const ELEMENT_UPDATE_FINAL_STYLE_MAP =
    "Element::UpdateFinalStyleMap";
inline constexpr const char* const ELEMENT_FLUSH_ANIMATED_STYLE =
    "Element::FlushAnimatedStyle";
inline constexpr const char* const ELEMENT_RESOLVE_AND_FLUSH_KEYFRAMES =
    "Element::ResolveAndFlushKeyframes";
inline constexpr const char* const DEVTOOL_FIBER_ATTACH_TO_INSPECTOR =
    "Devtool::FiberAttachToInspectorRecursively";
inline constexpr const char* const DEVTOOL_PREPARE_NODE_FOR_INSPECTOR =
    "Devtool::PrepareNodeForInspector";
inline constexpr const char* const DEVTOOL_CHECK_AND_PROCESS_FOR_INSPECTOR =
    "Devtool::CheckAndProcessSlotForInspector";
inline constexpr const char* const DEVTOOL_PREPARE_COMPONENT_FOR_INSPECTOR =
    "Devtool::PrepareComponentNodeForInspector";
inline constexpr const char* const DEVTOOL_PREPARE_COMPONENT_USELESS_UPDATE =
    "Devtool::OnComponentUselessUpdate";

inline constexpr const char* const AIR_TOUCH_EVENT_HANDLE_TOUCH_EVENT =
    "AirTouchEventHandler::HandleTouchEvent";
inline constexpr const char* const AIR_TOUCH_EVENT_FIRE_TOUCH_EVENT =
    "AirTouchEventHandler::FireTouchEvent";
inline constexpr const char* const AIR_TOUCH_EVENT_TRIGGER_COMPONENT_EVENT =
    "AirTouchEventHandler::TriggerComponentEvent";
inline constexpr const char* const AIR_TOUCH_EVENT_HANDLE_CUSTOM_EVENT =
    "AirTouchEventHandler::HandleCustomEvent";
inline constexpr const char* const AIR_TOUCH_EVENT_GENERATE_EVENT_OPERATION =
    "AirTouchEventHandler::GenerateEventOperation";
inline constexpr const char* const AIR_ELEMENT_CONTAINER_FIND_PARENT =
    "AirElementContainer::FindParentForChild";
inline constexpr const char* const AIR_ELEMENT_SET_ATTRIBUTE =
    "AirElement::SetAttribute";
inline constexpr const char* const AIR_ELEMENT_COMPUTE_CSS_STYLE =
    "AirElement::ComputeCSSStyle";
inline constexpr const char* const AIR_ELEMENT_UPDATE_LAYOUT =
    "AirElement::UpdateLayout";
inline constexpr const char* const AIR_ELEMENT_SET_STYLE =
    "AirElement::SetStyle";
inline constexpr const char* const AIR_ELEMENT_RESET_STYLE =
    "AirElement::ResetStyle";
inline constexpr const char* const AIR_ELEMENT_FLUSH_PROPS_RESOLVE_STYLES =
    "AirElement::FlushPropsResolveStyles";
inline constexpr const char* const AIR_ELEMENT_NO_PAINTING_NODE =
    "AirElement::FlushProps::NoPaintingNode";
inline constexpr const char* const AIR_ELEMENT_CALC_STYLE =
    "AirElement::CalcStyle";
inline constexpr const char* const AIR_ELEMENT_CALC_STYLE_ASYNC =
    "AirElement::CalcStyleAsync";
inline constexpr const char* const AIR_ELEMENT_SET_CLASSES =
    "AirElement::SetClasses";
inline constexpr const char* const AIR_ELEMENT_DIFF_STYLES =
    "AirElement::DiffStyles";
inline constexpr const char* const AIR_ELEMENT_REFRESH_STYLES =
    "AirElement::RefreshStyles";
inline constexpr const char* const AIR_ELEMENT_UPDATE_STYLE_PATCH =
    "AirElement::UpdateStylePatch";
inline constexpr const char* const AIR_ELEMENT_GET_STYLE_MAP =
    "AirElement::GetStyleMap";
inline constexpr const char* const AIR_ELEMENT_PUSH_TO_PROPS_BUNDLE =
    "AirElement::PushToPropsBundle";
inline constexpr const char* const AIR_ELEMENT_UPDATE_FIRST_SCREEN_LIST_STATE =
    "AirElement::UpdateFirstScreenListState";

inline constexpr const char* const LEPUS_DECODER_DECODE_MESSAGE =
    "LepusDecoder::DecodeMessage";
inline constexpr const char* const LEPUS_DECODER_ENCODE_MESSAGE =
    "LepusDecoder::EncodeMessage";
inline constexpr const char* const LEPUS_DECODER_WRITE_VALUE =
    "LepusEncoder::WriteValue";
inline constexpr const char* const LEPUS_DECODER_WRITE_STRING =
    "LepusEncoder::WriteString";
inline constexpr const char* const LEPUS_DECODER_WRITE_SIZE =
    "LepusEncoder::WriteSize";
inline constexpr const char* const LEPUS_DECODER_WRITE_JS_VALUE =
    "LepusEncoder::WriteJSValue";

/**
 * @trace_description: At this stage, based on the attribute of the element, the
 * computed style and prop bundle of the element are generated and synchronized
 * to the layout node. This stage will also create and modify layout node tree.
 * At the same time, it will also generate platform UI operations.
 */
inline constexpr const char* const FIBER_ELEMENT_FLUSH_ACTIONS =
    "FiberElement::FlushActions";
inline constexpr const char* const COMPONENT_ELEMENT_PREPARE_ROOT_CSS =
    "ComponentElement::PrepareForRootCSSVariables";
inline constexpr const char* const FIBER_ELEMENT_RESOLVE_PARENT_COMPONENT =
    "FiberElement::ResolveParentComponentElement";
inline constexpr const char* const FIBER_ELEMENT_ASYNC_RESOLVE_PROPERTY =
    "FiberElement::AsyncResolveProperty";
inline constexpr const char* const FIBER_ELEMENT_INSERT_NODE =
    "FiberElement::InsertNode";
inline constexpr const char* const FIBER_ELEMENT_SET_CLASS =
    "FiberElement::SetClass";
inline constexpr const char* const FIBER_ELEMENT_SET_CLASSES =
    "FiberElement::SetClasses";
inline constexpr const char* const FIBER_ELEMENT_SET_STYLE =
    "FiberElement::SetStyle";
inline constexpr const char* const FIBER_ELEMENT_SET_ATTRIBUTE =
    "FiberElement::SetAttribute";
inline constexpr const char* const FIBER_ELEMENT_SET_ID_SELECTOR =
    "FiberElement::SetAttribute";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_KEYFRAME_PROPS_CHANGE =
    "FiberElement::HandleKeyframePropsChange";
inline constexpr const char* const FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE =
    "FiberElement::PrepareForCreateOrUpdate";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_STYLE =
    "FiberElement::HandleStyle";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_PROPAGATE_INHERITED =
    "FiberElement::HandlePropagateInherited";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_DIRECTION_CHANGED =
    "FiberElement::HandleDirectionChanged";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_SET_STYLE =
    "FiberElement::HandleSetStyle";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_FONT_SIZE_CHANGE =
    "FiberElement::HandleFontSizeChange";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_TRANSITION_PROPS =
    "FiberElement::HandleTransitionProps";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_EVENTS =
    "FiberElement::HandleEvents";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_GESTURES =
    "FiberElement::HandleGestures";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_DATASET =
    "FiberElement::HandleDataset";
inline constexpr const char* const FIBER_ELEMENT_TRIGGER_ELEMENT_UPDATE =
    "FiberElement::TriggerElementUpdate";
inline constexpr const char* const FIBER_ELEMENT_FLUSH_ACTIONS_AS_ROOT =
    "FiberElement::FlushActionsAsRoot";
inline constexpr const char* const FIBER_ELEMENT_FLUSH_SELF =
    "FiberElement::FlushSelf";
inline constexpr const char* const FIBER_ELEMENT_PARALLEL_FLUSH_AS_ROOT =
    "FiberElement::ParallelFlushAsRoot";
inline constexpr const char* const FIBER_ELEMENT_CONSUME_PARALLEL_TASK =
    "FiberElement::ConsumeParallelTask";
inline constexpr const char* const FIBER_ELEMENT_CONSUME_LEFT_ITER =
    "FiberElement::ConsumeLeftIter";
inline constexpr const char* const FIBER_ELEMENT_WAIT_LEFT_ITER =
    "FiberElement::WaitLeftIter";
inline constexpr const char* const FIBER_ELEMENT_CONSUME_RIGHT_ITER =
    "FiberElement::ConsumeRightIter";
inline constexpr const char* const
    FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE_ASYNC =
        "FiberElement::PrepareForCreateOrUpdateAsync";
inline constexpr const char* const FIBER_ELEMENT_PARALLEL_FLUSH_RECURSIVELY =
    "FiberElement::ParallelFlushRecursively";
inline constexpr const char* const
    FIBER_ELEMENT_CHILD_PREPARE_FOR_CRATE_OR_UPDATE =
        "FiberElement::ChildrenPrepareForCreateOrUpdate";
inline constexpr const char* const FIBER_ELEMENT_PREPARE_CHILDREN =
    "FiberElement::PrepareChildren";
inline constexpr const char* const
    FIBER_ELEMENT_PREPARE_AND_GENERATE_CHILDREN_ACTIONS =
        "FiberElement::PrepareAndGenerateChildrenActions";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_CHILDREN_ACTION =
    "FiberElement::HandleChildrenAction";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_INSERT_CHILD_ACTION =
    "FiberElement::HandleInsertChildAction";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_REMOVE_CHILD_ACTION =
    "FiberElement::HandleRemoveChildAction";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_CONTAINER_INSERTION =
    "FiberElement::HandleContainerInsertion";
inline constexpr const char* const FIBER_ELEMENT_CONSUME_STYLE =
    "FiberElement::ConsumeStyle";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_ATTR =
    "FiberElement::HandleAttr";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_CRATE =
    "FiberElement::HandleCreate";
inline constexpr const char* const FIBER_ELEMENT_HANDLE_PARALLEL_REDUCE_TASKS =
    "FiberElement::HandleParallelReduceTasks";
inline constexpr const char* const FIBER_ELEMENT_ADD_DATA_SET =
    "FiberElement::AddDataset";
inline constexpr const char* const FIBER_ELEMENT_SET_DATA_SET =
    "FiberElement::SetDataset";
inline constexpr const char* const FIBER_ELEMENT_SET_JS_EVENT_HANDLER =
    "FiberElement::SetJSEventHandler";
inline constexpr const char* const FIBER_ELEMENT_SET_LEPUS_EVENT_HANDLER =
    "FiberElement::SetLepusEventHandler";
inline constexpr const char* const FIBER_ELEMENT_SET_WORKLET_EVENT_HANDLER =
    "FiberElement::SetWorkletEventHandler";
inline constexpr const char* const FIBER_ELEMENT_SET_NATIVE_PROPS =
    "FiberElement::SetNativeProps";
inline constexpr const char* const FIBER_ELEMENT_SET_PARSED_STYLES =
    "FiberElement::SetParsedStyles";
inline constexpr const char* const FIBER_ELEMENT_ADD_CONFIG =
    "FiberElement::AddConfig";
inline constexpr const char* const FIBER_ELEMENT_SET_CONFIG =
    "FiberElement::SetConfig";
inline constexpr const char* const FIBER_ELEMENT_FLUSH_PROPS =
    "FiberElement::FlushProps";
inline constexpr const char* const FIBER_ELEMENT_UPDATE_FIBER_ELEMENT =
    "FiberElement::UpdateFiberElement";
inline constexpr const char* const FIBER_ELEMENT_UPDATE_PAINTING_NODE =
    "FiberElement::UpdatePaintingNode";
inline constexpr const char* const FIBER_ELEMENT_TRANSITION_TO_NATIVE_VIEW =
    "FiberElement::TransitionToNativeView";
inline constexpr const char* const FIBER_ELEMENT_IS_RELATED_CSS_UPDATED =
    "FiberElement::IsRelatedCSSVariableUpdated";
inline constexpr const char* const FIBER_ELEMENT_UPDATE_CSS_VARIABLE =
    "FiberElement::UpdateCSSVariable";
inline constexpr const char* const FIBER_ELEMENT_PARSE_RAW_INLINE_STYLES =
    "FiberElement::ParseRawInlineStyles";
inline constexpr const char* const FIBER_ELEMENT_DO_FULL_STYLE_RESOLVE =
    "FiberElement::DoFullStyleResolve";
inline constexpr const char* const FIBER_ELEMENT_RESOLVE_CURRENT_STYLE =
    "FiberElement::ResolveCurrentStyleValue";
inline constexpr const char* const FIBER_ELEMENT_REFRESH_STYLE =
    "FiberElement::RefreshStyle";
inline constexpr const char* const FIBER_ELEMENT_PSEUDO_CHANGED =
    "FiberElement::OnPseudoStatusChanged";
inline constexpr const char* const FIBER_ELEMENT_FLUSH_ANIMATED_STYLE =
    "FiberElement::FlushAnimatedStyleInternal";
inline constexpr const char* const CATALYZER_NO_PAINTING_NODE =
    "Catalyzer::FlushProps::NoPaintingNode";
inline constexpr const char* const CATALYZER_HAS_PAINTING_NODE =
    "Catalyzer::FlushProps::HasPaintingNode";
inline constexpr const char* const TASM_TASK_RUNNER_WAIT_FOR_COMPLETION =
    "TasmTaskRunner::WaitForCompletion";

inline constexpr const char* const LIST_PARALLEL_FLUSH_AS_ROOT =
    "ListElement::ParallelFlushAsRoot";
inline constexpr const char* const LIST_ASYNC_RESOLVE_PROPERTY =
    "AsyncResolveListElementProperty";
inline constexpr const char* const LIST_ASYNC_RESOLVE_TREE =
    "AsyncResolveListElementTree";
inline constexpr const char* const LIST_COMPONENT_AT_INDEXES =
    "ListElement::ComponentAtIndexes";
inline constexpr const char* const LIST_ENQUEUE_COMPONENT =
    "ListElement::EnqueueComponent";
inline constexpr const char* const LIST_UPDATE_CALLBACKS =
    "ListElement::UpdateCallbacks";
inline constexpr const char* const LIST_NOTIFY_REUSE_NODE =
    "ListElement::NotifyListReuseNode";
inline constexpr const char* const LIST_ON_ELEMENT_UPDATED =
    "ListElement::OnListElementUpdated";
inline constexpr const char* const LIST_SCHEDULER_ADAPTER_RESOLVE_SUBTREE_PROP =
    "ListItemSchedulerAdapter::SubtreeNodePrepareAndPostResolveTask";
inline constexpr const char* const
    LIST_SCHEDULER_ADAPTER_SUBTREE_ASYNC_ENQUEUE =
        "ListItemSchedulerAdapter::SubtreeAsyncResolveEnqueue";
inline constexpr const char* const
    LIST_SCHEDULER_ADAPTER_CONSUME_ITEM_REDUCE_TASKS =
        "ListItemSchedulerAdapter::ConsumeListItemReduceTasks";
inline constexpr const char* const LIST_SCHEDULER_ADAPTER_CONSUME_REDUCE_TASKS =
    "ListItemSchedulerAdapter::ConsumeReduceTasks";
inline constexpr const char* const
    LIST_SCHEDULER_ADAPTER_RUN_AND_CONSUME_REDUCE_TASKS =
        "ListItemSchedulerAdapter::RunAndConsumeReduceTasks";
inline constexpr const char* const
    LIST_SCHEDULER_ADAPTER_WAIT_AND_REDUCE_TASKS =
        "ListItemSchedulerAdapter::WaitAndReduceTasks";
inline constexpr const char* const LIST_SCHEDULER_ADAPTER_POST_FLUSH_ACTIONS =
    "ListItemSchedulerAdapter::PostFlushListItemsActions";
inline constexpr const char* const LIST_SCHEDULER_ADAPTER_ASYNC_FLUSH =
    "ListItemSchedulerAdapter::AsyncFlushActionWithListItem";
inline constexpr const char* const
    LIST_SCHEDULER_ADAPTER_CONSUME_ELEMENT_REDUCE_TASKS =
        "ListItemSchedulerAdapter::ConsumeResolveElementTreeReduceTasks";

inline constexpr const char* const PAGE_ELEMENT_FLUSH_ACTIONS_AS_ROOT =
    "PageElement::FlushActionsAsRoot";

inline constexpr const char* const PAGE_ELEMENT_LAYOUT = "PageElement::Layout";

inline constexpr const char* const TREE_RESOLVER_CLONE_ELEMENTS =
    "TreeResolver::CloneElements";
inline constexpr const char* const TREE_RESOLVER_FROM_TEMPLATE_INFO =
    "TreeResolver::FromTemplateInfo";
inline constexpr const char* const TREE_RESOLVER_INIT_ELEMENT_TREE =
    "TreeResolver::InitElementTree";
inline constexpr const char* const TREE_RESOLVER_ATTACH_TO_ELEMENT_MANAGER =
    "TreeResolver::AttachRootToElementManager";
inline constexpr const char* const TREE_RESOLVER_FROM_ELEMENT_INFO =
    "TreeResolver::FromElementInfo";
inline constexpr const char* const FIBER_ELEMENT_SELECTOR_SELECT =
    "FiberElementSelector::Select";

inline constexpr const char* const RADON_NODE_SELECTOR_SELECT =
    "RadonNodeSelector::Select";
inline constexpr const char* const RADON_DISPATCH_CHILDREN = "DispatchChildren";
inline constexpr const char* const RADON_RESET_ELEMENT_RECURSIVELY =
    "RadonBase::ResetElementRecursively";
inline constexpr const char* const RADON_BASE_DISPATCH_CHILDREN =
    "RadonBase::RadonDiffChildren";
inline constexpr const char* const RADON_BASE_MODIFY_SUBTREE_COMPONENT =
    "RadonBase::NeedModifySubTreeComponent";
inline constexpr const char* const RADON_COMPONENT_PREPARE_ROOT_CSS =
    "RadonComponent::PrepareRootCSSVariables";
inline constexpr const char* const RADON_COMPONENT_UPDATE_PROP =
    "RadonComponent::UpdateProperty";
inline constexpr const char* const RADON_PREPROCESS_DATA = "PreprocessData";
inline constexpr const char* const RADON_PRERENDER_REACT = "PreRenderReact";
inline constexpr const char* const RADON_PRERENDER_TT = "PreRenderTT";
inline constexpr const char* const RADON_SHOULD_COMPONET_UPDATE =
    "ShouldComponentUpdate";
inline constexpr const char* const RADON_UPDATE_COMPONET_WITHOUT_DISPATCH =
    "RadonComponent::UpdateRadonComponentWithoutDispatch";
inline constexpr const char* const RADON_PRE_HANDLER_CSS =
    "RadonComponent::PreHandlerCSSVariable";
inline constexpr const char* const RADON_RENDER_COMPONENT_IF_NEEDED =
    "RadonComponent::RenderRadonComponentIfNeeded";
inline constexpr const char* const RADON_COMPONENT_RENDER_BASE =
    "RadonComponent::OnReactComponentRenderBase";
inline constexpr const char* const RADON_COMPONENT_DIFF_CHILDREN =
    "RadonComponent::RadonDiffChildren";
inline constexpr const char* const RADON_CREATE_ELEMENT_IF_NEEDED =
    "RadonNode::CreateElementIfNeeded";
inline constexpr const char* const RADON_DISPATCH_FIRST_TIME =
    "RadonNode::DispatchFirstTime";
inline constexpr const char* const RADON_SWAP_ELEMENT =
    "RadonNode::SwapElement";
inline constexpr const char* const RADON_SHOULD_FLUSH =
    "RadonNode::ShouldFlush";
inline constexpr const char* const RADON_SHOULD_FLUSH_ATTR =
    "RadonNode::ShouldFlushAttr";
inline constexpr const char* const RADON_SHOULD_FLUSH_STYLE =
    "RadonNode::ShouldFlushStyle";
inline constexpr const char* const RADON_SHOULD_FLUSH_DATA_SET =
    "RadonNode::ShouldFlushDataSet";
inline constexpr const char* const RADON_SHOULD_FLUSH_GESTURE_DETECTORS =
    "RadonNode::ShouldFlushGestureDetectors";
inline constexpr const char* const RADON_SHOULD_COLLECT_INVALID_SETS =
    "RadonNode::CollectInvalidationSetsAndInvalidate";
inline constexpr const char* const
    RADON_SHOULD_COLLECT_INVALID_SETS_FOR_PSEUDO =
        "RadonNode::CollectInvalidationSetsForPseudoAndInvalidate";
inline constexpr const char* const RADON_OPTIMIZE_SHOULD_FLUSH_STYLE =
    "RadonNode::OptimizedShouldFlushStyle";
inline constexpr const char* const RADON_HANDLE_FIXED_ELEMENT =
    "HandleFixedElement";
inline constexpr const char* const RADON_GET_PAGE_DATA_BY_KEY =
    "GetPageDataByKey";
inline constexpr const char* const RADON_REFRESH_WITH_GLOBAL_PROPS =
    "RefreshWithGlobalProps";
inline constexpr const char* const RADON_CHECK_TABLE_SHOULD_UPDATED =
    "RadonPage::UpdatePage::CheckTableShouldUpdated";
inline constexpr const char* const RADON_UPDATE_PAGE_DIFF =
    "RadonPage::UpdatePage::RadonDiff";
inline constexpr const char* const RADON_SLOT_DIFF_CHILDREN =
    "RadonSlot::RadonDiffChildren";
inline constexpr const char* const RADON_SLOT_MOVE_PLUGS_FROM_SLOTS =
    "RadonSlotsHelper::MovePlugsFromSlots";
inline constexpr const char* const RADON_SLOT_DIFF_WITH_PLUGS =
    "RadonSlotsHelper::DiffWithPlugs";
inline constexpr const char* const RADON_SLOT_REFILL_SLOTS_AFTER_DIFF =
    "RadonSlotsHelper::ReFillSlotsAfterChildrenDiff";
inline constexpr const char* const RADON_SLOT_FILL_UNATTACHED_PLUGS =
    "RadonSlotsHelper::FillUnattachedPlugs";
inline constexpr const char* const RADON_PLUG_SET_ATTACHED_COMPONENT =
    "RadonPlug::SetAttachedComponent";
inline constexpr const char* const DEVTOOL_CREATE_ELEMENT_IF_NEEDED =
    "Devtool::CreateElementIfNeeded";
inline constexpr const char* const DEVTOOL_NOTIFY_ELEMENT_NODE_ADDED =
    "Devtool::NotifyElementNodeAdded";
inline constexpr const char* const DEVTOOL_NOTIFY_ELEMENT_NODE_REMOVED =
    "Devtool::NotifyElementNodeRemoved";
inline constexpr const char* const DEVTOOL_NOTIFY_ELEMENT_NODE_SETTED =
    "Devtool::NotifyElementNodeSetted";
inline constexpr const char* const DEVTOOL_UPDATE_INLINE_STYLES_FROM_OLD_MODEL =
    "Devtool::UpdateInlineStylesFromOldModel";

inline constexpr const char* const RADON_LIST_SYNC_COMPONENT_EXTRA_INFO =
    "RadonDiffListNode::SyncComponentExtraInfo";
inline constexpr const char* const RADON_LIST_SHOULD_FLUSH =
    "RadonDiffListNode::ShouldFlush";
inline constexpr const char* const RADON_LIST_DIFF_CHILDREN =
    "RadonDiffListNode::RadonDiffChildren";
inline constexpr const char* const RADON_LIST_COMPONENT_AT_INDEX =
    "RadonDiffListNode2::ComponentAtIndex";
inline constexpr const char* const RADON_LIST_2_SYNC_COMPONENT_EXTRA_INFO =
    "RadonDiffListNode2::SyncComponentExtraInfo";
inline constexpr const char* const RADON_LIST_CREATE_COMPONENT_WITH_TYPE =
    "RadonListBase::CreateComponentWithType";
inline constexpr const char* const RADON_LIST_GET_COMPONENT =
    "List::GetComponent";
inline constexpr const char* const RADON_LIST_REMOVE_COMPONENT =
    "List::RemoveComponent";
inline constexpr const char* const RADON_LIST_UPDATE_COMPONENT =
    "List::UpdateComponent";
inline constexpr const char* const RADON_LIST_DIFF_COMPONENTS =
    "List::DiffListComponents";

inline constexpr const char* const RADON_ELEMENT_SET_NATIVE_PROPS =
    "RadonElement::SetNativeProps";
inline constexpr const char* const RADON_ELEMENT_UPDATE_DYNAMIC_STYLE =
    "Element.PreparePropsBundleForDynamicCSS";
inline constexpr const char* const RADON_ELEMENT_FLUSH_PROPS =
    "RadonElement::FlushProps";
inline constexpr const char* const RADON_ELEMENT_ON_PSEUDO_STATUS_CHANGED =
    "RadonElement::OnPseudoStatusChanged";
inline constexpr const char* const RADON_ELEMENT_CONSUME_STYLE =
    "RadonElement::ConsumeStyle";
inline constexpr const char* const RADON_ELEMENT_FLUSH_ANIMATED_STYLE =
    "RadonElement::FlushAnimatedStyleInternal";

inline constexpr const char* const LAZY_COMPONENT_DERIVE_FROM_MOULD =
    "LazyComponent::DeriveFromMould";
inline constexpr const char* const LAZY_COMPONENT_SET_CONTEXT =
    "LazyComponent::SetContext";
inline constexpr const char* const LAZY_COMPONENT_RENDER_ENTRANCE =
    "LazyComponent::RenderEntrance";
inline constexpr const char* const LAZY_COMPONENT_LOAD_LAZY_BUNDLE =
    "LazyComponent::LoadLazyBundle";

inline constexpr const char* const TOUCH_EVENT_HANDLE_TOUCH_EVENT =
    "TouchEventHandler::HandleTouchEvent";
inline constexpr const char* const TOUCH_EVENT_HANDLE_GESTURE_EVENT =
    "TouchEventHandler::HandleGestureEvent";
inline constexpr const char* const TOUCH_EVENT_HANDLE_PSEUDO_STATUS_CHANGED =
    "TouchEventHandler::HandlePseudoStatusChanged";
inline constexpr const char* const TOUCH_EVENT_HANDLE_CUSTOM_EVENT =
    "TouchEventHandler::HandleCustomEvent";
inline constexpr const char* const TOUCH_EVENT_FIRE_EVENT =
    "TouchEventHandler::FireEvent";
inline constexpr const char* const TOUCH_EVENT_FIRE_EVENT_FOR_AIR =
    "TouchEventHandler::FireEventForAir";
inline constexpr const char* const TOUCH_EVENT_HANDLE_BUBBLE_EVENT =
    "TouchEventHandler::HandleBubbleEvent";
inline constexpr const char* const TOUCH_EVENT_TRIGGER_COMPONENT_EVENT =
    "TouchEventHandler::HandleTriggerComponentEvent";
inline constexpr const char* const TOUCH_EVENT_TRIGGER_FIBER_ELEMENT_WORKLET =
    "TouchEventHandler::TriggerFiberElementWorklet";
inline constexpr const char* const TOUCH_EVENT_HANDLE_EVENT_INTERNAL =
    "HandleEventInternal";

inline constexpr const char* const EVENT_DISPATCHER_DISPATCH =
    "EventDispatcher::Dispatch";
inline constexpr const char* const TOUCH_EVENT_CUSTOM_DETAIL =
    "TouchEvent::HandleEventCustomDetail";
inline constexpr const char* const CUSTOM_EVENT_CUSTOM_DETAIL =
    "CustomEvent::HandleEventCustomDetail";
inline constexpr const char* const EVENT_TARGET_DISPATCHEVENT =
    "EventTarget::DispatchEvent";
inline constexpr const char* const CLOSURE_EVENT_LISTENER_INVOKE =
    "ClosureEventListener::Invoke";
inline constexpr const char* const LEPUS_CLOSURE_EVENT_LISTENER_INVOKE =
    "LepusClosureEventListener::Invoke";
inline constexpr const char* const JS_CLOSURE_EVENT_LISTENER_INVOKE =
    "JSClosureEventListener::Invoke";

inline constexpr const char* const I18N_BIND = "i18n::Bind";

inline constexpr const char* const ADAPTER_HELPER_UPDATE_DIFF_RESULT =
    "AdapterHelper::UpdateDiffResult";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_INSERTIONS =
    "AdapterHelper::UpdateInsertions";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_REMOVALS =
    "AdapterHelper::UpdateRemovals";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FROM =
    "AdapterHelper::UpdateUpdateFrom";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_TO =
    "AdapterHelper::UpdateUpdateTo";
inline constexpr const char* const ADAPTER_HELPER_MOVE_FROM =
    "AdapterHelper::UpdateMoveFrom";
inline constexpr const char* const ADAPTER_HELPER_MOVE_TO =
    "AdapterHelper::UpdateMoveTo";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_ITEM_KEYS =
    "AdapterHelper::UpdateItemKeys";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_ESTIMATED_HEIGHT =
    "AdapterHelper::UpdateEstimatedHeightsPx";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_ESTIMATED_SIZE =
    "AdapterHelper::UpdateEstimatedSizesPx";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FULL_SPANS =
    "AdapterHelper::UpdateFullSpans";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_STICKY_BOTTOMS =
    "AdapterHelper::UpdateStickyBottoms";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_STICKY_TOPS =
    "AdapterHelper::UpdateStickyTops";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FIBER_INSERT_ACTION =
    "AdapterHelper::UpdateFiberInsertAction";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FIBER_REMOVE_ACTION =
    "AdapterHelper::UpdateFiberRemoveAction";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FIBER_UPDATE_ACTION =
    "AdapterHelper::UpdateFiberUpdateAction";
inline constexpr const char* const ADAPTER_HELPER_UPDATE_FIBER_EXTRA_INFO =
    "AdapterHelper::UpdateFiberExtraInfo";
inline constexpr const char* const LIST_ADAPTER_BIND_ITEM_HOLDER =
    "ListAdapter::BindItemHolder";
inline constexpr const char* const LIST_ADAPTER_ENQUEUE_ELEMENTS =
    "ListAdapter::EnqueueElements";
inline constexpr const char* const LIST_ADAPTER_BIND_ITEM_HOLDERS =
    "BatchListAdapter::BindItemHolders";
inline constexpr const char* const LIST_ADAPTER_FINISH_BIND_ITEM_HOLDERS =
    "BatchListAdapter::OnFinishBindItemHolders";
inline constexpr const char* const LIST_ADAPTER_FINISH_BIND_INTERNAL =
    "BatchListAdapter::OnFinishBindInternal";
inline constexpr const char* const
    DEFAULT_LIST_ADAPTER_FINISH_BIND_ITEM_HOLDER =
        "DefaultListAdapter::OnFinishBindItemHolder";
inline constexpr const char* const
    DEFAULT_LIST_ADAPTER_FINISH_BIND_ITEM_HOLDER_FINISH =
        "DefaultListAdapter::OnFinishBindItemHolder.finish";
inline constexpr const char* const LIST_ADAPTER_RECYCLE_ITEM_HOLDER =
    "ListAdapter::RecycleItemHolder";
inline constexpr const char* const LIST_ADAPTER_RECYCLE_ALL_ITEM_HOLDER =
    "ListAdapter::RecycleAllItemHolders";
inline constexpr const char* const LIST_ADAPTER_RECYCLE_REMOVED_ITEM_HOLDER =
    "ListAdapter::RecycleRemovedItemHolders";
inline constexpr const char* const LIST_ADAPTER_OUTPUT_DATA_SOURCE_DIFF_INFO =
    "ListAdapter::UpdateDataSource.OutputDiffInfo";
inline constexpr const char* const
    LIST_ADAPTER_OUTPUT_FIBER_DATA_SOURCE_DIFF_INFO =
        "ListAdapter::UpdateFiberDataSource.OutputDiffInfo";
inline constexpr const char* const LIST_ADAPTER_UPDATE_ITEM_TO_LATEST =
    "ListAdapter::UpdateItemHolderToLatest";
inline constexpr const char* const LIST_ADAPTER_MARK_CHILD_DIRTY =
    "ListAdapter::MarkChildHolderDirty";
inline constexpr const char* const LIST_CHILDREN_ATTACH_CHILD =
    "ListChildrenHelper::AttachChild";
inline constexpr const char* const LIST_CHILDREN_DETACH_CHILD =
    "ListChildrenHelper::DetachChild";
inline constexpr const char* const LIST_CHILDREN_UPDATE_ON_SCREEN_CHILD =
    "ListChildrenHelper::UpdateOnScreenChildren";
inline constexpr const char* const
    LIST_CHILDREN_HANDLE_LAYOUT_OR_SCROLL_RESULT =
        "ListChildrenHelper::HandleLayoutOrScrollResult";
inline constexpr const char* const LIST_CONTAINER_ON_NEXT_FRAME =
    "ListContainerImpl::OnNextFrame";
inline constexpr const char* const LIST_CONTAINER_FLUSH_PATCHING =
    "ListContainerImpl::FlushPatching";
inline constexpr const char* const LIST_CONTAINER_RESOLVE_ATTRIBUTE =
    "ListContainerImpl::ResolveAttribute";

inline constexpr const char* const GRID_LAYOUT_MANAGER_LAYOUT_CHUNK =
    "GridLayoutManager::LayoutChunk";
inline constexpr const char* const GRID_LAYOUT_MANAGER_LAYOUT_INVALID_ITEM =
    "GridLayoutManager::LayoutInvalidItemHolder";
inline constexpr const char* const GRID_LAYOUT_MANAGER_BATCH_CHILDREN =
    "OnBatchLayoutChildren";
inline constexpr const char* const GRID_LAYOUT_MANAGER_LAYOUT_CHILDREN =
    "OnLayoutChildren";
inline constexpr const char* const GRID_LAYOUT_MANAGER_FILL = "Fill";
inline constexpr const char* const STAGGERED_GRID_LAYOUT_MANAGER_FILL_TO_END =
    "StaggeredGridLayoutManager::FillToEnd";
inline constexpr const char* const STAGGERED_GRID_LAYOUT_MANAGER_FILL_TO_START =
    "StaggeredGridLayoutManager::FillToStart";
inline constexpr const char* const
    STAGGERED_GRID_LAYOUT_MANAGER_BIND_ALL_VISIBLE_ITEM =
        "StaggeredGridLayoutManager::BindAllVisibleItemHolders";
inline constexpr const char* const
    STAGGERED_GRID_LAYOUT_MANAGER_LAYOUT_INVALID_ITEM =
        "StaggeredGridLayoutManager::LayoutInvalidItemHolder";
inline constexpr const char* const GRID_LAYOUT_MANAGER_SCROLL_BY_INTERNAL =
    "ScrollByInternal";
inline constexpr const char* const GRID_LAYOUT_MANAGER_HANDLE_LAYOUT_OR_SCROLL =
    "HandleLayoutOrScrollResult";
inline constexpr const char* const GRID_LAYOUT_MANAGER_BATCH_RENDER =
    "BatchRender";
inline constexpr const char* const
    GRID_LAYOUT_MANAGER_LAYOUT_CHILDREN_INTERNAL = "OnLayoutChildrenInternal";
inline constexpr const char* const GRID_LAYOUT_MANAGER_LAYOUT_AFTER =
    "OnLayoutAfter";
inline constexpr const char* const GRID_LAYOUT_MANAGER_BIND_ALL_VISIBLE_ITEM =
    "BindAllVisibleItemHolders";
inline constexpr const char* const
    GRID_LAYOUT_MANAGER_HANDLE_CONTENT_SIZE_AND_OFFSET =
        "UpdateContentSizeAndOffset";
inline constexpr const char* const GRID_LAYOUT_MANAGER_HANDLE_PRELOAD_IF_NEED =
    "HandlePreloadIfNeeded";
inline constexpr const char* const GRID_LAYOUT_MANAGER_SCROLL_BY_INTERNAL_FILL =
    "StaggeredGridLayoutManager::ScrollByInternal.Fill";
inline constexpr const char* const GRID_LAYOUT_MANAGER_ON_SCROLL_AFTER =
    "OnScrollAfter";

inline constexpr const char* const LINEAR_LAYOUT_MANAGER_BATCH_CHILDREN =
    "OnBatchLayoutChildren";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_LAYOUT_CHILDREN =
    "OnLayoutChildren";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_HANDLE_LAYOUT_OR_SCROLL =
        "HandleLayoutOrScrollResult";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_PRELOAD_SECTION =
    "LinearLayoutManager::PreloadSectionOnNextFrame";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_FILL_ANCHOR_EXTRA =
    "LinearLayoutManager::FillWithAnchor.FillExtra";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_FILL_ANCHOR_START =
    "LinearLayoutManager::FillWithAnchor.FillToStart";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_FILL_ANCHOR_END =
    "LinearLayoutManager::FillWithAnchor.FillToEnd";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_RECYCLE_PRELOAD_ITEM =
    "LinearLayoutManager::RecycleOffPreloadItemHolders";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_SCROLL_BY_INTERNAL =
    "ScrollByInternal";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_UPDATE_SCROLL_ANCHOR_INFO =
        "LinearLayoutManager::UpdateScrollAnchorInfo";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_LAYOUT_INVALID_ITEM =
    "LinearLayoutManager::LayoutInvalidItemHolder";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_LAYOUT_CHUNK =
    "LinearLayoutManager::LayoutChunk";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_PRELOAD_SECTION_TO_END =
        "LinearLayoutManager::PreloadSectionToEnd";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_PRELOAD_SECTION_TO_START =
        "LinearLayoutManager::PreloadSectionToStart";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_BATCH_RENDER =
    "BatchRender";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_LAYOUT_CHILDREN_INTERNAL = "OnLayoutChildrenInternal";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_LAYOUT_AFTER =
    "OnLayoutAfter";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_HANDLE_CONTENT_SIZE_AND_OFFSET =
        "UpdateContentSizeAndOffset";
inline constexpr const char* const
    LINEAR_LAYOUT_MANAGER_HANDLE_PRELOAD_IF_NEED = "HandlePreloadIfNeeded";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_HANDLE_PRELOAD_TO_END =
    "LinearLayoutManager::PreloadToEnd";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_UPDATE_STICKY_ITEMS =
    "UpdateStickyItems";
inline constexpr const char* const LINEAR_LAYOUT_MANAGER_ON_SCROLL_AFTER =
    "OnScrollAfter";

inline constexpr const char* const LIST_LAYOUT_MANAGER_RECYCLE_OFF_SCREEN_ITEM =
    "ListLayoutManager::RecycleOffScreenItemHolders";
inline constexpr const char* const
    LIST_LAYOUT_MANAGER_PREPARE_FOR_LAYOUT_CHILDREN =
        "ListLayoutManager::OnPrepareForLayoutChildren";
inline constexpr const char* const LIST_LAYOUT_MANAGER_LAYOUT_COMPONENT =
    "ListLayoutManager::OnLayoutCompleted";
inline constexpr const char* const
    LIST_LAYOUT_MANAGER_HANDLE_PLATFORM_OPERATION =
        "ListLayoutManager::HandlePlatformOperation";
inline constexpr const char* const LIST_LAYOUT_MANAGER_RETRIEVE_ANCHOR_INFO =
    "RetrieveAnchorInfoBeforeLayout";

inline constexpr const char* const RADON_LIST_ELEMENT_UPDATED =
    "RadonListElement::OnListElementUpdated";

inline constexpr const char* const LAYOUT_CONTEXT_DISPATCH_LAYOUT_UPDATES =
    "LayoutContext::DispatchLayoutUpdates";
inline constexpr const char* const LAYOUT_CONTEXT_DISPATCH_BEFORE_RECURSIVE =
    "DispatchLayoutBeforeRecursively";
inline constexpr const char* const LAYOUT_CONTEXT_CALCULATE_LAYOUT =
    "CalculateLayout";
inline constexpr const char* const LAYOUT_CONTEXT_LAYOUT_RECURSIVE =
    "LayoutRecursively";
inline constexpr const char* const LAYOUT_CONTEXT_ON_LAYOUT_AFTER =
    "OnLayoutAfter";
inline constexpr const char* const LAYOUT_CONTEXT_UPDATE_LAYOUT_NODE =
    "LayoutContext.UpdateLayoutNode";
inline constexpr const char* const LAYOUT_CONTEXT_UPDATE_LAYOUT_RECURSIVE =
    "UpdateLayoutRecursively";
inline constexpr const char* const LAYOUT_CONTEXT_CREATE_NODE =
    "LayoutContext.CreateLayoutNode";
inline constexpr const char* const LAYOUT_CONTEXT_UPDATE_MEASURE =
    "UpdateMeasure";
inline constexpr const char* const LAYOUT_CONTEXT_UPDATE_ALIGNMENT =
    "UpdateAlignment";
inline constexpr const char* const LAYOUT_CONTEXT_REMOVE_ALGORITHM_RECURSIVE =
    "RemoveAlgorithmRecursive";
inline constexpr const char* const LAYOUT_CONTEXT_ROUND_TO_PIXEL_GRID =
    "RoundToPixelGrid";
inline constexpr const char* const LAYOUT_CONTEXT_LAYOUT_RESULT =
    "LayoutContext.LayoutResult";
inline constexpr const char* const LAYOUT_CONTEXT_UPDATE_VIEW_PORT =
    "LayoutContext.UpdateViewport";
inline constexpr const char* const CATALYZER_TRIGGER_NODE_READY =
    "Catalyzer::TriggerOnNodeReady";

inline constexpr const char* const DOM_CONSTRUCT_ELEMENT_TREE =
    "ConstructElementTree";
inline constexpr const char* const DOM_DUMP_ELEMENT_TREE = "DumpElementTree";

inline constexpr const char* const PAINTING_CONTEXT_CREATE_NODE =
    "CreatePaintingNode";
inline constexpr const char* const PAINTING_CONTEXT_INSERT_NODE =
    "InsertPaintingNode";
inline constexpr const char* const PAINTING_CONTEXT_REMOVE_NODE =
    "PaintingContext::RemovePaintingNode";
inline constexpr const char* const PAINTING_CONTEXT_DESTROY_NODE =
    "DestroyPaintingNode";
inline constexpr const char* const PAINTING_CONTEXT_FINISH_LAYOUT_OPERATION =
    "FinishLayoutOperation";
inline constexpr const char* const PAINTING_CONTEXT_FINISH_TASM_OPERATION =
    "FinishTasmOperation";
inline constexpr const char* const PAINTING_CONTEXT_CLEAN_OPTIONS_FOR_TIMING =
    "CleanOptionsForTiming";
inline constexpr const char* const UI_OPERATION_QUEUE_INSERT_PAINTING_TASK =
    "UIOperationQueue::InsertPaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_REMOVE_PAINTING_TASK =
    "UIOperationQueue::RemovePaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_DESTORY_PAINTING_TASK =
    "UIOperationQueue::DestroyPaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_SCROLL_INFO_TASK =
    "UIOperationQueue::UpdateScrollInfoTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_UPDATE_NODE_READY_PATCHING =
        "UIOperationQueue::UpdateNodeReadyPatchingTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_UPDATE_NODE_RELOAD_PATCHING =
        "UIOperationQueue::UpdateNodeReloadPatchingTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_EVENT_INFO =
    "UIOperationQueue::UpdateEventInfoTask";
inline constexpr const char* const UI_OPERATION_QUEUE_LIST_CELL_WILL_APPEAR =
    "UIOperationQueue::ListCellWillAppearTask";
inline constexpr const char* const UI_OPERATION_QUEUE_LIST_CELL_DISAPPEAR =
    "UIOperationQueue::ListCellDisappearTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_OFFSET_FOR_LIST =
    "UIOperationQueue::UpdateContentOffsetForListContainerTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_INSERT_LIST_PAINTING_TASK =
        "UIOperationQueue::InsertListItemPaintingNodeTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_REMOVE_LIST_PAINTING_TASK =
        "UIOperationQueue::RemoveListItemPaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_SET_NEED_MARK_TIMING =
    "UIOperationQueue::SetNeedMarkDrawEndTimingTask";
inline constexpr const char* const UI_OPERATION_QUEUE_SET_KEYFRAME_TASK =
    "UIOperationQueue::SetKeyframesTask";
inline constexpr const char* const UI_OPERATION_QUEUE_CREATE_PAINTING_NODE =
    "UIOperationQueue::CreatePaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_PAINTING_NODE =
    "UIOperationQueue::UpdatePaintingNodeTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_EXTRA_BUNDLE =
    "UIOperationQueue::UpdatePlatformExtraBundleTask";
inline constexpr const char* const UI_OPERATION_QUEUE_FINISH_LAYOUT_OPERATION =
    "UIOperationQueue::FinishLayoutOperationTask";

inline constexpr const char* const LEPUS_COMPONENT_QUERY_SELECTOR =
    "LepusComponent::QuerySelector";
inline constexpr const char* const LEPUS_COMPONENT_QUERY_SELECTOR_ALL =
    "LepusComponent::QuerySelectorAll";
inline constexpr const char* const LEPUS_COMPONENT_REQUEST_ANIMATION_FRAME =
    "LepusComponent::RequestAnimationFrame";
inline constexpr const char* const LEPUS_COMPONENT_CANCEL_ANIMATION_FRAME =
    "LepusComponent::CancelAnimationFrame";
inline constexpr const char* const LEPUS_COMPONENT_TRIGGER_EVENT =
    "LepusComponent::TriggerEvent";
inline constexpr const char* const LEPUS_COMPONENT_CALL_JS_FUNCTION =
    "LepusComponent::CallJSFunction";
inline constexpr const char* const LEPUS_COMPONENT_DO_FRAME =
    "LepusComponent::DoFrame";
inline constexpr const char* const LEPUS_COMPONENT_GET_STORE =
    "LepusComponent::GetStore";
inline constexpr const char* const LEPUS_COMPONENT_SET_STORE =
    "LepusComponent::SetStore";
inline constexpr const char* const LEPUS_COMPONENT_GET_DATA =
    "LepusComponent::GetStore";
inline constexpr const char* const LEPUS_COMPONENT_SET_DATA =
    "LepusComponent::SetStore";
inline constexpr const char* const LEPUS_COMPONENT_GET_PROPERTIES =
    "LepusComponent::GetProperties";
inline constexpr const char* const LEPUS_ELEMENT_FIRE_ELEMENT_WORKLET =
    "LepusElement::FireElementWorklet";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_WORKLET_FUNC =
    "LepusElement::TriggerWorkletFunction";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_SET_STYLES =
    "LepusElement::SetStyles";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_SET_ATTRIBUTES =
    "LepusElement::SetAttributes";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_GET_ATTRIBUTES =
    "LepusElement::GetAttributes";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_GET_DATASET =
    "LepusElement::GetDataset";
inline constexpr const char* const LEPUS_ELEMENT_TRIGGER_SCROLL_BY =
    "LepusElement::ScrollBy";
inline constexpr const char* const LEPUS_ELEMENT_GET_BOUNDING_CLIENT_RECT =
    "LepusElement::GetBoundingClientRect";
inline constexpr const char* const LEPUS_ELEMENT_GET_COMPUTED_STYLES =
    "LepusElement::GetComputedStyles";
inline constexpr const char* const LEPUS_ELEMENT_INVOKE =
    "LepusElement::Invoke";

inline constexpr const char* const LEPUS_GESTURE_ACTIVE =
    "LepusGesture::Active";
inline constexpr const char* const LEPUS_GESTURE_FAIL = "LepusGesture::Fail";
inline constexpr const char* const LEPUS_GESTURE_END = "LepusGesture::End";
inline constexpr const char* const LEPUS_GESTURE_SCROLL_BY =
    "LepusGesture::ScrollBy";
inline constexpr const char* const LEPUS_MAIN_THREAD_SET_TIMEOUT =
    "MainThread::SetTimeout";
inline constexpr const char* const LEPUS_MAIN_THREAD_SET_INTERVAL =
    "MainThread::SetInterval";
inline constexpr const char* const LEPUS_MAIN_THREAD_INVOKE_TIMEOUT_TASK =
    "MainThread::InvokeSetTimeoutTask";
inline constexpr const char* const LEPUS_MAIN_THREAD_INVOKE_INTERVAL_TASK =
    "MainThread::InvokeSetIntervalTask";
inline constexpr const char* const LEPUS_LYNX_TRIGGER_LEPUS_BRIDGE =
    "LepusLynx::TriggerLepusBridge";
inline constexpr const char* const LEPUS_LYNX_TRIGGER_LEPUS_BRIDGE_SYNC =
    "LepusLynx::TriggerLepusBridge";
inline constexpr const char* const LEPUS_ANIMATION_FRAME_TASK_EXECUTE =
    "LepusAnimationFrameTaskHandler::FrameTask::Execute";
inline constexpr const char* const LEPUS_ANIMATION_FRAME_REQUEST_FRAME =
    "LepusAnimationFrameTaskHandler::RequestAnimationFrame";
inline constexpr const char* const LEPUS_ANIMATION_FRAME_CANCEL_FRAME =
    "LepusAnimationFrameTaskHandler::CancelAnimationFrame";
inline constexpr const char* const LEPUS_ANIMATION_FRAME_DO_FRAME =
    "LepusAnimationFrameTaskHandler::DoFrame";
inline constexpr const char* const LEPUS_API_FUNC_TASK_EXECUTE =
    "LepusApiHandler::FuncTask::Execute";
inline constexpr const char* const LEPUS_API_STORE_TASK =
    "LepusApiHandler::StoreTask";
inline constexpr const char* const LEPUS_API_STORE_TIMED_TASK =
    "LepusApiHandler::StoreTimedTask";
inline constexpr const char* const LEPUS_API_INVOKE_WITH_TASK_ID =
    "LepusApiHandler::InvokeWithTaskID";
inline constexpr const char* const LEPUS_API_INVOKE_WITH_TIMED_TASK_ID =
    "LepusApiHandler::InvokeWithTaskID";
inline constexpr const char* const FRAME_ELEMENT_ON_SET_SRC =
    "FrameElement::OnSetSrc";
inline constexpr const char* const FRAME_ELEMENT_DID_BUNDLED_LOADED =
    "FrameElement::DidBundleLoaded";

inline constexpr const char* const SIGNAL_CONTEXT_COMPLETE_UPDATES =
    "SignalContext::CompleteUpdates";
inline constexpr const char* const SIGNAL_CONTEXT_COMPLETE_UPDATES_UPDATES =
    "SignalContext::CompleteUpdates::Updates";
inline constexpr const char* const SIGNAL_CONTEXT_COMPLETE_UPDATES_EFFECTS =
    "SignalContext::CompleteUpdates::Effects";

inline constexpr const char* const UI_OPERATION_QUEUE_MARK_TIMING =
    "UIOperationQueue::MarkUIOperationQueueFlushTimingTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_SET_NEED_MARK_PAINT_END_TIMING =
        "UIOperationQueue::SetNeedMarkPaintEndTiming";

inline constexpr const char* const UI_OPERATION_QUEUE_CONSUME_GESTURE =
    "UIOperationQueue::ConsumeGesture";

inline constexpr const char* const UI_OPERATION_QUEUE_SET_GESTURE_STATE_TASK =
    "UIOperationQueue::SetGestureDetectorStateTask";

inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_FLATTEN_STATUS =
    "UIOperationQueue::UpdateFlattenStatusTask";

inline constexpr const char* const UI_OPERATION_QUEUE_ENQUEUE_CREATE_VIEW =
    "UIOperationQueue::createNode.enqueueCreateView";
inline constexpr const char* const CATALYZER_UPDATE_PAINTING_NODE =
    "Catalyzer.UpdatePaintingNode";

inline constexpr const char* const UI_OPERATION_QUEUE_FINISH_TASM_OPERATION =
    "UIOperationQueue::FinishTasmOperationTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_LAYOUT_PATCHING =
    "UIOperationQueue::UpdateLayoutPatchingTask";

inline constexpr const char* const PAINTING_CONTEXT_ANDROID_CREAT_NODE =
    "PaintingContextAndroid::CreatePaintingNode";
inline constexpr const char* const
    PAINTING_CONTEXT_ANDROID_SET_CONTEXT_ATTACHED =
        "PaintingContextAndroid::SetContextHasAttached";
inline constexpr const char* const
    PAINTING_CONTEXT_ANDROID_RESET_PAINTING_NODE_CONTAINER =
        "PaintingContextAndroid::ResetCreatePaintingNodeIterableContainer";
inline constexpr const char* const
    PAINTING_CONTEXT_ANDROID_REINIT_PAINTING_NODE_CONTAINER =
        "PaintingContextAndroid::"
        "ReinitializeCreatePaintingNodeIterableContainer";
inline constexpr const char* const
    PAINTING_CONTEXT_ANDROID_FLUSH_UI_OPERATION_BATCH = "FlushUIOperationBatch";

inline constexpr const char* const UI_OPERATION_QUEUE_LIST_REUSE_PAINTING_NODE =
    "UIOperationQueue::ListReusePaintingNodeTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_CREATE_PAINTING_NODE_ASYNC =
        "UIOperationQueue::CreatePaintingNodeAsyncTask";
inline constexpr const char* const
    UI_OPERATION_QUEUE_CREATE_PAINTING_NODE_SYNC =
        "UIOperationQueue::CreatePaintingNodeSyncTask";
inline constexpr const char* const UI_OPERATION_QUEUE_UPDATE_LAYOUT_TASK =
    "UIOperationQueue::UpdateLayoutTask";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_RENDERER_TRACE_RENDERER_TRACE_EVENT_DEF_H_
