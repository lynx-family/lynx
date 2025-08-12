// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_TRACE_RUNTIME_TRACE_EVENT_DEF_H_
#define CORE_RUNTIME_TRACE_RUNTIME_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

/**
 * @trace_description: Execute the callbacks for the @args{type} event on
 * background scripting thread (historically known as "JS Thread").
 */
inline constexpr const char* const CALL_JS_CLOSURE_EVENT = "CallJSClosureEvent";

/**
 * @trace_description: Create virtual Component on Engine thread (historically
 * known as "TASM Thread"). Component's name is @args{componentName}. One or
 * more components form a virtual node tree, which is used for subsequent render
 * and dispatch processes.
 */
inline constexpr const char* const CREATE_VIRTUAL_COMPONENT =
    "CreateVirtualComponent";
/**
 * @trace_description: Update the component's info, such as path, id, and the
 * compiled render function.
 */
inline constexpr const char* const RADON_DIFF_UPDATE_COMPONENT_INFO =
    "UpdateComponentInfo";

inline constexpr const char* const API_CALLBACK_MANAGER_INVOKE_WITH_VALUE =
    "ApiCallBackManager::InvokeWithValue";
inline constexpr const char* const API_CALLBACK_MANAGER_CREATE_CALLBACK =
    "ApiCallBackManager::createCallbackImpl";

inline constexpr const char* const APP_PROXY_UPDATE_DATA = "updateData";
inline constexpr const char* const APP_PROXY_UPDATE_DATA_TO_TASM =
    "UpdateDataToTASM";
inline constexpr const char* const APP_PROXY_UPDATE_COMPONENT_DATA =
    "updateComponentData";
inline constexpr const char* const APP_PROXY_UPDATE_COMPONENT_DATA_TO_TASM =
    "updateComponentDataToTASM";
inline constexpr const char* const APP_PROXY_TRIGGER_COMPONENT_EVENT =
    "triggerComponentEvent";
inline constexpr const char* const APP_PROXY_TRIGGER_COMPONENT_EVENT_TO_TASM =
    "triggerComponentEventToTASM";
inline constexpr const char* const APP_PROXY_SELECT_COMPONENT =
    "selectComponent";
inline constexpr const char* const APP_GET_PATH_INFO = "App::getPathInfo";
inline constexpr const char* const APP_GET_ENV = "App::getEnv";
inline constexpr const char* const APP_GET_FIELDS = "App::getFields";
inline constexpr const char* const APP_INVOKE_UI_METHOD = "App::getEnv";
inline constexpr const char* const APP_CALL_LEPUS_METHOD = "callLepusMethod";
inline constexpr const char* const APP_CALL_LEPUS_METHOD_INNER =
    "CallLepusMethodInner";
inline constexpr const char* const APP_GENERATE_PIPELINE_OPTIONS =
    "generatePipelineOptions";
inline constexpr const char* const APP_TRIGGER_WORKLET_FUNC =
    "triggerWorkletFunction";
inline constexpr const char* const APP_DO_FRAME = "AnimationFrame";

inline constexpr const char* const APP_CALL_DESTROY_LIFE_TIME_FUNC =
    "CardLifeTimeCallback:onDestroy";

inline constexpr const char* const LEPUS_VALUE_TO_JS_VALUE =
    "LepusValueToJSValue";
inline constexpr const char* const PUB_VALUE_TO_JS_VALUE = "PubValueToJSValue";
inline constexpr const char* const JS_VALUE_TO_LEPUS_VALUE =
    "JSValueToLepusValue";
inline constexpr const char* const JS_VALUE_TO_PUB_VALUE = "JSValueToPubValue";
inline constexpr const char* const APP_LOAD_SCRIPT_ASYNC =
    "App::LoadScriptAsync";
inline constexpr const char* const APP_UPDATE_CARD_DATA = "App::updateCardData";
/**
 * @trace_description: After the frontend framework completes the initialization
 * of the App object in the background thread, the Lynx framework will notify
 * the frontend framework to update the page data.
 */
inline constexpr const char* const APP_NOTIFY_JS_UPDATE_PAGE_DATA =
    "NotifyJSUpdatePageData";

inline constexpr const char* const APP_JS_UPDATE_CARD_CONFIG_DATA =
    "App::NotifyJSUpdateCardConfigData";
inline constexpr const char* const APP_EVAL_SCRIPT = "App::EvalScript";
inline constexpr const char* const APP_CALLBACK_TO_LEPUS_EVENT =
    "CallbackToLepusEvent";
inline constexpr const char* const APP_SEND_PAGE_EVENT = "SendPageEvent";
inline constexpr const char* const APP_ON_SCRIPT_LOADED = "OnScriptLoaded";
inline constexpr const char* const BATCHED_UPDATE_DATA_JS_VALUE_TO_LEPUS_VALUE =
    "batchedUpdateData:JSValueToLepusValue";
inline constexpr const char* const UPDATE_DATA_TO_TASM =
    "updateData:UpdateDataToTASM";
