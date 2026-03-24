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

/**
 * @trace_description: Create a JS callback entry used for native-to-JS async
 * invocation, and return the callback id to the caller.
 */
inline constexpr const char* const API_CALLBACK_MANAGER_CREATE_CALLBACK =
    "ApiCallBackManager::createCallbackImpl";

/**
 * @trace_description: Update the page-level data model from JS, and trigger the
 * subsequent diff/render pipeline.
 */
inline constexpr const char* const APP_PROXY_UPDATE_DATA = "updateData";
/**
 * @trace_description: Transfer the updated page data from background scripting
 * thread to Engine thread (historically known as "TASM Thread") for rendering.
 */
inline constexpr const char* const APP_PROXY_UPDATE_DATA_TO_TASM =
    "UpdateDataToTASM";
/**
 * @trace_description: Update a component's data payload from JS, typically for
 * partial updates that target a specific component instance.
 */
inline constexpr const char* const APP_PROXY_UPDATE_COMPONENT_DATA =
    "updateComponentData";
/**
 * @trace_description: Transfer the updated component data from background
 * scripting thread to Engine thread for diff/render.
 */
inline constexpr const char* const APP_PROXY_UPDATE_COMPONENT_DATA_TO_TASM =
    "updateComponentDataToTASM";
/**
 * @trace_description: Trigger a component event dispatch from JS (e.g., tap,
 * change), and route it to the native event handling pipeline.
 */
inline constexpr const char* const APP_PROXY_TRIGGER_COMPONENT_EVENT =
    "triggerComponentEvent";
/**
 * @trace_description: Transfer a component event dispatch request from
 * background scripting thread to Engine thread for handling.
 */
inline constexpr const char* const APP_PROXY_TRIGGER_COMPONENT_EVENT_TO_TASM =
    "triggerComponentEventToTASM";
/**
 * @trace_description: Query and select components using a selector, returning
 * results (e.g., nodes/fields) to the caller.
 */
inline constexpr const char* const APP_PROXY_SELECT_COMPONENT =
    "selectComponent";
/**
 * @trace_description: Query path-related info for a resource or node (e.g.,
 * for resolving template/assets).
 */
inline constexpr const char* const APP_GET_PATH_INFO = "App::getPathInfo";
/**
 * @trace_description: Query the runtime environment information exposed to JS.
 */
inline constexpr const char* const APP_GET_ENV = "App::getEnv";
/**
 * @trace_description: Query multiple fields from the native side and return
 * them to JS (e.g., layout/measure/scroll related fields).
 */
inline constexpr const char* const APP_GET_FIELDS = "App::getFields";
/**
 * @trace_description: Invoke a UI method on the native view/component side
 * from JS (e.g., scrollTo, focus, setNativeProps).
 */
inline constexpr const char* const APP_INVOKE_UI_METHOD = "App::invokeUIMethod";
/**
 * @trace_description: Call a Lepus method exported by the template runtime,
 * bridging arguments/results between JS and Lepus.
 */
inline constexpr const char* const APP_CALL_LEPUS_METHOD = "callLepusMethod";
/**
 * @trace_description: Execute the internal path of calling a Lepus method
 * (validation, conversion, dispatch), used by App/Lepus bridge.
 */
inline constexpr const char* const APP_CALL_LEPUS_METHOD_INNER =
    "CallLepusMethodInner";
/**
 * @trace_description: Generate pipeline options for template loading/rendering
 * based on inputs such as url, page options and runtime configuration.
 */
inline constexpr const char* const APP_GENERATE_PIPELINE_OPTIONS =
    "generatePipelineOptions";
/**
 * @trace_description: Trigger execution of a worklet function (off-main-thread
 * or isolated execution unit) from the App runtime.
 */
inline constexpr const char* const APP_TRIGGER_WORKLET_FUNC =
    "triggerWorkletFunction";
/**
 * @trace_description: Execute one animation frame tick and run scheduled frame
 * callbacks for the current page/runtime.
 */
inline constexpr const char* const APP_DO_FRAME = "AnimationFrame";

/**
 * @trace_description: Invoke the card lifecycle onDestroy callback when a card
 * is being torn down.
 */
inline constexpr const char* const APP_CALL_DESTROY_LIFE_TIME_FUNC =
    "CardLifeTimeCallback:onDestroy";

/**
 * @trace_description: Convert a Lepus value into a JS runtime value for
 * bridging results/events from Lepus to JS.
 */
inline constexpr const char* const LEPUS_VALUE_TO_JS_VALUE =
    "LepusValueToJSValue";
/**
 * @trace_description: Convert a Pub value into a JS runtime value for bridging
 * between Pub and JS.
 */
inline constexpr const char* const PUB_VALUE_TO_JS_VALUE = "PubValueToJSValue";
/**
 * @trace_description: Convert a JS runtime value into a Lepus value for
 * bridging arguments/data from JS to Lepus.
 */
inline constexpr const char* const JS_VALUE_TO_LEPUS_VALUE =
    "JSValueToLepusValue";
/**
 * @trace_description: Convert a JS runtime value into a Pub value for bridging
 * arguments/data from JS to Pub.
 */
inline constexpr const char* const JS_VALUE_TO_PUB_VALUE = "JSValueToPubValue";
/**
 * @trace_description: Asynchronously load and execute a script resource
 * referenced by @args{url}.
 */
inline constexpr const char* const APP_LOAD_SCRIPT_ASYNC =
    "App::LoadScriptAsync";
/**
 * @trace_description: Update the card's data payload and trigger re-render or
 * component update pipeline.
 */
inline constexpr const char* const APP_UPDATE_CARD_DATA = "App::updateCardData";
/**
 * @trace_description: After the frontend framework completes the initialization
 * of the App object in the background thread, the Lynx framework will notify
 * the frontend framework to update the page data.
 */
inline constexpr const char* const APP_NOTIFY_JS_UPDATE_PAGE_DATA =
    "NotifyJSUpdatePageData";

/**
 * @trace_description: Notify the JS framework to update card config data after
 * native initialization or config changes.
 */
inline constexpr const char* const APP_JS_UPDATE_CARD_CONFIG_DATA =
    "App::NotifyJSUpdateCardConfigData";
/**
 * @trace_description: Evaluate a JS script in the current JS runtime.
 */
inline constexpr const char* const APP_EVAL_SCRIPT = "App::EvalScript";
/**
 * @trace_description: Convert a native callback result into a Lepus event and
 * dispatch it to the Lepus/templating layer.
 */
inline constexpr const char* const APP_CALLBACK_TO_LEPUS_EVENT =
    "CallbackToLepusEvent";
/**
 * @trace_description: Send a page-level event from native to JS.
 */
inline constexpr const char* const APP_SEND_PAGE_EVENT = "SendPageEvent";
/**
 * @trace_description: Handle script loaded completion, including post-load
 * initialization hooks and error propagation.
 */
