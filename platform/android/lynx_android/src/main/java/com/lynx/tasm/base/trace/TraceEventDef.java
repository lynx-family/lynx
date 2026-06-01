// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base.trace;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class TraceEventDef {
  /**
   * @trace_description: LynxView's onMeasure function, it is triggered by the Android system.
   */
  public static final String LYNX_VIEW_ON_MEASURE = "LynxView.onMeasure";

  /**
   * @trace_description: LynxView's onLayout function, it is triggered by the Android system.
   */
  public static final String LYNX_VIEW_ON_LAYOUT = "LynxView.onLayout";

  /**
   * @trace_description: LynxView's onDraw function, it is triggered by the Android system.
   */
  public static final String LYNX_VIEW_ON_DRAW = "LynxView.onDraw";

  /**
   * @trace_description: LynxView's onAttachedToWindow function, it is triggered by the Android
   * system.
   */
  public static final String LYNX_VIEW_ON_ATTACH_TO_WINDOW = "LynxView.onAttachedToWindow";

  /**
   * @trace_description: LynxView's onDetachedFromWindow function, it is triggered by the Android
   * system.
   */
  public static final String LYNX_VIEW_ON_DETACHED_FROM_WINDOW = "LynxView.onDetachedFromWindow";

  /**
   * @trace_description: Update LynxView's viewport.
   */
  public static final String LYNX_VIEW_UPDATE_VIEWPORT = "LynxView.updateViewport";

  /**
   * @trace_description: Layout of <text> element's platform layout node, where the preview text
   * are `@args{preview_text}`.
   * @history_name{text.TextShadowNode.measure}
   */
  public static final String TEXT_SHADOW_NODE_MEASURE = "TextShadowNode.measure";
  public static final String TEXT_LAYOUT_MEASURE_TEXT = "TextLayout.measureText";
  public static final String TEXT_LAYOUT_DISPATCH_LAYOUT_BEFORE = "TextLayout.dispatchLayoutBefore";
  public static final String TEXT_LAYOUT_ALIGN = "TextLayout.align";

  /**
   * @trace_description: Redirect the image URL.
   * @link{https://lynxjs.org/api/lynx-native-api/resource-fetcher/mediaresourcefetcher#shouldredirecturl}
   */
  public static final String IMAGE_SHOULD_REDIRECT_IMAGE_URL = "Interceptor.shouldRedirectImageUrl";

  public static final String UI_BODY_ATTACH_UI_BODY_VIEW = "UIBody.attachUIBodyView";
  public static final String UI_BODY_DETACH_UI_BODY_VIEW = "UIBody.detachUIBodyView";
  public static final String UI_BODY_REBUILD_VIEW_TREE = "UIBody.rebuildViewTree";
  public static final String UI_BODY_SET_MEASURED_DIMENSION = "UIBody.innerSetMeasuredDimension";

  public static final String DEVTOOL_INIT = "LynxDevtool initialized";

  public static final String CANCEL_RESOURCE_PREFETCH = "cancelResourcePrefetch";
  public static final String REQUEST_RESOURCE_PREFETCH = "requestResourcePrefetch";
  public static final String ENGINE_BUILDER_BUILD = "LynxEngineBuilder.build";

  public static final String TEMPLATE_RENDER_INIT = "TemplateRender.init";
  public static final String TEMPLATE_RENDER_INIT_PIPER = "TemplateRender.initPiper";
  public static final String TEMPLATE_RENDER_START_LOAD = "StartLoad";
  public static final String FIRST_MEANINGFUL_PAINT = "FirstMeaningfulPaint";
  public static final String TEMPLATE_RENDER_INIT_WITH_CONTEXT = "TemplateRender.initWithContext";
  public static final String TEMPLATE_RENDER_CREATE_TASM = "TemplateRender.createLynxEngine";
  public static final String TEMPLATE_RENDER_DISPATCH_ERROR = "TemplateRender.dispatchError";
  public static final String TEMPLATE_RENDER_SET_GLOBAL_PROPS = "TemplateRender.setGlobalProps";
  public static final String TEMPLATE_RENDER_ON_RUN_PIPELINE_FINISHED =
      "TemplateRender.onRunPipelineFinished";
  public static final String TEMPLATE_RENDER_DETACH_LYNX_ENGINE =
      "TemplateRender.detachLynxEngineWrapper";
  public static final String TEMPLATE_RENDER_UPDATE_META_DATE = "LynxTemplateRender.updateMetaData";
  public static final String TEMPLATE_RENDER_PROCESS_RENDER = "TemplateRender.processRender";
  public static final String TEMPLATE_RENDER_ATTACH_LYNX_VIEW = "TemplateRender.attachLynxView";
  public static final String TEMPLATE_RENDER_RELOAD_AND_INIT = "TemplateRender.reloadAndInit";
  public static final String TEMPLATE_RENDER_TRY_REUSE_ENGINE =
      "TemplateRender.tryReuseLynxEngineFromPool";
  public static final String TEMPLATE_RENDER_RENDER_TEMPLATE_BUNDLE =
      "TemplateRender.renderTemplateBundle";
  public static final String TEMPLATE_RENDER_FALLBACK_NEW_ENGINE =
      "TemplateRender.fallbackNewEngine";
  public static final String CLIENT_REPORT_COMPONENT_INFO = "Client.onReportComponentInfo";
  public static final String CLIENT_ON_PAGE_START = "Client.onPageStart";
  public static final String CLIENT_ON_LOAD_SUCCESS = "Client.onLoadSuccess";
  public static final String CLIENT_ON_PAGE_UPDATE = "Client.onPageUpdate";
  public static final String CLIENT_ON_UPDATE_WITHOUT_CHANGE = "Client.onUpdateDataWithoutChange";
  public static final String CLIENT_ON_TASM_FINISHED_BY_NATIVE = "Client.onTASMFinishedByNative";
  public static final String CLIENT_ON_RUNTIME_READY = "Client.onRuntimeReady";
  public static final String CLIENT_ON_DATA_UPDATED = "Client.onDataUpdated";
  public static final String CLIENT_ON_REPORT_COMPONENT = "Client.onReportComponentInfo";
  public static final String CLIENT_ON_DYNAMIC_COMPONENT_PERF = "Client.onDynamicComponentPerf";
  public static final String CLIENT_ON_MODULE_FUNCTION = "Client.onModuleMethodInvoked";
  public static final String CLIENT_ON_TEMPLATE_BUNDLE_READY = "Client.onTemplateBundleReady";

  public static final String LYNX_ENGINE_POOL_REGISTER_ENGINE =
      "LynxEnginePool.registerReuseEngineWrapper";
  public static final String LYNX_ENGINE_POOL_POOL_ENGINE = "LynxEnginePool.pollEngineFromPool";

  public static final String DESTORY_LYNXVIEW = "DestroyLynxView";
  public static final String LYNXVIEW_BUILDER_BUILD = "CreateLynxView";
  /**
   * @trace_description: Dispatch memory pressure signal with level @args{pressure}.
   */
  public static final String LYNX_ENV_TRIM_MEMORY = "LynxEnv.trimMemory";
  /**
   * @trace_description: Schedule one global Lynx memory usage query.
   */
  public static final String MEMORY_USAGE_GLOBAL_QUERY = "GlobalMemoryUsage.query";
  /**
   * @trace_description: Start or coalesce a global Lynx memory usage query on the report thread.
   */
  public static final String MEMORY_USAGE_GLOBAL_REPORT_THREAD_QUERY =
      "GlobalMemoryUsage.reportThreadQuery";
  /**
   * @trace_description: Fan out a global Lynx memory usage query to registered instance fetchers.
   */
  public static final String MEMORY_USAGE_GLOBAL_FETCHER_FAN_OUT =
      "GlobalMemoryUsage.fetcherFanOut";
  /**
   * @trace_description: Build and deliver the final global Lynx memory usage result.
   */
  public static final String MEMORY_USAGE_GLOBAL_FINISH = "GlobalMemoryUsage.finish";
  /**
   * @trace_description: Record total duration and counts for one global Lynx memory usage query.
   */
  public static final String MEMORY_USAGE_GLOBAL_TOTAL = "GlobalMemoryUsage.total";
  /**
   * @trace_description: Query memory usage for one TemplateRender instance.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_QUERY = "TemplateRenderMemoryUsage.query";
  /**
   * @trace_description: Dispatch native memory usage sampling for one TemplateRender instance.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_DISPATCH_NATIVE =
      "TemplateRenderMemoryUsage.dispatchNative";
  /**
   * @trace_description: Receive the native memory usage callback for one TemplateRender instance.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_CALLBACK =
      "TemplateRenderMemoryUsage.nativeCallback";
  /**
   * @trace_description: Complete native memory usage aggregation on the report thread.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_NATIVE_COMPLETE =
      "TemplateRenderMemoryUsage.nativeComplete";
  /**
   * @trace_description: Sample Android view memory for one TemplateRender instance on UI thread.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_VIEW_SAMPLE =
      "TemplateRenderMemoryUsage.viewSample";
  /**
   * @trace_description: Complete Android view memory aggregation on the report thread.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_VIEW_COMPLETE =
      "TemplateRenderMemoryUsage.viewComplete";
  /**
   * @trace_description: Build one TemplateRender instance memory usage result.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_FINISH =
      "TemplateRenderMemoryUsage.finish";
  /**
   * @trace_description: Record total duration and bytes for one TemplateRender memory usage query.
   */
  public static final String MEMORY_USAGE_TEMPLATE_RENDER_TOTAL = "TemplateRenderMemoryUsage.total";

  public static final String TRIGGER_EMBEDDED_MODE_LIFECYCLE = "TriggerEmbeddedModeLifecycle";

  public static final String LOGIC_EXECUTOR_INIT = "LogicExecutor.Init";
  public static final String LOGIC_EXECUTOR_EVENT = "LogicExecutor.OnLynxEvent";

  public static final String CLIENT_LYNXVIEW_AND_JSRUNTIME_DESTORY =
      "Client.onLynxViewAndJSRuntimeDestroy";
  public static final String CLIENT_PIPER_INVOKED = "Client.onPiperInvoked";
  public static final String CLIENT_DESTORY = "Client.onDestory";
  public static final String CLIENT_TIMING_SETUP = "Client.onTimingSetup";
  public static final String CLIENT_TIMING_UPDATE = "Client.onTimingUpdate";
  public static final String CLIENT_SCROLL_STOP = "Client.onScrollStop";
  public static final String CLIENT_SCROLL_START = "Client.onScrollStart";
  public static final String CLIENT_FLING = "Client.onFling";
  public static final String CLIENT_ON_FIRST_SCREEN = "Client.onFirstScreen";
  public static final String TEMPLATE_BUNDLE_FROM_TEMPLATE = "TemplateBundle.fromTemplate";
  public static final String TEMPLATE_BUNDLE_FROM_BYTEBUFFER =
      "TemplateBundle.fromTemplateWithByteBuffer";
  public static final String TEMPLATE_DATA_FROM_MAP = "TemplateData.FromMap";
  public static final String TEMPLATE_DATA_SHALLOW_CLONE = "TemplateData.ShallowClone";
  public static final String TEMPLATE_DATA_FROM_STRING = "TemplateData.FromString";
  public static final String CLEAN_REF_RUN_CLEAN_TASK_REAPER_THREAD =
      "CleanupReference.ReaperThread.runCleanupTask";
  public static final String CLEAN_REF_HANDLE_MESSAGE = "CleanupReference.LazyHolder.handleMessage";
  public static final String CLEAN_REF_RUN_CLEAN_TASK_INVOKE_THREAD =
      "CleanupReference.InvokingThread.runCleanupTask";
  public static final String LYNX_CONTEXT_UPDATE_SESSION_ID = "LynxContext.updateLynxSessionID";
  public static final String INTERSECTION_OBSERVER_MANAGER_INIT =
      "LynxIntersectionObserverManager initialized";
  public static final String OBSERVER_MANAGER_OBSERVER_HANDLER = "ObserverManager.ObserverHandler";
  public static final String UI_OWNER_INIT = "LynxUIOwner initialized";
  public static final String UI_OWNER_UPDATE_PROPS = "UIOwner.updateProps.";
  public static final String UI_OWNER_UPDATE_EXTRA_DATA = "UIOwner.updateViewExtraData.";
  public static final String UI_OWNER_UPDATE_LAYOUT = "UIOwner.updateLayout.";
  public static final String UI_OWNER_CREATE_VIEW = "UIOwner.createView.";
  public static final String UI_OWNER_CREATE_VIEW_ASYNC = "UIOwner.createViewAsync.";
  public static final String UI_OWNER_CREATE_VIEW_ASYNC_RUNNABLE =
      "UIOwner.createAsyncViewRunnable.";
  public static final String UI_OWNER_CREATE_VIEW_ASYNC_RUNNABLE_AFTER =
      "UIOwner.AfterCreateAsyncViewRunnable.";
  public static final String UI_OWNER_CREATE_VIEW_ASYNC_AFTER = "UIOwner.AfterCreateViewAsync.";
  public static final String UI_OWNER_UPDATE_FLATTEN = "UIOwner.updateFlatten.";
  public static final String UI_OWNER_REBUILD_VIEW_TREE = "UIOwner.rebuildViewTree";
  public static final String UI_OWNER_REMOVE = "UIOwner.remove.";
  public static final String UI_OWNER_DESTORY_ITEM = "UIOwner.destroy.item";
  public static final String UI_OWNER_DESTORY = "UIOwner.destroy";
  public static final String UI_OWNER_LAYOUT_FINISH = "UIOwner.layoutFinish.";
  public static final String UI_OWNER_INVOKE_UI_METHOD_FOR_SELECTOR_QUERY =
      "UIOwner.invokeUIMethodForSelectorQuery.";

  public static final String PAINTING_CONTEXT_REMOVE_LIST_ITEM =
      "PaintingContext.removeListItemNode.";
  public static final String PAINTING_CONTEXT_INSERT_LIST_ITEM =
      "PaintingContext.insertListItemNode.";

  public static final String TYPEFACE_CACHE_CATCH_FROM_FILE =
      "text.TypefaceCache.cacheTypefaceFromFile";

  public static final String FLATTEN_UI_DRAW = "LynxFlattenUI.draw.";
  public static final String LYNX_UI_MEASURE = "LynxUI.measure.";
  public static final String LYNX_UI_LAYOUT = "LynxUI.layout.";
  public static final String CREATE_BITMAP_SHADER = "createBitmapShader";
  public static final String IMAGE_SERVICE_PROXY_FETCH_IMAGE = "LynxImageServiceProxy.fetchImage";

  public static final String IMAGE_MANAGER_UPDATE_PROPS_INTERVAL =
      "LynxImageManager.updatePropertiesInterval";
  public static final String IMAGE_MANAGER_UPDATE_IMAGE_SOURCE =
      "LynxImageManager.updateImageSource";
  public static final String IMAGE_MANAGER_UPDATE_PLACEHOLDER_SOURCE =
      "LynxImageManager.updatePlaceholderSource";
  public static final String UI_LIST_MEASURE = "UIList.measure";
  public static final String UI_LIST_LAYOUT = "UIList.layout";

  public static final String LIST_CONTAINER_VIEW_DESTORY = "ListContainerView.destroy";

  public static final String FLATTEN_UI_TEXT_DRAW = "text.FlattenUIText.onDraw";

  public static final String LEPUS_BUFFER_ENCODE_MESSAGE = "LepusBuffer::EncodeMessage";
  public static final String LEPUS_BUFFER_DECODE_MESSAGE = "LepusBuffer::DecodeMessage";

  public static final String EVENT_REPORTER_ON_EVENT = "LynxEventReporter::OnEvent";
  public static final String EVENT_REPORTER_HANDLE_EVENT = "LynxEventReporter::handleEvent";
  public static final String EVENT_REPORTER_UPDATE_GENERIC_INFO =
      "LynxEventReporter::updateGenericInfo";
  public static final String EVENT_REPORTER_REMOVE_GENERIC_INFO =
      "LynxEventReporter::removeGenericInfo";

  public static final String FONT_FACE_MANAGER_LOAD_TYPEFACE_WITH_GENERIC_RESOURCE_FETCHER =
      "FontFaceManager.loadTypefaceWithGenericLynxResourceFetcher";
  public static final String FONT_FACE_MANAGER_LOAD_TYPEFACE = "FontFaceManager.loadTypeface";

  public static final String NINE_PATCH_HELPER_GET_MATRIX = "image.NinePatchHelper.getMatrix";
  public static final String NINE_PATCH_HELPER_DRAW_NINE_PATH =
      "image.NinePatchHelper.drawNinePatch";
  public static final String NINE_PATCH_HELPER_DRAW_WITH_CAP_INSETS =
      "image.NinePatchHelper.drawWithCapInsets";

  public static final String FETCHER_WRAPPER_USE_RESOURCE_SERVICE =
      "Using LynxResourceServiceProvider";
  public static final String FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER =
      "Using DynamicComponentFetcher";

  public static final String FLUENCY_TRACER_START = "StartFluencyTrace";
  public static final String FLUENCY_TRACER_STOP = "StopFluencyTrace";
  public static final String INSTANCE_ID = "instance_id";
  public static final String LYNX_VIEW = "lynx_view";
  public static final String EXCEPTION = "exception";
  public static final String WIDTH_MEASURE_SPEC = "widthMeasureSpec";
  public static final String HEIGHT_MEASURE_SPEC = "heightMeasureSpec";
  public static final String NEED_LAYOUT = "needLayout";
  public static final String PARAMS = "params";
  public static final String PIPELINE_IDS = "pipeline_ids";
  public static final String DUMP_UI_TREE_LAYOUT = "DumpUITreeLayout";
  public static final String VIEW_BLUR = "AndroidView:blur";

  // timing
  public static final String PIPELINE_ID = "pipeline_id";
  public static final String TIMING_KEY = "timing_key";
  public static final String TIMING_TIMESTAMP = "timestamp";
  public static final String TIMING_KEY_PAINT_END = "paintEnd";
  public static final String MARK_TIMING = "Timing::Mark";
  public static final String MARK_HOST_PLATFORM_TIMING = "Timing::MarkHostPlatformTiming";

  // fsp
  public static final String CATEGORY_DEFAULT = "lynx";
  public static final String FSP_SNAPSHOT_BASE_64_BITMAP = "FSPSnapshotBase64Bitmap";
  public static final String FSP_CONFIG_PARSE = "FSPConfig.parse";
  public static final String FSP_TRACER_ON_VALUABLE_SNAPSHOT = "FSPTracer::OnValuableSnapshot";
  public static final String FSP_TRACER_SNAPSHOT_STABLE = "FSPTracer::SnapshotStable";
  public static final String FSP_TIMING_MARK_FSP_END = "Timing::Mark.fspEnd";

  // frame
  /**
   * @trace_description: Destroy LynxFrameView.
   */
  public static final String DESTORY_LYNXFRAMEVIEW = "LynxFrameView.destroy";
  /**
   * @trace_description: Set LynxFrameView's init data.
   */
  public static final String LYNX_FRAME_VIEW_SET_INIT_DATA = "LynxFrameView.setInitData";
  /**
   * @trace_description: Set LynxFrameView's global props.
   */
  public static final String LYNX_FRAME_VIEW_SET_GLOBAL_PROPS = "LynxFrameView.setGlobalProps";
  /**
   * @trace_description: Measure LynxFrameView.
   */
  public static final String LYNX_FRAME_VIEW_ON_MEASURE = "LynxFrameView.onMeasure";
  /**
   * @trace_description: Measure LynxFrameView with target result.
   */
  public static final String LYNX_FRAME_VIEW_ON_MEASURE_TARGET = "LynxFrameView.onMeasureTarget";
  /**
   * @trace_description: Set LynxFrameView's intrinsic content size.
   */
  public static final String LYNX_FRAME_VIEW_SET_INTRINSIC_CONTENT_SIZE =
      "LynxFrameView.setIntrinsicContentSize";
  /**
   * @trace_description: Update LynxFrameView's layout.
   */
  public static final String LYNX_FRAME_VIEW_UPDATE_LAYOUT = "LynxFrameView.updateLayout";

  /**
   * @trace_description: Measure FrameShadowNode with result.
   * @args{width}: measured width result.
   * @args{height}: measured height result.
   */
  public static final String FRAME_SHADOW_NODE_MEASURE = "FrameShadowNode.measure";

  /**
   * @trace_description: ViewLayoutTick requestLayout.
   */
  public static final String VIEW_LAYOUT_TICK_REQUEST_LAYOUT = "ViewLayoutTick.requestLayout";

  /**
   * @trace_description: ViewLayoutTick triggerLayout.
   */
  public static final String VIEW_LAYOUT_TICK_TRIGGER_LAYOUT = "ViewLayoutTick.triggerLayout";

  /**
   * @trace_description: ChoreographerLayoutTick requestLayout.
   */
  public static final String CHOREOGRAPHER_LAYOUT_TICK_REQUEST_LAYOUT =
      "ChoreographerLayoutTick.requestLayout";

  /**
   * @trace_description: ChoreographerLayoutTick triggerLayout.
   */
  public static final String CHOREOGRAPHER_LAYOUT_TICK_TRIGGER_LAYOUT =
      "ChoreographerLayoutTick.triggerLayout";
}