inline constexpr const char* const BACKGROUND_THREAD_SET_TIMEOUT =
    "BackgroundThread::SetTimeout";
inline constexpr const char* const BACKGROUND_THREAD_SET_INTERVAL =
    "BackgroundThread::SetInterval";
inline constexpr const char* const BACKGROUND_THREAD_QUEUE_MICROTASK =
    "BackgroundThread::QueueMicrotask";
inline constexpr const char* const APP_PUBLISH_COMPONENT_EVENT =
    "App::PublishComponentEvent";
inline constexpr const char* const JS_UPDATE_COMPONET_DATA =
    "LynxJSUpdateComponentData";

inline constexpr const char* const MODULE_INVOKE_CALLBACK = "InvokeCallback";
inline constexpr const char* const MODULE_INVOKE_FIRE = "Fire";
inline constexpr const char* const MODULE_ON_METHOD_INVOKE = "OnMethodInvoked";
inline constexpr const char* const JSB_TIMING_CALLBACK_CONVERT_PARAMS_START =
    "JSBTiming::jsb_callback_convert_params_start";
inline constexpr const char* const JSB_TIMING_CALLBACK_CONVERT_PARAMS_END =
    "JSBTiming::jsb_callback_convert_params_end";
inline constexpr const char* const JSB_TIMING_CALLBACK_INVOKE_START =
    "JSBTiming::jsb_callback_invoke_start";
inline constexpr const char* const JSB_TIMING_CALLBACK_INVOKE_END =
    "JSBTiming::jsb_callback_invoke_end";
inline constexpr const char* const JSB_TIMING_CALLBACK_CALL_START =
    "JSBTiming::jsb_callback_call_start";
inline constexpr const char* const JSB_TIMING_CALLBACK_CALL_END =
    "JSBTiming::jsb_callback_call_end";
inline constexpr const char* const JSB_TIMING_FUNC_CALL_START =
    "JSBTiming::jsb_func_call_start";
inline constexpr const char* const JSB_TIMING_FUNC_CALL_END =
    "JSBTiming::jsb_func_call_end";
inline constexpr const char* const JSB_TIMING_FUNC_CONVERT_PARAMS_START =
    "JSBTiming::jsb_func_convert_params_start";
inline constexpr const char* const JSB_TIMING_FUNC_CONVERT_PARAMS_END =
    "JSBTiming::jsb_func_convert_params_end";
inline constexpr const char* const JSB_TIMING_FUNC_PLATFORM_METHOD_START =
    "JSBTiming::jsb_func_platform_method_start";
inline constexpr const char* const JSB_TIMING_FUNC_PLATFORM_METHOD_END =
    "JSBTiming::jsb_func_platform_method_end";
inline constexpr const char* const JSB_TIMING_CALLBACK_THREAD_SWITCH_START =
    "JSBTiming::jsb_callback_thread_switch_start";
inline constexpr const char* const JSB_TIMING_CALLBACK_THREAD_SWITCH_END =
    "JSBTiming::jsb_callback_thread_switch_end";
inline constexpr const char* const CREATE_JSB_CALLBACK = "CreateJSB Callback";
inline constexpr const char* const JSB_TIMING_FLUSH = "JSBTiming::Flush";
inline constexpr const char* const JNI_VALUE_TO_JS_VALUE = "JNIValueToJSValue";
inline constexpr const char* const JS_VALUE_TO_JNI_VALUE = "JSValueToJNIValue";
inline constexpr const char* const OBJC_VALUE_TO_JSI_VALUE =
    "ObjCValueToJSIValue";
inline constexpr const char* const JS_VALUE_TO_OBJC_VALUE =
    "JSValueToObjCValue";

inline constexpr const char* const SLOT_FUNCTION = "SlotFunction";
inline constexpr const char* const TRIGGER_GC = "TriggerGC";
inline constexpr const char* const TRIGGER_GC_FOR_TESTING =
    "TriggerGCForTesting";

inline constexpr const char* const JS_CACHE_MANAGER_TRY_GET_CACHE =
    "JsCacheManager::TryGetCache";

inline constexpr const char* const EVALUATE_JAVA_SCRIPT = "evaluateJavaScript";
inline constexpr const char* const EVALUATE_JAVA_SCRIPT_BYTECODE =
    "evaluateJavaScriptBytecode";
inline constexpr const char* const EVALUATE_PREPARED_JAVA_SCRIPT =
    "evaluatePreparedJavaScript";

inline constexpr const char* const LYNX_JS_LOAD_CORE = "LynxJSLoadCore";
inline constexpr const char* const PREPARE_NAPI_ENV =
    "Lynx::PrepareNapiEnvironment";
inline constexpr const char* const CREATE_AND_LOAD_APP = "LynxCreateAndLoadApp";
inline constexpr const char* const TIME_TO_INTERACTIVE = "TimeToInteractive";
inline constexpr const char* const RUNTIME_LIFECYCLE_OBSERVER_RUNTIME_ATTACH =
    "RuntimeLifecycleObserver::OnRuntimeAttach";