inline constexpr const char* const APP_ON_SCRIPT_LOADED = "OnScriptLoaded";
/**
 * @trace_description: Convert batched update data items from JS values into
 * Lepus values before dispatching them to the Engine thread.
 */
inline constexpr const char* const BATCHED_UPDATE_DATA_JS_VALUE_TO_LEPUS_VALUE =
    "batchedUpdateData:JSValueToLepusValue";
/**
 * @trace_description: Transfer updated data from background scripting thread to
 * Engine thread as part of updateData processing.
 */
inline constexpr const char* const UPDATE_DATA_TO_TASM =
    "updateData:UpdateDataToTASM";
/**
 * @trace_description: Schedule a JS timer task via setTimeout on background
 * scripting thread.
 */
inline constexpr const char* const BACKGROUND_THREAD_SET_TIMEOUT =
    "BackgroundThread::SetTimeout";
/**
 * @trace_description: Schedule a repeating JS timer task via setInterval on
 * background scripting thread.
 */
inline constexpr const char* const BACKGROUND_THREAD_SET_INTERVAL =
    "BackgroundThread::SetInterval";
/**
 * @trace_description: Enqueue a microtask to be executed on the background
 * scripting thread microtask queue.
 */
inline constexpr const char* const BACKGROUND_THREAD_QUEUE_MICROTASK =
    "BackgroundThread::QueueMicrotask";
/**
 * @trace_description: Publish a component event from native to JS subscribers.
 */
inline constexpr const char* const APP_PUBLISH_COMPONENT_EVENT =
    "App::PublishComponentEvent";
/**
 * @trace_description: Execute the JS-side component data update entrypoint for
 * Lynx (used by framework-driven updates).
 */
inline constexpr const char* const JS_UPDATE_COMPONET_DATA =
    "LynxJSUpdateComponentData";

/**
 * @trace_description: Invoke a stored callback for a native module call.
 */
inline constexpr const char* const NATIVE_MODULE_CALLBACK =
    "NativeModule::Callback";
/**
 * @trace_description: Invoke a generic callback function holder used by module
 * manager to dispatch callbacks into JS.
 */
inline constexpr const char* const MODULE_INVOKE_CALLBACK = "InvokeCallback";
/**
 * @trace_description: Call a platform-specific implementation for a module
 * method, bridging execution from JS to native platform layer.
 */
inline constexpr const char* const CALL_PLATFORM_IMPLEMENTATION =
    "CallPlatformImplementation";
/**
 * @trace_description: Mark the start of a platform callback path for a module
 * call, used for latency tracking across thread hops.
 * @history_name{JSBTiming::jsb_callback_thread_switch_start}
 */
inline constexpr const char* const NATIVE_MODULE_PLATFORM_CALLBACK_START =
    "NativeModule::PlatformCallbackStart";
/**
 * @trace_description: Create a JSB callback wrapper that can be invoked from
 * native and forwarded to JS.
 */
inline constexpr const char* const CREATE_JSB_CALLBACK = "CreateJSB Callback";
/**
 * @trace_description: Convert JNI value into a JS runtime value for Android
 * bridge interop.
 */
inline constexpr const char* const JNI_VALUE_TO_JS_VALUE = "JNIValueToJSValue";
/**
 * @trace_description: Convert Pub value into a JNI value for Android bridge
 * interop.
 */
inline constexpr const char* const PUB_VALUE_TO_JNI_VALUE =
    "PubValueToJNIValue";
/**
 * @trace_description: Convert Objective-C value into a JS runtime value for
 * Apple platform bridge interop.
 */
inline constexpr const char* const OBJC_VALUE_TO_JSI_VALUE =
    "ObjCValueToJSIValue";
/**
 * @trace_description: Convert Pub value into an Objective-C value for Apple
 * platform bridge interop.
 */
inline constexpr const char* const PUB_VALUE_TO_OBJC_VALUE =
    "PubValueToObjCValue";
/**
 * @trace_description: Execute a slot function bridging call from template
 * runtime.
 */
inline constexpr const char* const SLOT_FUNCTION = "SlotFunction";
/**
 * @trace_description: Trigger a garbage collection cycle in the JS engine.
 */
inline constexpr const char* const TRIGGER_GC = "TriggerGC";
/**
 * @trace_description: Trigger a garbage collection cycle for testing or debug
 * scenarios.
 */
inline constexpr const char* const TRIGGER_GC_FOR_TESTING =
    "TriggerGCForTesting";

/**
 * @trace_description: Try to fetch a compiled JS cache entry from cache manager
 * for faster startup.
 */
inline constexpr const char* const JS_CACHE_MANAGER_TRY_GET_CACHE =
    "JsCacheManager::TryGetCache";

/**
 * @trace_description: Evaluate a JS source string in the JS engine.
 */
inline constexpr const char* const EVALUATE_JAVA_SCRIPT = "evaluateJavaScript";
/**
 * @trace_description: Evaluate a JS bytecode buffer in the JS engine.
 */
inline constexpr const char* const EVALUATE_JAVA_SCRIPT_BYTECODE =
    "evaluateJavaScriptBytecode";

/**
 * @trace_description: Load the core runtime JS bundles (e.g., lynx_core.js) and
 * initialize the JS environment baseline.
 */
inline constexpr const char* const LYNX_JS_LOAD_CORE = "LynxJSLoadCore";
/**
 * @trace_description: Collect and build the preload JS sources (core JS and
 * additional preload scripts) before creating the JS runtime.
 */
inline constexpr const char* const BTS_RUNTIME_PRELOAD_JS_SOURCES_GETTER =
    "BTSRuntime::PreloadJSSourcesGetter";
/**
 * @trace_description: Prepare and attach N-API environment to the runtime so
 * native modules can be loaded and invoked.
 */
inline constexpr const char* const PREPARE_NAPI_ENV =
    "Lynx::PrepareNapiEnvironment";
/**
 * @trace_description: Create the native App instance and perform initial app
 * load/bootstrapping steps in the runtime.
 */
inline constexpr const char* const CREATE_AND_LOAD_APP = "LynxCreateAndLoadApp";
/**
 * @trace_description: Mark the moment when runtime reaches "interactive"
 * (runtime-ready) state for startup performance measurement.
 */
inline constexpr const char* const TIME_TO_INTERACTIVE = "TimeToInteractive";
/**
 * @trace_description: Load the pre-JS bundles required before template script
 * execution, and create/init the JS runtime.
 */
inline constexpr const char* const JS_EXECUTOR_LOAD_PRE_JS_BUNDLE =
    "JSExecutor::loadPreJSBundle";
/**
 * @trace_description: Create a JS runtime according to @args{group_id} and
 * page options, including selecting engine type and sharing context if needed.
 */
inline constexpr const char* const RUNTIME_MANAGER_CREATE_JS_RUNTIME =
    "RuntimeManager::CreateJSRuntime";
/**
 * @trace_description: Create a runtime in single-context mode (no shared
 * JSContext across pages).
 */
inline constexpr const char* const
    RUNTIME_MANAGER_CREATE_SINGLE_CONTEXT_RUNTIME =
        "RuntimeManager::CreateSingleContextRuntime";
/**
 * @trace_description: Lookup an existing shared JSContext by @args{group_id}.
 */
inline constexpr const char* const RUNTIME_MANAGER_GET_SHARED_JS_CONTEXT =
    "RuntimeManager::GetSharedJSContext";
/**
 * @trace_description: Reuse an already-created shared JSContext for the same
 * @args{group_id}.
 */
inline constexpr const char* const RUNTIME_MANAGER_SHARED_CONTEXT_REUSED =
    "RuntimeManager::SharedContextReused";
/**
 * @trace_description: Create the shared JSContext for @args{group_id} for the
 * first time and register it for subsequent reuse.
 */
inline constexpr const char* const
    RUNTIME_MANAGER_CREATE_SHARED_CONTEXT_FIRST_TIME =
        "RuntimeManager::CreateSharedContextFirstTime";
/**
 * @trace_description: Initialize the global object/builtins for a new
 * JSContextWrapper (global runtime setup).
 */
inline constexpr const char* const RUNTIME_MANAGER_CONTEXT_WRAPPER_INIT_GLOBAL =
    "RuntimeManager::JSContextWrapperInitGlobal";
/**
 * @trace_description: Prepare JS environment by evaluating preloaded scripts
 * (e.g., core JS) inside the context wrapper.
 */
inline constexpr const char* const
    RUNTIME_MANAGER_CONTEXT_WRAPPER_PREPARE_JS_ENV =
        "RuntimeManager::JSContextWrapperPrepareJSEnv";
/**
 * @trace_description: Create an engine runtime instance by (MakeRuntime), and
 * set some runtime params.
 */
inline constexpr const char* const RUNTIME_MANAGER_CREATE_RUNTIME =
    "RuntimeManager::CreateRuntime";
/**
 * @trace_description: Real make js engine's runtime.
 */
inline constexpr const char* const RUNTIME_MANAGER_MAKE_RUNTIME =
    "RuntimeManager::MakeRuntime";
/**
 * @trace_description: Evaluate the preloaded JS scripts in the JS runtime to
 * finish JS environment initialization.
 */
inline constexpr const char* const JS_CONTEXT_WRAPPER_PREPARE_JS_ENV =
    "JSContextWrapper::prepareJSEnv";
/**
 * @trace_description: Notify runtime lifecycle observers that the JS runtime
 * is attached and native modules can bind to it.
 */
inline constexpr const char* const RUNTIME_LIFECYCLE_OBSERVER_RUNTIME_ATTACH =
    "RuntimeLifecycleObserver::OnRuntimeAttach";
/**
 * @trace_description: Invoke a BTS API callback on background scripting thread,
 * typically for native-to-JS async completion.
 * @history_name{CallJSApiCallback}
 */
inline constexpr const char* const RUNTIME_CALL_JS_API_CALLBACK =
    "LynxRuntime::CallJSApiCallback";
/**
 * @trace_description: Execute BTS callbacks on the background scripting thread.
 * They are typically triggered by scenarios such as: UI operation result
 * returns, component update or loading completion, resource loading status
 * notifications, and event handling responses, etc.
 * @history_name{CallJSApiCallbackWithValue}
 */
inline constexpr const char* const RUNTIME_CALL_JS_API_CALLBACK_WITH_VALUE =
    "LynxRuntime::CallJSApiCallbackWithValue";

/**
 * @trace_description: Deserialize a function object from serialized bytecode
 * or snapshot data.
 */
inline constexpr const char* const DESERIALIZE_FUNCTION = "DeserializeFunction";
/**
 * @trace_description: Deserialize global object/state from serialized data.
 */
inline constexpr const char* const DESERIALIZE_GLOBAL = "DeserializeGlobal";
/**
 * @trace_description: Deserialize top-level variables from serialized data.
 */
inline constexpr const char* const DESERIALIZE_TOP_VARIABLES =
    "DeserializeTopVariables";
/**
 * @trace_description: Read a string value directly from serialized/encoded
 * buffer without additional decoding steps.
 */
inline constexpr const char* const READ_STRING_DIRECTLY = "ReadStringDirectly";
/**
 * @trace_description: Register built-in functions/types into the runtime
 * before user script execution.
 */
inline constexpr const char* const REGISTER_BUILD_IN = "RegisterBuiltin";
/**
 * @trace_description: Handle QuickContext property set callback for LepusRef,
 * used by Lepus-JS bridging.
 */
inline constexpr const char* const QUICK_CONTEXT_SET_PROPERTY_CALLBACK =
    "QuickContext::LepusRefSetPropertyCallBack";
/**
 * @trace_description: Handle QuickContext property get callback for LepusRef,
 * used by Lepus-JS bridging.
 */
inline constexpr const char* const QUICK_CONTEXT_GET_PROPERTY_CALLBACK =
    "QuickContext::LepusRefGetPropertyCallBack";
/**
 * @trace_description: Convert a LepusRef into a JS object representation in
 * QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_CONVERT_TO_OBJECT_CALLBACK =
    "QuickContext::LepusConvertToObjectCallBack";
/**
 * @trace_description: Create a QuickContext for executing LepusNG in the
 * runtime.
 */
inline constexpr const char* const CONTEXT_CREATE_QUICK_CONTEXT =
    "Context::CreateQuickContext";
/**
 * @trace_description: Create a VM-backed context for executing Lepus in the
 * runtime.
 */
inline constexpr const char* const CONTEXT_CREATE_VM_CONTEXT =
    "Context::CreateVMContext";
/**
 * @trace_description: Try to execute a callable (script/function) in the
 * context, capturing error information on failure.
 */
inline constexpr const char* const CONTEXT_TRY_EXECUTE = "Context::TryExecute";

/**
 * @trace_description: Emit a devtool event when a node is created.
 */
inline constexpr const char* const DEVTOOL_ON_NODE_CREATE =
    "Devtool::ON_NODE_CREATE";
/**
 * @trace_description: Emit a devtool event when a node is modified.
 */
inline constexpr const char* const DEVTOOL_ON_NODE_MODIFIED =
    "Devtool::ON_NODE_MODIFIED";
/**
 * @trace_description: Emit a devtool event when a node is added.
 */
inline constexpr const char* const DEVTOOL_ON_NODE_ADDED =
    "Devtool::ON_NODE_ADDED";
/**
 * @trace_description: Emit a devtool event when a node is removed.
 */
inline constexpr const char* const DEVTOOL_ON_NODE_REMOVED =
    "Devtool::ON_NODE_REMOVED";

/**
 * @trace_description: Translate resources under theme rules (e.g., swap assets
 * or styles by theme) during template processing.
 */
inline constexpr const char* const INNER_TRANSLATE_RESOURCE_FOR_THEME =
    "InnerTranslateResourceForTheme";
/**
 * @trace_description: Attach a page into the render/runtime pipeline so it can
 * participate in subsequent layout, render and event dispatch stages.
 */