inline constexpr const char* const RUNTIME_CALL_JS_API_CALLBACK =
    "CallJSApiCallback";
inline constexpr const char* const RUNTIME_CALL_JS_API_CALLBACK_WITH_VALUE =
    "CallJSApiCallbackWithValue";

inline constexpr const char* const DESERIALIZE_FUNCTION = "DeserializeFunction";
inline constexpr const char* const DESERIALIZE_GLOBAL = "DeserializeGlobal";
inline constexpr const char* const DESERIALIZE_TOP_VARIABLES =
    "DeserializeTopVariables";
inline constexpr const char* const READ_STRING_DIRECTLY = "ReadStringDirectly";
inline constexpr const char* const REGISTER_BUILD_IN = "RegisterBuiltin";
inline constexpr const char* const QUICK_CONTEXT_SET_PROPERTY_CALLBACK =
    "QuickContext::LepusRefSetPropertyCallBack";
inline constexpr const char* const QUICK_CONTEXT_GET_PROPERTY_CALLBACK =
    "QuickContext::LepusRefGetPropertyCallBack";
inline constexpr const char* const QUICK_CONTEXT_CONVERT_TO_OBJECT_CALLBACK =
    "QuickContext::LepusConvertToObjectCallBack";
inline constexpr const char* const CONTEXT_CREATE_QUICK_CONTEXT =
    "Context::CreateQuickContext";
inline constexpr const char* const CONTEXT_CREATE_VM_CONTEXT =
    "Context::CreateVMContext";

inline constexpr const char* const DEVTOOL_ON_NODE_CREATE =
    "Devtool::ON_NODE_CREATE";
inline constexpr const char* const DEVTOOL_ON_AIR_NODE_CREATE =
    "Devtool::ON_AIR_NODE_CREATED";
inline constexpr const char* const DEVTOOL_ON_NODE_MODIFIED =
    "Devtool::ON_NODE_MODIFIED";
inline constexpr const char* const DEVTOOL_ON_NODE_ADDED =
    "Devtool::ON_NODE_ADDED";
inline constexpr const char* const DEVTOOL_ON_NODE_REMOVED =
    "Devtool::ON_NODE_REMOVED";

inline constexpr const char* const INNER_TRANSLATE_RESOURCE_FOR_THEME =
    "InnerTranslateResourceForTheme";
inline constexpr const char* const ATTACH_PAGE = "AttachPage";
inline constexpr const char* const CREATE_VIRTUAL_NODE = "CreateVirtualNode";
inline constexpr const char* const CREATE_VIRTUAL_PAGE = "CreateVirtualPage";
inline constexpr const char* const APPEND_CHILD = "AppendChild";
inline constexpr const char* const APPEND_SUB_TREE = "AppendSubTree";
inline constexpr const char* const CLONE_SUB_TREE = "CloneSubTree";
inline constexpr const char* const SET_ATTRIBUTE_TO = "SetAttributeTo";
inline constexpr const char* const SET_CONTEXT_DATA = "SetContextData";
inline constexpr const char* const SET_STATIC_STYLe_TO_2 = "SetStaticStyleTo2";
inline constexpr const char* const SET_SCRIPT_EVENT_TO = "SetScriptEventTo";
inline constexpr const char* const COMPONENT_INFO_FROM_CONTEXT =
    "ComponentInfoFromContext";
inline constexpr const char* const APPEND_LIST_COMPONENT_INFO =
    "AppendListComponentInfo";
inline constexpr const char* const CREATE_VIRTUAL_PLUG = "CreateVirtualPlug";
inline constexpr const char* const CREATE_VIRTUAL_PLUG_WITH_COMPONENT =
    "CreateVirtualPlugWithComponent";
inline constexpr const char* const MARK_COMPONENT_HAS_RENDER =
    "MarkComponentHasRenderer";
inline constexpr const char* const SET_STATIC_ATTRIBUTE_TO = "SetStaticAttrTo";
inline constexpr const char* const SET_STYLE_TO = "SetStyleTo";
inline constexpr const char* const SET_DYNAMIC_STYLE_TO = "SetDynamicStyleTo";
inline constexpr const char* const SET_STATIC_STYLE_TO = "SetStaticStyleTo";
inline constexpr const char* const SET_DATA_SET_TO = "SetDataSetTo";
inline constexpr const char* const SET_STATIC_EVENT_TO = "SetStaticEventTo";
inline constexpr const char* const SET_CLASS_TO = "SetClassTo";
inline constexpr const char* const SET_STATIC_CLASS_TO = "SetStaticClassTo";
inline constexpr const char* const SET_ID = "SetId";
inline constexpr const char* const GET_COMPONENT_INFO = "GetComponentInfo";
inline constexpr const char* const CREATE_SLOT = "CreateSlot";
inline constexpr const char* const SET_PROP = "SetProp";
inline constexpr const char* const SET_DATA = "SetData";
inline constexpr const char* const APPEND_VIRTUAL_PLUG_TO_COMPONENT =
    "AppendVirtualPlugToComponent";