inline constexpr const char* const ATTACH_PAGE = "AttachPage";
/**
 * @trace_description: Create a virtual node in the engine-side virtual tree,
 * used as an intermediate representation for rendering.
 */
inline constexpr const char* const CREATE_VIRTUAL_NODE = "CreateVirtualNode";
/**
 * @trace_description: Create the virtual page root node that anchors the
 * virtual tree for a page.
 */
inline constexpr const char* const CREATE_VIRTUAL_PAGE = "CreateVirtualPage";
/**
 * @trace_description: Append a child virtual node to a parent node.
 */
inline constexpr const char* const APPEND_CHILD = "AppendChild";
/**
 * @trace_description: Append a whole subtree under a parent node.
 */
inline constexpr const char* const APPEND_SUB_TREE = "AppendSubTree";
/**
 * @trace_description: Clone an existing subtree to support reuse or dynamic
 * component creation.
 */
inline constexpr const char* const CLONE_SUB_TREE = "CloneSubTree";
/**
 * @trace_description: Set an attribute on a virtual node (static/dynamic attrs
 * resolved by template runtime).
 */
inline constexpr const char* const SET_ATTRIBUTE_TO = "SetAttributeTo";
/**
 * @trace_description: Set context data associated with the current rendering
 * context (page/component scoped).
 */
inline constexpr const char* const SET_CONTEXT_DATA = "SetContextData";
/**
 * @trace_description: Apply static style data to a node using a specific
 * optimized path/version.
 */
inline constexpr const char* const SET_STATIC_STYLe_TO_2 = "SetStaticStyleTo2";
/**
 * @trace_description: Bind a script event handler to a node for later dispatch
 * from native to JS.
 */
inline constexpr const char* const SET_SCRIPT_EVENT_TO = "SetScriptEventTo";
/**
 * @trace_description: Build component metadata from context (e.g., resolved
 * component name/path/id) for subsequent render steps.
 */
inline constexpr const char* const COMPONENT_INFO_FROM_CONTEXT =
    "ComponentInfoFromContext";
/**
 * @trace_description: Append list component metadata into the component list
 * used by list rendering/virtualization.
 */
inline constexpr const char* const APPEND_LIST_COMPONENT_INFO =
    "AppendListComponentInfo";
/**
 * @trace_description: Create a virtual plug node used by plugin/extension
 * mechanism inside the virtual tree.
 */
inline constexpr const char* const CREATE_VIRTUAL_PLUG = "CreateVirtualPlug";
/**
 * @trace_description: Create a virtual plug and associate it with a component.
 */
inline constexpr const char* const CREATE_VIRTUAL_PLUG_WITH_COMPONENT =
    "CreateVirtualPlugWithComponent";
/**
 * @trace_description: Mark a component as having rendered output, which can be
 * used to optimize subsequent updates.
 */
inline constexpr const char* const MARK_COMPONENT_HAS_RENDER =
    "MarkComponentHasRenderer";
/**
 * @trace_description: Set a static attribute on a node (fast path for static
 * attributes).
 */
inline constexpr const char* const SET_STATIC_ATTRIBUTE_TO = "SetStaticAttrTo";
/**
 * @trace_description: Set styles on a node (combined or resolved styles).
 */
inline constexpr const char* const SET_STYLE_TO = "SetStyleTo";
/**
 * @trace_description: Set dynamic style values on a node.
 */
inline constexpr const char* const SET_DYNAMIC_STYLE_TO = "SetDynamicStyleTo";
/**
 * @trace_description: Set static style values on a node.
 */
inline constexpr const char* const SET_STATIC_STYLE_TO = "SetStaticStyleTo";
/**
 * @trace_description: Set dataset attributes on a node.
 */
inline constexpr const char* const SET_DATA_SET_TO = "SetDataSetTo";
/**
 * @trace_description: Bind a static event handler to a node.
 */
inline constexpr const char* const SET_STATIC_EVENT_TO = "SetStaticEventTo";
/**
 * @trace_description: Set class attribute on a node.
 */
inline constexpr const char* const SET_CLASS_TO = "SetClassTo";
/**
 * @trace_description: Set static class attribute on a node.
 */
inline constexpr const char* const SET_STATIC_CLASS_TO = "SetStaticClassTo";
/**
 * @trace_description: Set the id attribute on a node.
 */
inline constexpr const char* const SET_ID = "SetId";
/**
 * @trace_description: Query component metadata from the current context.
 */
inline constexpr const char* const GET_COMPONENT_INFO = "GetComponentInfo";
/**
 * @trace_description: Create a slot node/function used by template component
 * composition.
 */
inline constexpr const char* const CREATE_SLOT = "CreateSlot";
/**
 * @trace_description: Set a property on a node/component instance.
 */
inline constexpr const char* const SET_PROP = "SetProp";
/**
 * @trace_description: Set data payload on a node/component instance.
 */
inline constexpr const char* const SET_DATA = "SetData";
/**
 * @trace_description: Append a virtual plug node to a component's virtual tree.
 */
inline constexpr const char* const APPEND_VIRTUAL_PLUG_TO_COMPONENT =
    "AppendVirtualPlugToComponent";
/**
 * @trace_description: Add a virtual plug node to a component (alias of append
 * virtual plug operation).
 */
inline constexpr const char* const ADD_VIRTUAL_PLUG_TO_COMPONENT =
    "AppendVirtualPlugToComponent";
/**
 * @trace_description: Add fallback content to a dynamic component when target
 * component cannot be resolved.
 */
inline constexpr const char* const ADD_FALLBACK_TO_DYNAMIC_COMPONENT =
    "AddFallbackToDynamicComponent";
/**
 * @trace_description: Query component's data payload from the current context.
 */
inline constexpr const char* const GET_COMPONENT_DATA = "GetComponentData";
/**
 * @trace_description: Query component's props from the current context.
 */
inline constexpr const char* const GET_COMPONENT_PROPS = "GetComponentProps";
/**
 * @trace_description: Query component's context data from the current context.
 */
inline constexpr const char* const GET_COMPONENT_CONTEXT_DATA =
    "GetComponentContextData";
/**
 * @trace_description: Create a component instance by its registered name.
 */
inline constexpr const char* const CREATE_COMPONENT_BY_NAME =
    "CreateComponentByName";
/**
 * @trace_description: Create a virtual component node for a dynamically
 * resolved component.
 */
inline constexpr const char* const CREATE_DYNAMIC_VIRTUAL_COMPONENT =
    "CreateDynamicVirtualComponent";
/**
 * @trace_description: Process component data (normalize/convert) before
 * applying it to the render pipeline.
 */
inline constexpr const char* const PROCESS_COMPONENT_DATA =
    "ProcessComponentData";
/**
 * @trace_description: Enter the LazyBundle render path which defers template
 * loading/execution until needed.
 */
inline constexpr const char* const LAZY_BUNDLE_RENDER_ENTRANCE =
    "LazyBundle::RenderEntrance";
/**
 * @trace_description: Register a data processor used to transform data before
 * it is applied to template/component updates.
 */
inline constexpr const char* const REGISTER_DATA_PROCESSOR =
    "RegisterDataProcessor";
/**
 * @trace_description: Add an event listener binding to the runtime/component.
 */
inline constexpr const char* const ADD_EVENT_LISTENER = "AddEventListener";
/**
 * @trace_description: Re-flush the page render pipeline (force refresh).
 */
inline constexpr const char* const RE_FLUSH_PAGE = "ReFlushPage";
/**
 * @trace_description: Set the component instance into the current render
 * context (component binding for subsequent operations).
 */
inline constexpr const char* const SET_COMPONENT = "SetComponent";
/**
 * @trace_description: Register an element-level worklet for executing isolated
 * logic associated with an element.
 */
inline constexpr const char* const REGISTER_ELEMENT_WORKLET =
    "RegisterElementWorklet";
/**
 * @trace_description: Create a virtual list node used by list components.
 */
inline constexpr const char* const CREATE_VIRTUAL_LIST_NODE =
    "CreateVirtualListNode";
/**
 * @trace_description: Send a global event from native/template runtime into
 * the JS framework.
 */
inline constexpr const char* const SEND_GLOBAL_EVENT = "SendGlobalEvent";
/**
 * @trace_description: Create a Fiber element node via FiberElement API.
 */
inline constexpr const char* const FIBER_CREATE_ELEMENT = "FiberCreateElement";
/**
 * @trace_description: Create a Fiber page root node via FiberElement API.
 */
inline constexpr const char* const FIBER_CREATE_PAGE = "FiberCreatePage";
/**
 * @trace_description: Get the page root element from FiberElement API.
 */
inline constexpr const char* const FIBER_GET_PAGE_ELEMENT =
    "FiberGetPageElement";
/**
 * @trace_description: Create a Fiber component node.
 */
inline constexpr const char* const FIBER_CREATE_COMPONENT =
    "FiberCreateComponent";
/**
 * @trace_description: Create a Fiber view element.
 */
inline constexpr const char* const FIBER_CREATE_VIEW = "FiberCreateView";
/**
 * @trace_description: Create a Fiber list element.
 */
inline constexpr const char* const FIBER_CREATE_LIST = "FiberCreateList";
/**
 * @trace_description: Create a Fiber scroll-view element.
 */
inline constexpr const char* const FIBER_CREATE_SCROLL_VIEW =
    "FiberCreateScrollView";
/**
 * @trace_description: Create a Fiber text element.
 */
inline constexpr const char* const FIBER_CREATE_TEXT = "FiberCreateText";
/**
 * @trace_description: Create a Fiber image element.
 */
inline constexpr const char* const FIBER_CREATE_IMAGE = "FiberCreateImage";
/**
 * @trace_description: Create a Fiber raw-text element.
 */
inline constexpr const char* const FIBER_CREATE_RAW_TEXT = "FiberCreateRawText";
/**
 * @trace_description: Create a conditional (if) Fiber node.
 */
inline constexpr const char* const FIBER_CREATE_IF = "FiberCreateIf";
/**
 * @trace_description: Create a loop (for) Fiber node.
 */
inline constexpr const char* const FIBER_CREATE_FOR = "FiberCreateFor";
/**
 * @trace_description: Create a block/container Fiber node.
 */
inline constexpr const char* const FIBER_CREATE_BLOCK = "FiberCreateBlock";
/**
 * @trace_description: Add config to a Fiber node or page context.
 */
inline constexpr const char* const FIBER_ADD_CONFIG = "FiberAddConfig";
/**
 * @trace_description: Set/override config for a Fiber node or page context.
 */
inline constexpr const char* const FIBER_SET_CONFIG = "FiberSetConfig";
/**
 * @trace_description: Create a non-element Fiber node (placeholder).
 */
inline constexpr const char* const FIBER_CREATE_NO_ELEMENT =
    "FiberCreateNonElement";
/**
 * @trace_description: Append a Fiber child element to a parent element.
 */
inline constexpr const char* const FIBER_APPEND_ELEMENT = "FiberAppendElement";
/**
 * @trace_description: Remove a Fiber element from its parent.
 */
inline constexpr const char* const FIBER_REMOVE_ELEMENT = "FiberRemoveElement";
/**
 * @trace_description: Insert a Fiber element before another child element.
 */
inline constexpr const char* const FIBER_INSERT_ELEMENT_BEFORE =
    "FiberInsertElementBefore";
/**
 * @trace_description: Get the first child element of a Fiber element.
 */
inline constexpr const char* const FIBER_FIRST_ELEMENT = "FiberFirstElement";
/**
 * @trace_description: Get the last child element of a Fiber element.
 */
inline constexpr const char* const FIBER_LAST_ELEMENT = "FiberLastElement";
/**
 * @trace_description: Get the next sibling element of a Fiber element.
 */
inline constexpr const char* const FIBER_NEXT_ELEMENT = "FiberNextElement";
/**
 * @trace_description: Resolve a Fiber element asynchronously (e.g., lazy or
 * deferred resolution).
 */
inline constexpr const char* const FIBER_ASYNC_RESOLVE_ELEMENT =
    "FiberAsyncResolveElement";
/**
 * @trace_description: Mark the root as an async-resolve root for Fiber update
 * propagation.
 */
inline constexpr const char* const FIBER_MARK_ASYNC_RESOLVE_ROOT =
    "FiberMarkAsyncResolveRoot";
/**
 * @trace_description: Asynchronously resolve subtree properties for a Fiber
 * node.
 */
inline constexpr const char* const FIBER_ASYNC_RESOLVE_SUBTREE_PROPERTY =
    "FiberAsyncResolveSubtreeProperty";
/**
 * @trace_description: Replace one Fiber element with another element.
 */
inline constexpr const char* const FIBER_REPLACE_ELEMENT =
    "FiberReplaceElement";
/**
 * @trace_description: Replace multiple Fiber elements as a batch operation.
 */
inline constexpr const char* const FIBER_REPLACE_ELEMENTS =
    "FiberReplaceElements";
/**
 * @trace_description: Swap two Fiber elements in the tree.
 */
inline constexpr const char* const FIBER_SWAP_ELEMENT = "FiberSwapElement";
/**
 * @trace_description: Get the parent element of a Fiber element.
 */
inline constexpr const char* const FIBER_GET_PARENT = "FiberGetParent";
/**
 * @trace_description: Get the children list of a Fiber element.
 */
inline constexpr const char* const FIBER_GET_CHILDREN = "FiberGetChildren";
/**
 * @trace_description: Check whether the element is a template element.
 */
inline constexpr const char* const FIBER_IS_TEMPLATE_ELEMENT =
    "FiberIsTemplateElement";
/**
 * @trace_description: Check whether the element is a template-part element.
 */
inline constexpr const char* const FIBER_IS_PART_ELEMENT = "FiberIsPartElement";
/**
 * @trace_description: Mark a Fiber element as a template element for template
 * operations.
 */