inline constexpr const char* const ADD_VIRTUAL_PLUG_TO_COMPONENT =
    "AppendVirtualPlugToComponent";
inline constexpr const char* const ADD_FALLBACK_TO_DYNAMIC_COMPONENT =
    "AddFallbackToDynamicComponent";
inline constexpr const char* const GET_COMPONENT_DATA = "GetComponentData";
inline constexpr const char* const GET_COMPONENT_PROPS = "GetComponentProps";
inline constexpr const char* const GET_COMPONENT_CONTEXT_DATA =
    "GetComponentContextData";
inline constexpr const char* const CREATE_COMPONENT_BY_NAME =
    "CreateComponentByName";
inline constexpr const char* const CREATE_DYNAMIC_VIRTUAL_COMPONENT =
    "CreateDynamicVirtualComponent";
inline constexpr const char* const PROCESS_COMPONENT_DATA =
    "ProcessComponentData";
inline constexpr const char* const LAZY_BUNDLE_RENDER_ENTRANCE =
    "LazyBundle::RenderEntrance";
inline constexpr const char* const REGISTER_DATA_PROCESSOR =
    "RegisterDataProcessor";
inline constexpr const char* const ADD_EVENT_LISTENER = "AddEventListener";
inline constexpr const char* const RE_FLUSH_PAGE = "ReFlushPage";
inline constexpr const char* const SET_COMPONENT = "SetComponent";
inline constexpr const char* const REGISTER_ELEMENT_WORKLET =
    "RegisterElementWorklet";
inline constexpr const char* const CREATE_VIRTUAL_LIST_NODE =
    "CreateVirtualListNode";
inline constexpr const char* const SEND_GLOBAL_EVENT = "SendGlobalEvent";
inline constexpr const char* const FIBER_CREATE_ELEMENT = "FiberCreateElement";
inline constexpr const char* const FIBER_CREATE_PAGE = "FiberCreatePage";
inline constexpr const char* const FIBER_GET_PAGE_ELEMENT =
    "FiberGetPageElement";
inline constexpr const char* const FIBER_CREATE_COMPONENT =
    "FiberCreateComponent";
inline constexpr const char* const FIBER_CREATE_VIEW = "FiberCreateView";
inline constexpr const char* const FIBER_CREATE_LIST = "FiberCreateList";
inline constexpr const char* const FIBER_CREATE_SCROLL_VIEW =
    "FiberCreateScrollView";
inline constexpr const char* const FIBER_CREATE_TEXT = "FiberCreateText";
inline constexpr const char* const FIBER_CREATE_IMAGE = "FiberCreateImage";
inline constexpr const char* const FIBER_CREATE_RAW_TEXT = "FiberCreateRawText";
inline constexpr const char* const FIBER_CREATE_IF = "FiberCreateIf";
inline constexpr const char* const FIBER_CREATE_FOR = "FiberCreateFor";
inline constexpr const char* const FIBER_CREATE_BLOCK = "FiberCreateBlock";
inline constexpr const char* const FIBER_ADD_CONFIG = "FiberAddConfig";
inline constexpr const char* const FIBER_SET_CONFIG = "FiberSetConfig";
inline constexpr const char* const FIBER_CREATE_NO_ELEMENT =
    "FiberCreateNonElement";
inline constexpr const char* const FIBER_APPEND_ELEMENT = "FiberAppendElement";
inline constexpr const char* const FIBER_REMOVE_ELEMENT = "FiberRemoveElement";
inline constexpr const char* const FIBER_INSERT_ELEMENT_BEFORE =
    "FiberInsertElementBefore";
inline constexpr const char* const FIBER_FIRST_ELEMENT = "FiberFirstElement";
inline constexpr const char* const FIBER_LAST_ELEMENT = "FiberLastElement";
inline constexpr const char* const FIBER_NEXT_ELEMENT = "FiberNextElement";
inline constexpr const char* const FIBER_ASYNC_RESOLVE_ELEMENT =
    "FiberAsyncResolveElement";
inline constexpr const char* const FIBER_MARK_ASYNC_RESOLVE_ROOT =
    "FiberMarkAsyncResolveRoot";
inline constexpr const char* const FIBER_ASYNC_RESOLVE_SUBTREE_PROPERTY =
    "FiberAsyncResolveSubtreeProperty";
inline constexpr const char* const FIBER_REPLACE_ELEMENT =
    "FiberReplaceElement";
inline constexpr const char* const FIBER_REPLACE_ELEMENTS =
    "FiberReplaceElements";
inline constexpr const char* const FIBER_SWAP_ELEMENT = "FiberSwapElement";
inline constexpr const char* const FIBER_GET_PARENT = "FiberGetParent";
inline constexpr const char* const FIBER_GET_CHILDREN = "FiberGetChildren";
inline constexpr const char* const FIBER_IS_TEMPLATE_ELEMENT =
    "FiberIsTemplateElement";