inline constexpr const char* const FIBER_MARK_TEMPLATE_ELEMENT =
    "FiberMarkTemplateElement";
/**
 * @trace_description: Mark a Fiber element as a part element for template-part
 * operations.
 */
inline constexpr const char* const FIBER_MARK_PART_ELEMENT =
    "FiberMarkPartElement";
/**
 * @trace_description: Get template parts associated with a template element.
 */
inline constexpr const char* const FIBER_GET_TEMPLATE_PARTS =
    "FiberGetTemplateParts";
/**
 * @trace_description: Convert a JSON value into a Lepus value for template
 * runtime processing.
 */
inline constexpr const char* const JSON_VALUE_TO_LEPUS_VALUE =
    "jsonValueTolepusValue";
/**
 * @trace_description: Compare two Fiber elements for equality (structure/id).
 */
inline constexpr const char* const FIBER_ELEMENT_IS_EQUAL =
    "FiberElementIsEqual";
/**
 * @trace_description: Get the unique id associated with a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ELEMENT_UNIQUE_ID =
    "FiberGetElementUniqueID";
/**
 * @trace_description: Get the tag name of a Fiber element.
 */
inline constexpr const char* const FIBER_GET_TAG = "FiberGetTag";
/**
 * @trace_description: Set an attribute on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_ATTRIBUTE = "FiberSetAttribute";
/**
 * @trace_description: Get an attribute value by name from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ATTRIBUTE_BY_NAME =
    "FiberGetAttributeByName";
/**
 * @trace_description: Get attribute names from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ATTRIBUTE_NAMES =
    "FiberGetAttributeNames";
/**
 * @trace_description: Get all attributes from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ATTRIBUTES = "FiberGetAttributes";
/**
 * @trace_description: Add a class to a Fiber element.
 */
inline constexpr const char* const FIBER_ADD_CLASS = "FiberAddClass";
/**
 * @trace_description: Set the class list on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_CLASSES = "FiberSetClasses";
/**
 * @trace_description: Get the class list from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_CLASSES = "FiberGetClasses";
/**
 * @trace_description: Add an inline style entry to a Fiber element.
 */
inline constexpr const char* const FIBER_ADD_INLINE_STYLE =
    "FiberAddInlineStyle";
/**
 * @trace_description: Set inline styles on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_INLINE_STYLES =
    "FiberSetInlineStyles";
/**
 * @trace_description: Get inline styles from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_INLINE_STYLES =
    "FiberGetInlineStyles";
/**
 * @trace_description: Set parsed styles on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_PARSED_STYLES =
    "FiberSetParsedStyles";
/**
 * @trace_description: Add an event binding to a Fiber element.
 */
inline constexpr const char* const FIBER_ADD_EVENT = "FiberAddEvent";
/**
 * @trace_description: Create a gesture detector instance for a Fiber element.
 */
inline constexpr const char* const CREATE_GESTURE_DETECTOR =
    "CreateGestureDetector";
/**
 * @trace_description: Attach a gesture detector to a Fiber element.
 */
inline constexpr const char* const FIBER_SET_GESTURE_DETECTOR =
    "FiberSetGestureDetector";
/**
 * @trace_description: Remove the gesture detector from a Fiber element.
 */
inline constexpr const char* const FIBER_REMOVE_GESTURE_DETECTOR =
    "FiberRemoveGestureDetector";
/**
 * @trace_description: Update the gesture state associated with a Fiber element.
 */
inline constexpr const char* const FIBER_SET_GESTURE_STATE =
    "FiberSetGestureState";
/**
 * @trace_description: Consume a gesture event and prevent further propagation.
 */
inline constexpr const char* const FIBER_CONSUME_GESTURE =
    "FiberConsumeGesture";
/**
 * @trace_description: Set event bindings on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_EVENTS = "FiberSetEvents";
/**
 * @trace_description: Get an event binding from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_EVENT = "FiberGetEvent";
/**
 * @trace_description: Get all event bindings from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_EVENTS = "FiberGetEvents";
/**
 * @trace_description: Set the id on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_ID = "FiberSetID";
/**
 * @trace_description: Get the id from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ID = "FiberGetID";
/**
 * @trace_description: Add a dataset entry to a Fiber element.
 */
inline constexpr const char* const FIBER_ADD_DATA_SET = "FiberAddDataset";
/**
 * @trace_description: Set dataset on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_DATA_SET = "FiberSetDataset";
/**
 * @trace_description: Get dataset from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_DATA_SET = "FiberGetDataset";
/**
 * @trace_description: Get a data value by key from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_DATA_BY_KEY = "FiberGetDataByKey";
/**
 * @trace_description: Get the component id associated with a Fiber element.
 */
inline constexpr const char* const FIBER_GET_COMPONENT_ID =
    "FiberGetComponentID";
/**
 * @trace_description: Update the component id associated with a Fiber element.
 */
inline constexpr const char* const FIBER_UPDATE_COMPONENT_ID =
    "FiberUpdateComponentID";
/**
 * @trace_description: Update list-related callbacks (e.g., item render/scroll)
 * bound to a Fiber list element.
 */
inline constexpr const char* const FIBER_UPDATE_LIST_CALLBACKS =
    "FiberUpdateListCallbacks";
/**
 * @trace_description: Set css id on a Fiber element.
 */
inline constexpr const char* const FIBER_SET_CSS_ID = "FiberSetCSSId";
/**
 * @trace_description: Flush element tree updates to the native view layer.
 */
inline constexpr const char* const FIBER_FLUSH_ELEMENT_TREE =
    "FiberFlushElementTree";
/**
 * @trace_description: Mark the end of flushing element tree updates.
 */
inline constexpr const char* const FIBER_FLUSH_ELEMENT_TREE_END =
    "FiberFlushElementTreeEnd";
/**
 * @trace_description: Build Fiber element tree from a binary/template payload.
 */
inline constexpr const char* const FIBER_ELEMENT_FROM_BINARY =
    "FiberElementFromBinary";
/**
 * @trace_description: Query a component via Fiber APIs.
 */
inline constexpr const char* const FIBER_QUERY_COMPONENT =
    "FiberQueryComponent";
/**
 * @trace_description: Query an element by selector via Fiber APIs.
 */
inline constexpr const char* const FIBER_QUERY_SELECTOR = "FiberQuerySelector";
/**
 * @trace_description: Update component metadata for a Fiber component element.
 */
inline constexpr const char* const FIBER_UPDATE_COMPONENT_INFO =
    "FiberUpdateComponentInfo";
/**
 * @trace_description: Get element config from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_ELEMENT_CONFIG =
    "FiberGetElementConfig";
/**
 * @trace_description: Get an inline style entry from a Fiber element.
 */
inline constexpr const char* const FIBER_GET_INLINE_STYLE =
    "FiberGetInlineStyle";
/**
 * @trace_description: Query all elements matching selector via Fiber APIs.
 */