inline constexpr const char* const FIBER_IS_PART_ELEMENT = "FiberIsPartElement";
inline constexpr const char* const FIBER_MARK_TEMPLATE_ELEMENT =
    "FiberMarkTemplateElement";
inline constexpr const char* const FIBER_MARK_PART_ELEMENT =
    "FiberMarkPartElement";
inline constexpr const char* const FIBER_GET_TEMPLATE_PARTS =
    "FiberGetTemplateParts";
inline constexpr const char* const JSON_VALUE_TO_LEPUS_VALUE =
    "jsonValueTolepusValue";
inline constexpr const char* const FIBER_ELEMENT_IS_EQUAL =
    "FiberElementIsEqual";
inline constexpr const char* const FIBER_GET_ELEMENT_UNIQUE_ID =
    "FiberGetElementUniqueID";
inline constexpr const char* const FIBER_GET_TAG = "FiberGetTag";
inline constexpr const char* const FIBER_SET_ATTRIBUTE = "FiberSetAttribute";
inline constexpr const char* const FIBER_GET_ATTRIBUTE_BY_NAME =
    "FiberGetAttributeByName";
inline constexpr const char* const FIBER_GET_ATTRIBUTE_NAMES =
    "FiberGetAttributeNames";
inline constexpr const char* const FIBER_GET_ATTRIBUTES = "FiberGetAttributes";
inline constexpr const char* const FIBER_ADD_CLASS = "FiberAddClass";
inline constexpr const char* const FIBER_SET_CLASSES = "FiberSetClasses";
inline constexpr const char* const FIBER_GET_CLASSES = "FiberGetClasses";
inline constexpr const char* const FIBER_ADD_INLINE_STYLE =
    "FiberAddInlineStyle";
inline constexpr const char* const FIBER_SET_INLINE_STYLES =
    "FiberSetInlineStyles";
inline constexpr const char* const FIBER_GET_INLINE_STYLES =
    "FiberGetInlineStyles";
inline constexpr const char* const FIBER_SET_PARSED_STYLES =
    "FiberSetParsedStyles";
inline constexpr const char* const FIBER_ADD_EVENT = "FiberAddEvent";
inline constexpr const char* const CREATE_GESTURE_DETECTOR =
    "CreateGestureDetector";
inline constexpr const char* const FIBER_SET_GESTURE_DETECTOR =
    "FiberSetGestureDetector";
inline constexpr const char* const FIBER_REMOVE_GESTURE_DETECTOR =
    "FiberRemoveGestureDetector";
inline constexpr const char* const FIBER_SET_GESTURE_STATE =
    "FiberSetGestureState";
inline constexpr const char* const FIBER_CONSUME_GESTURE =
    "FiberConsumeGesture";
inline constexpr const char* const FIBER_SET_EVENTS = "FiberSetEvents";
inline constexpr const char* const FIBER_GET_EVENT = "FiberGetEvent";
inline constexpr const char* const FIBER_GET_EVENTS = "FiberGetEvents";
inline constexpr const char* const FIBER_SET_ID = "FiberSetID";
inline constexpr const char* const FIBER_GET_ID = "FiberGetID";
inline constexpr const char* const FIBER_ADD_DATA_SET = "FiberAddDataset";
inline constexpr const char* const FIBER_SET_DATA_SET = "FiberSetDataset";
inline constexpr const char* const FIBER_GET_DATA_SET = "FiberGetDataset";
inline constexpr const char* const FIBER_GET_DATA_BY_KEY = "FiberGetDataByKey";
inline constexpr const char* const FIBER_GET_COMPONENT_ID =
    "FiberGetComponentID";
inline constexpr const char* const FIBER_UPDATE_COMPONENT_ID =
    "FiberUpdateComponentID";
inline constexpr const char* const FIBER_UPDATE_LIST_CALLBACKS =
    "FiberUpdateListCallbacks";
inline constexpr const char* const FIBER_SET_CSS_ID = "FiberSetCSSId";
inline constexpr const char* const FIBER_FLUSH_ELEMENT_TREE =
    "FiberFlushElementTree";
inline constexpr const char* const FIBER_ON_LIFECYCLE_EVENT =
    "FiberOnLifecycleEvent";
inline constexpr const char* const FIBER_ELEMENT_FROM_BINARY =
    "FiberElementFromBinary";
inline constexpr const char* const FIBER_QUERY_COMPONENT =
    "FiberQueryComponent";
inline constexpr const char* const FIBER_QUERY_SELECTOR = "FiberQuerySelector";
inline constexpr const char* const FIBER_UPDATE_COMPONENT_INFO =
    "FiberUpdateComponentInfo";
inline constexpr const char* const FIBER_GET_ELEMENT_CONFIG =
    "FiberGetElementConfig";
inline constexpr const char* const FIBER_GET_INLINE_STYLE =
    "FiberGetInlineStyle";
inline constexpr const char* const FIBER_QUERY_SELECTOR_ALL =
    "FiberQuerySelectorAll";
inline constexpr const char* const FIBER_SET_LEPUS_INIT_DATA =
    "FiberSetLepusInitData";
inline constexpr const char* const FIBER_GET_DIFF_DATA = "FiberGetDiffData";
inline constexpr const char* const FIBER_GET_ELEMENT_BY_UNIQUE_ID =
    "FiberGetElementByUniqueID";
inline constexpr const char* const FIBER_UPDATE_IF_NODE_INDEX =
    "FiberUpdateIfNodeIndex";
inline constexpr const char* const FIBER_UPDATE_FOR_CHILD_COUNT =
    "FiberUpdateForChildCount";
inline constexpr const char* const LOAD_LEPUS_CHUNK = "LoadLepusChunk";
inline constexpr const char* const FIBER_CREATE_FRAME = "FiberCreateFrame";
inline constexpr const char* const FIBER_CREATE_ELEMENT_WITH_PROPS =
    "FiberCreateElementWithProperties";
inline constexpr const char* const FIBER_CREATE_SIGNAL = "FiberCreateSignal";
inline constexpr const char* const FIBER_WRITE_SIGNAL = "FiberWriteSignal";
inline constexpr const char* const FIBER_READ_SIGNAL = "FiberReadSignal";
inline constexpr const char* const FIBER_CREATE_COMPUTATION =
    "FiberCreateComputation";
inline constexpr const char* const FIBER_CREATE_MEMO = "FiberCreateMemo";
inline constexpr const char* const FIBER_UN_TRACK = "FiberUnTrack";
inline constexpr const char* const FIBER_RUN_UPDATES = "FiberRunUpdates";
inline constexpr const char* const FIBER_CREATE_SCOPE = "FiberCreateScope";
inline constexpr const char* const FIBER_GET_SCOPE = "FiberGetScope";
inline constexpr const char* const FIBER_CLEAN_UP = "FiberCleanUp";
inline constexpr const char* const FIBER_ON_CLEAN_UP = "FiberOnCleanUp";
inline constexpr const char* const
    ELEMENT_CONTEXT_DELEGATE_FLUSH_ENQUEUED_TASKS =
        "ElementContextDelegate::FlushEnqueuedTasks";
inline constexpr const char* const ELEMENT_CONTEXT_DELEGATE_ENQUEUE_TASK =
    "ElementContextDelegate::EnqueueTask";
inline constexpr const char* const SET_SOURCE_MAP_RELEASE =
    "SetSourceMapRelease";
inline constexpr const char* const REPORT_ERROR = "ReportError";
inline constexpr const char* const LYNX_ADD_REPORTER_CUSTOM_INFO =
    "lynx.AddReporterCustomInfo";

inline constexpr const char* const GENERATE_PIPELINE_OPTIONS =
    "GeneratePipelineOptions";
inline constexpr const char* const ON_PIPELINE_START = "OnPipelineStart";
inline constexpr const char* const BIND_PIPELINE_ID_WITH_TIMING_FLAG =
    "BindPipelineIDWithTimingFlag";
inline constexpr const char* const MARK_TIMING = "MarkTiming";
inline constexpr const char* const CLEAR_TIMEOUT = "ClearTimeout";
inline constexpr const char* const CLEAR_TIME_INTERVAL = "ClearTimeInterval";
inline constexpr const char* const REQUEST_ANIMATION_FRAME =
    "RequestAnimationFrame";
inline constexpr const char* const CANCEL_ANIMATION_FRAME =
    "CancelAnimationFrame";

inline constexpr const char* const RAPID_JSON_VALUE_TO_LEPUS_VALUE =
    "rapidJsonValueTolepusValue";
inline constexpr const char* const QUICKJS_ARRAY_TO_JSON_STRING =
    "qjsArrayToJSONString";
inline constexpr const char* const QUICKJS_OBJECT_TO_JSON_STRING =
    "qjsObjectToJSONString";
inline constexpr const char* const QUICKJS_VALUE_TO_JSON_STRING =
    "qjsValueToJSONString";
inline constexpr const char* const LEPUS_VALUE_TO_JSON_STRING =
    "lepusValueToJSONString";
inline constexpr const char* const LEPUS_VALUE_MAP_TO_JSON_STRING =
    "lepusValueMapToJSONString";
inline constexpr const char* const LEPUS_VALUE_TO_STRING = "lepusValueToString";

inline constexpr const char* const QUICK_CONTEXT_SET_FUNC_FILE_NAME =
    "SetFunctionFileName";
inline constexpr const char* const QUICK_CONTEXT_EXECUTE = "LepusNG.Execute";
inline constexpr const char* const QUICK_CONTEXT_DO_SERIALIZE =
    "LepusNG.DeSerialize";
inline constexpr const char* const QUICK_CONTEXT_EVAL_BINARY =
    "LepusNG.EvalBinary";