inline constexpr const char* const FIBER_QUERY_SELECTOR_ALL =
    "FiberQuerySelectorAll";
/**
 * @trace_description: Set initial Lepus data for Fiber/template runtime.
 */
inline constexpr const char* const FIBER_SET_LEPUS_INIT_DATA =
    "FiberSetLepusInitData";
/**
 * @trace_description: Get diff payload/data calculated for Fiber updates.
 */
inline constexpr const char* const FIBER_GET_DIFF_DATA = "FiberGetDiffData";
/**
 * @trace_description: Get Fiber element by its unique id.
 */
inline constexpr const char* const FIBER_GET_ELEMENT_BY_UNIQUE_ID =
    "FiberGetElementByUniqueID";
/**
 * @trace_description: Update the index for a conditional (if) Fiber node.
 */
inline constexpr const char* const FIBER_UPDATE_IF_NODE_INDEX =
    "FiberUpdateIfNodeIndex";
/**
 * @trace_description: Update the child count for a loop (for) Fiber node.
 */
inline constexpr const char* const FIBER_UPDATE_FOR_CHILD_COUNT =
    "FiberUpdateForChildCount";
/**
 * @trace_description: Load a Lepus chunk (partial template/script payload).
 */
inline constexpr const char* const LOAD_LEPUS_CHUNK = "LoadLepusChunk";
/**
 * @trace_description: Create a frame element in Fiber (layout/frame container).
 */
inline constexpr const char* const FIBER_CREATE_FRAME = "FiberCreateFrame";
/**
 * @trace_description: Create a Fiber element with initial props applied.
 */
inline constexpr const char* const FIBER_CREATE_ELEMENT_WITH_PROPS =
    "FiberCreateElementWithProperties";
/**
 * @trace_description: Create a reactive signal in Fiber runtime.
 */
inline constexpr const char* const FIBER_CREATE_SIGNAL = "FiberCreateSignal";
/**
 * @trace_description: Write/update a reactive signal value in Fiber runtime.
 */
inline constexpr const char* const FIBER_WRITE_SIGNAL = "FiberWriteSignal";
/**
 * @trace_description: Read a reactive signal value in Fiber runtime.
 */
inline constexpr const char* const FIBER_READ_SIGNAL = "FiberReadSignal";
/**
 * @trace_description: Create a computation node in Fiber reactivity system.
 */
inline constexpr const char* const FIBER_CREATE_COMPUTATION =
    "FiberCreateComputation";
/**
 * @trace_description: Create a memoized computation in Fiber reactivity system.
 */
inline constexpr const char* const FIBER_CREATE_MEMO = "FiberCreateMemo";
/**
 * @trace_description: Temporarily disable dependency tracking in Fiber.
 */
inline constexpr const char* const FIBER_UN_TRACK = "FiberUnTrack";
/**
 * @trace_description: Run pending reactive updates in Fiber runtime.
 */
inline constexpr const char* const FIBER_RUN_UPDATES = "FiberRunUpdates";
/**
 * @trace_description: Create a reactive scope in Fiber runtime.
 */
inline constexpr const char* const FIBER_CREATE_SCOPE = "FiberCreateScope";
/**
 * @trace_description: Get current reactive scope in Fiber runtime.
 */
inline constexpr const char* const FIBER_GET_SCOPE = "FiberGetScope";
/**
 * @trace_description: Clean up reactive resources/subscriptions for a scope.
 */
inline constexpr const char* const FIBER_CLEAN_UP = "FiberCleanUp";
/**
 * @trace_description: Register clean-up callback for a reactive scope.
 */
inline constexpr const char* const FIBER_ON_CLEAN_UP = "FiberOnCleanUp";
/**
 * @trace_description: Get computed style value by key from Fiber element.
 */
inline constexpr const char* const FIBER_GET_COMPUTED_STYLE_BY_KEY =
    "FiberGetComputedStyleByKey";
/**
 * @trace_description: Flush enqueued tasks in element context delegate.
 */
inline constexpr const char* const
    ELEMENT_CONTEXT_DELEGATE_FLUSH_ENQUEUED_TASKS =
        "ElementContextDelegate::FlushEnqueuedTasks";
/**
 * @trace_description: Enqueue a task into element context delegate for later
 * execution.
 */
inline constexpr const char* const ELEMENT_CONTEXT_DELEGATE_ENQUEUE_TASK =
    "ElementContextDelegate::EnqueueTask";
/**
 * @trace_description: Record source map release information for debugging.
 */
inline constexpr const char* const SET_SOURCE_MAP_RELEASE =
    "SetSourceMapRelease";
/**
 * @trace_description: Report an error from runtime to native error pipeline.
 */
inline constexpr const char* const REPORT_ERROR = "ReportError";
/**
 * @trace_description: Add custom info into reporter context for diagnostics.
 */
inline constexpr const char* const LYNX_ADD_REPORTER_CUSTOM_INFO =
    "lynx.AddReporterCustomInfo";

/**
 * @trace_description: Generate pipeline options used by the render/template
 * pipeline.
 */
inline constexpr const char* const GENERATE_PIPELINE_OPTIONS =
    "GeneratePipelineOptions";
/**
 * @trace_description: Mark pipeline start for a render/template pipeline.
 */
inline constexpr const char* const ON_PIPELINE_START = "OnPipelineStart";
/**
 * @trace_description: Bind pipeline id with timing flag for timing correlation.
 */
inline constexpr const char* const BIND_PIPELINE_ID_WITH_TIMING_FLAG =
    "BindPipelineIDWithTimingFlag";
/**
 * @trace_description: Mark a timing point used by timing collector/reporting.
 */
inline constexpr const char* const MARK_TIMING = "MarkTiming";
/**
 * @trace_description: Clear a scheduled timeout task.
 */
inline constexpr const char* const CLEAR_TIMEOUT = "ClearTimeout";
/**
 * @trace_description: Clear a scheduled interval task.
 */
inline constexpr const char* const CLEAR_TIME_INTERVAL = "ClearTimeInterval";
/**
 * @trace_description: Request an animation frame callback from runtime.
 */
inline constexpr const char* const REQUEST_ANIMATION_FRAME =
    "RequestAnimationFrame";
/**
 * @trace_description: Cancel a previously requested animation frame callback.
 */
inline constexpr const char* const CANCEL_ANIMATION_FRAME =
    "CancelAnimationFrame";

/**
 * @trace_description: Convert rapidjson value into Lepus value.
 */
inline constexpr const char* const RAPID_JSON_VALUE_TO_LEPUS_VALUE =
    "rapidJsonValueTolepusValue";
/**
 * @trace_description: Convert QuickJS array value into JSON string.
 */
inline constexpr const char* const QUICKJS_ARRAY_TO_JSON_STRING =
    "qjsArrayToJSONString";
/**
 * @trace_description: Convert QuickJS object value into JSON string.
 */
inline constexpr const char* const QUICKJS_OBJECT_TO_JSON_STRING =
    "qjsObjectToJSONString";
/**
 * @trace_description: Convert a QuickJS value (any type) into a JSON string.
 */
inline constexpr const char* const QUICKJS_VALUE_TO_JSON_STRING =
    "qjsValueToJSONString";
/**
 * @trace_description: Convert a Lepus value into a JSON string representation.
 */
inline constexpr const char* const LEPUS_VALUE_TO_JSON_STRING =
    "lepusValueToJSONString";
/**
 * @trace_description: Convert a Lepus map value into a JSON string
 * representation.
 */
inline constexpr const char* const LEPUS_VALUE_MAP_TO_JSON_STRING =
    "lepusValueMapToJSONString";
/**
 * @trace_description: Convert a Lepus value into a string representation.
 */
inline constexpr const char* const LEPUS_VALUE_TO_STRING = "lepusValueToString";

/**
 * @trace_description: Set function file name metadata for debugging and
 * stacktrace mapping in QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_SET_FUNC_FILE_NAME =
    "SetFunctionFileName";
/**
 * @trace_description: Execute LepusNG bytecode/script entrypoint in
 * QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_EXECUTE = "LepusNG.Execute";
/**
 * @trace_description: Deserialize LepusNG state from serialized data.
 */
inline constexpr const char* const QUICK_CONTEXT_DO_SERIALIZE =
    "LepusNG.DeSerialize";
/**
 * @trace_description: Evaluate LepusNG binary payload in QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_EVAL_BINARY =
    "LepusNG.EvalBinary";
/**
 * @trace_description: Evaluate LepusNG script payload in QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_EVAL_SCRIPT =
    "LepusNG.EvalScript";
/**
 * @trace_description: Check whether table shadow is updated with top-level
 * variables in QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_CHECK_TABLE_SHADOW_UPDATED =
    "QuickContext::CheckTableShadowUpdatedWithTopLevelVariable";
/**
 * @trace_description: Update top-level variables in QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_UPDATE_TOP_LEVEL_VARIABLE =
    "QuickContext::UpdateTopLevelVariable";
/**
 * @trace_description: Apply runtime/config changes to QuickContext.
 */
inline constexpr const char* const QUICK_CONTEXT_APPLY_CONFIG =
    "QuickContext::ApplyConfig";
/**
 * @trace_description: Initialize VMContext for Lepus execution.
 */
inline constexpr const char* const VM_CONTEXT_INIT = "VMContext::Initialize";
/**
 * @trace_description: Execute Lepus script in VMContext.
 */
inline constexpr const char* const VM_CONTEXT_EXECUTE = "Lepus.Execute";
/**
 * @trace_description: Call a function in VMContext.
 */
inline constexpr const char* const VM_CONTEXT_CALL = "VMContext::Call";
/**
 * @trace_description: Call a closure in VMContext.
 */
inline constexpr const char* const VM_CONTEXT_CALL_CLOSURE =
    "VMContext::CallClosure";
/**
 * @trace_description: Check whether table shadow is updated with top-level
 * variables in VMContext.
 */
inline constexpr const char* const VM_CONTEXT_CHECK_TABLE_SHADOW_UPDATED =
    "VMContext::CheckTableShadowUpdatedWithTopLevelVariable";
/**
 * @trace_description: Call a C function from VMContext.
 */
inline constexpr const char* const VM_CONTEXT_CALL_C_FUNCTION =
    "VMContext::CallCFunction";
/**
 * @trace_description: Clean up closures created after execution in VMContext.
 */
inline constexpr const char* const VM_CONTEXT_CLEAN_CLOSURES_AFTER_EXECUTED =
    "CleanUpClosuresCreatedAfterExecuted";
/**
 * @trace_description: Construct a VMContext instance.
 */
inline constexpr const char* const VM_CONTEXT_CONSTRUCTION =
    "VMContext::VMContext";
/**
 * @trace_description: Execute one animation frame task item.
 */
inline constexpr const char* const ANIMATION_FRAME_TASK_EXECUTE =
    "AnimationFrameTaskHandler::FrameTask::Execute";
/**
 * @trace_description: Request an animation frame via animation frame task
 * handler.
 */
inline constexpr const char* const
    ANIMATION_FRAME_TASK_REQUEST_ANIMATION_FRAME =
        "AnimationFrameTaskHandler::RequestAnimationFrame";
/**
 * @trace_description: Cancel an animation frame via animation frame task
 * handler.
 */
inline constexpr const char* const ANIMATION_FRAME_TASK_CANCEL_ANIMATION_FRAME =
    "AnimationFrameTaskHandler::CancelAnimationFrame";
/**
 * @trace_description: Execute do-frame logic in animation frame task handler.
 */
inline constexpr const char* const ANIMATION_FRAME_TASK_DO_FRAME =
    "AnimationFrameTaskHandler::DoFrame";
/**
 * @trace_description: Convert a closure event payload into Piper (JSI) values
 * before dispatching to JS.
 */
inline constexpr const char* const
    CLOSURE_EVENT_LISTENER_CONVERT_TO_PIPER_VALUE =
        "JSClosureEventListener::ConvertEventToPiperValue";
/**
 * @trace_description: Invoke the NativeModule method with module name
 * @args{module}, method name @args(method} and first_args @args{first_arg}.
 * @history_name{InvokeNativeModule}
 */
inline constexpr const char* const NATIVE_MODULE_INVOKE =
    "NativeModule::Invoke";
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
 * @trace_description: Parse, and execute the script.
 */
inline constexpr const char* const APP_PREPARE_ANB_EVAL_SCRIPT =
    "App::prepareAndEvalScript";
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

/**
 * @trace_description: Get context data asynchronously from native and return
 * it to JS.
 */
inline constexpr const char* const APP_GET_CONTEXT_DATA_ASYNC =
    "App::getContextDataAsync";

/**
 * @trace_description: Read a session storage item by key from native storage.
 */
inline constexpr const char* const APP_GET_SESSION_STORAGE_ITEM =
    "App::getSessionStorageItem";
/**
 * @trace_description: Subscribe to changes of a session storage item so JS can
 * be notified when it updates.
 */
inline constexpr const char* const APP_SUBSCRIBE_SESSION_STORAGE =
    "App::subscribeSessionStorage";

/**
 * @trace_description: Trigger a Lepus bridge call (sync bridge entrypoint).
 */
inline constexpr const char* const TRIGGER_LEPUS_BRIDGE = "TriggerLepusBridge";
/**
 * @trace_description: Trigger a Lepus bridge call using async bridge path.
 */
inline constexpr const char* const TRIGGER_LEPUS_BRIDGE_ASYNC =
    "TriggerLepusBridgeSync";
/**
 * @trace_description: Trigger a component event from runtime into component
 * event handling pipeline.
 */
inline constexpr const char* const TRIGGER_COMPONENT_EVENT =
    "TriggerComponentEvent";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_RUNTIME_TRACE_RUNTIME_TRACE_EVENT_DEF_H_