inline constexpr const char* const QUICK_CONTEXT_EVAL_SCRIPT =
    "LepusNG.EvalScript";
inline constexpr const char* const QUICK_CONTEXT_CHECK_TABLE_SHADOW_UPDATED =
    "QuickContext::CheckTableShadowUpdatedWithTopLevelVariable";
inline constexpr const char* const QUICK_CONTEXT_UPDATE_TOP_LEVEL_VARIABLE =
    "QuickContext::UpdateTopLevelVariable";
inline constexpr const char* const VM_CONTEXT_INIT = "VMContext::Initialize";
inline constexpr const char* const VM_CONTEXT_EXECUTE = "Lepus.Execute";
inline constexpr const char* const VM_CONTEXT_CALL = "VMContext::Call";
inline constexpr const char* const VM_CONTEXT_CALL_CLOSURE =
    "VMContext::CallClosure";
inline constexpr const char* const VM_CONTEXT_CHECK_TABLE_SHADOW_UPDATED =
    "VMContext::CheckTableShadowUpdatedWithTopLevelVariable";
inline constexpr const char* const VM_CONTEXT_CALL_C_FUNCTION =
    "VMContext::CallCFunction";
inline constexpr const char* const VM_CONTEXT_CLEAN_CLOSURES_AFTER_EXECUTED =
    "CleanUpClosuresCreatedAfterExecuted";
inline constexpr const char* const VM_CONTEXT_CONSTRUCTION =
    "VMContext::VMContext";
inline constexpr const char* const ANIMATION_FRAME_TASK_EXECUTE =
    "AnimationFrameTaskHandler::FrameTask::Execute";
inline constexpr const char* const
    ANIMATION_FRAME_TASK_REQUEST_ANIMATION_FRAME =
        "AnimationFrameTaskHandler::RequestAnimationFrame";
inline constexpr const char* const ANIMATION_FRAME_TASK_CANCEL_ANIMATION_FRAME =
    "AnimationFrameTaskHandler::CancelAnimationFrame";
inline constexpr const char* const ANIMATION_FRAME_TASK_DO_FRAME =
    "AnimationFrameTaskHandler::DoFrame";
inline constexpr const char* const
    CLOSURE_EVENT_LISTENER_CONVERT_TO_PIPER_VALUE =
        "JSClosureEventListener::ConvertEventToPiperValue";
/**
 * @trace_description: Invoke the NativeModule method with module name
 * @args{module}, method name @args(method} and first_args @args{first_arg}.
 * @history_name{CallJSB}
 */
inline constexpr const char* const INVOKE_NATIVE_MODULE = "InvokeNativeModule";
/*
 * @trace_description: Convert the updated value into LepusValue. Then send this
 * value to the Engine thread to trigger the component's update process.
 */
inline constexpr const char* const BATCHED_UPDATE_DATA = "batchedUpdateData";

/**
 * @trace_description: Load, parse, and execute @args{url}.
 */
inline constexpr const char* const APP_LOAD_SCRIPT = "App::loadScript";
/**
 * @trace_description: Load, parse and execute background scripts @args{url}.
 */
inline constexpr const char* const LOAD_JS_APP = "LoadJSApp";
/**
 * @trace_description: Execute @args{name} on background scripting
 * thread(historically known as "JS Thread").
 */
inline constexpr const char* const RUNNING_IN_JS = "RunningInJS";

/**
 * @trace_description: Call @args{name} on Engine thread (historically known as
 * "Tasm Thread").
 */
inline constexpr const char* const QUICK_CONTEXT_CALL = "QuickContext::Call";

/**
 * @trace_description: Get and call the main script's global function:
 * @args{name}.
 */
inline constexpr const char* const QUICK_CONTEXT_GET_AND_CALL =
    "QuickContext::GetAndCall";

/**
 * @trace_description: Create a <wrapper/> element, a special
 * element provided by the FiberElement API designed to serve as a low-cost
 * container.
 */
inline constexpr const char* const FIBER_ELEMENT_CREATE_WRAPPER_ELEMENT =
    "FiberCreateWrapperElement";
/**
 * @trace_description: Send a request of LazyBundle on Background Thread
 * Script(BTS). It only occurs when rendering LazyBundle on ReactLynx3.
 */
inline constexpr const char* const APP_QUERY_COMPONENT = "App::QueryComponent";

#if ENABLE_AIR
inline constexpr const char* const AIR_CREATE_ELEMENT = "AirCreateElement";
inline constexpr const char* const AIR_GET_ELEMENT = "AirGetElement";
inline constexpr const char* const AIR_CREATE_PAGE = "AirCreatePage";
inline constexpr const char* const AIR_CREATE_COMPONENT = "AirCreateComponent";
inline constexpr const char* const AIR_CREATE_BLOCK = "AirCreateBlock";
inline constexpr const char* const AIR_CREATE_IF = "AirCreateIf";
inline constexpr const char* const AIR_CREATE_FOR = "AirCreateFor";
inline constexpr const char* const AIR_CREATE_PLUG = "AirCreatePlug";
inline constexpr const char* const AIR_CREATE_SLOT = "AirCreateSlot";
inline constexpr const char* const AIR_APPEND_ELEMENT = "AirAppendElement";
inline constexpr const char* const AIR_REMOVE_ELEMENT = "AirRemoveElement";
inline constexpr const char* const AIR_INSERT_ELEMENT_BEFORE =
    "AirInsertElementBefore";
inline constexpr const char* const AIR_GET_ELEMENT_UNIQUE_ID =
    "AirGetElementUniqueID";
inline constexpr const char* const AIR_GET_ELEMENT_TAG = "AirGetElementTag";
inline constexpr const char* const AIR_SET_ATTRIBUTE = "AirSetAttribute";
inline constexpr const char* const AIR_SET_INLINE_STYLES = "AirSetInlineStyles";
inline constexpr const char* const AIR_SET_EVENT = "AirSetEvent";
inline constexpr const char* const AIR_SET_ID = "AirSetID";
inline constexpr const char* const AIR_GET_ELEMENT_BY_ID = "AirGetElementByID";
inline constexpr const char* const AIR_GET_ELEMENT_BY_UNIQUE_ID =
    "AirGetElementByUniqueID";
inline constexpr const char* const AIR_GET_ROOT_ELEMENT = "AirGetRootElement";
inline constexpr const char* const AIR_GET_ELEMENT_BY_LEPUS_ID =
    "AirGetElementByLepusID";
inline constexpr const char* const AIR_UPDATE_IF_NODE_INDEX =
    "AirUpdateIfNodeIndex";
inline constexpr const char* const AIR_UPDATE_FOR_NODE_INDEX =
    "AirUpdateForNodeIndex";
inline constexpr const char* const AIR_UPDATE_FOR_CHILD_COUNT =
    "AirUpdateForChildCount";
inline constexpr const char* const AIR_GET_FOR_NODE_CHILD_WITH_INDEX =
    "AirGetForNodeChildWithIndex";
inline constexpr const char* const AIR_PUSH_FOR_NODE = "AirPushForNode";
inline constexpr const char* const AIR_POP_FOR_NODE = "AirPopForNode";
inline constexpr const char* const AIR_GET_CHILD_ELEMENT_BY_INDEX =
    "AirGetChildElementByIndex";
inline constexpr const char* const AIR_PUSH_DYNAMIC_NODE = "AirPushDynamicNode";
inline constexpr const char* const AIR_GET_DYNAMIC_NODE = "AirGetDynamicNode";
inline constexpr const char* const AIR_SET_COMPONENT_PROP =
    "AirSetComponentProp";
inline constexpr const char* const AIR_RENDER_COMPONENT_IN_LEPUS =
    "AirRenderComponentInLepus";
inline constexpr const char* const AIR_UPDATE_COMPONENT_IN_LEPUS =
    "AirUpdateComponentInLepus";
inline constexpr const char* const AIR_GET_COMPONENT_INFO =
    "AirGetComponentInfo";
inline constexpr const char* const AIR_UPDATE_COMPONENT_INFO =
    "AirUpdateComponentInfo";
inline constexpr const char* const AIR_GET_DATA = "AirGetData";
inline constexpr const char* const AIR_GET_PROPS = "AirGetProps";
inline constexpr const char* const AIR_SET_DATA = "AirSetData";
inline constexpr const char* const AIR_FLUSH_RECURSIVELY =
    "AirFlushRecursively";
inline constexpr const char* const TRIGGER_LEPUS_BRIDGE = "TriggerLepusBridge";
inline constexpr const char* const TRIGGER_LEPUS_BRIDGE_ASYNC =
    "TriggerLepusBridgeSync";
inline constexpr const char* const TRIGGER_COMPONENT_EVENT =
    "TriggerComponentEvent";
inline constexpr const char* const AIR_SET_DATA_SET = "AirSetDataSet";
inline constexpr const char* const AIR_SEND_GLOBAL_EVENT = "AirSendGlobalEvent";
inline constexpr const char* const REMOVE_EVENT_LISTENER =
    "RemoveEventListener";
inline constexpr const char* const AIR_CREATE_RAW_TEXT = "AirCreateRawText";
inline constexpr const char* const AIR_SET_CLASSES = "AirSetClasses";
inline constexpr const char* const AIR_PUSH_COMPONENT_NODE =
    "AirPushComponentNode";
inline constexpr const char* const AIR_POP_COMPONENT_NODE =
    "AirPopComponentNode";
inline constexpr const char* const AIR_GET_PARENT_FOR_NODE =
    "AirGetParentForNode";
inline constexpr const char* const AIR_FLUSH_TREE = "AirFlushTree";
#endif  // ENABLE_AIR
#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_RUNTIME_TRACE_RUNTIME_TRACE_EVENT_DEF_H_
