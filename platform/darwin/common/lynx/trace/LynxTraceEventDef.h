// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
NS_ASSUME_NONNULL_BEGIN

#if ENABLE_TRACE_PERFETTO

/**
 * @trace_description: Check the exposure and disexposure states of LynxUIs and send disexposure
 * and exposure events to trigger custom exposure listeners. link:
 * @link{https://lynxjs.org/guide/interaction/visibility-detection/exposure-ability.html}
 */
static NSString* const UI_EXPOSURE_HANDLER = @"LynxUIExposure.exposureHandler";
/**
 * @trace_description:  Layout of <text> element's platform layout node, where the preview text
 * are `@args{preview_text}`.
 * @history_name{text.TextShadowNode.measure}
 */
static NSString* const TEXT_SHADOW_NODE_MEASURE = @"TextShadowNode.measure";

static NSString* const RESOURCE_MODULE_CANCEL_PREFETCH = @"cancelResourcePrefetch";

static NSString* const RESOURCE_MODULE_REQUEST_PREFETCH = @"requestResourcePrefetch";

static NSString* const UI_OWNER_INSERT_NODE = @"UIOwner.insertNode.";
static NSString* const UI_OWNER_CREATE_VIEW = @"UIOwner.createView.";
static NSString* const UI_OWNER_CREATE_VIEW_ASYNC = @"UIOwner.createViewAsync.";
static NSString* const UI_OWNER_UPDATE_PROPS = @"UIOwner.updateProps.";
static NSString* const UI_OWNER_REMOVE_RECURSIVELY = @"UIOwner.removeRecursively.";
static NSString* const UI_OWNER_REMOVE = @"UIOwner.remove.";
static NSString* const UI_OWNER_UPDATE_LAYOUT = @"UIOwner.updateLayout.";
static NSString* const UI_OWNER_RECEIVE_UI_OPERATION = @"UIOwner.ReceiveUIOperation.";
static NSString* const UI_OWNER_INVOKE_UI_METHOD_FOR_SELECTOR_QUERY =
    @"UIOwner.invokeUIMethodForSelectorQuery.";
static NSString* const UI_OWNER_LAYOUT_FINISH = @"UIOwner.layoutFinish.";

static NSString* const UI_OWNER_INIT = @"LynxUIOwner init";

static NSString* const LIST_LIGHT_VIEW_DISPATCH_INVALID_CONTEXT = @"dispatchInvalidationContext";

static NSString* const LIST_LIGHT_VIEW_LOAD_NEW_CELL_AT_INDEX = @"loadNewCellAtIndex";
static NSString* const LIST_LIGHT_VIEW_ON_COMPONENT_LAYOUT_UPDATE = @"onComponentLayoutUpdated";
static NSString* const LIST_LIGHT_VIEW_ON_ASYNC_COMPONENT_LAYOUT_UPDATE =
    @"onAsyncComponentLayoutUpdated";
static NSString* const LIST_LIGHT_VIEW_RECYCLE_CELL = @"recycleCell";
static NSString* const LIST_LIGHT_VIEW_ADJUST_WITH_BOUNDS_CHANGE = @"adjustWithBoundsChange";
static NSString* const LIST_LIGHT_VIEW_FILL_TO_UPPER_BOUNDS = @"fillToUpperBoundsIfNecessary";
static NSString* const LIST_LIGHT_VIEW_FILL_TO_LOWER_BOUNDS = @"fillToLowerBoundsIfNecessary";
static NSString* const LIST_LIGHT_VIEW_REFRESH_DISPLAY_CELLS = @"refreshDisplayCells";

static NSString* const SERVICES_REGISTER_SERVICES = @"LynxServices registerServices";

static NSString* const TEMPLATE_RENDER_UPDATE_GLOBAL_PROPS = @"TemplateRender::updateGlobalProps";
static NSString* const FONT_FACE_MANAGER_REQUEST_WITH_GENERIC_FETCHER =
    @"LynxFontFaceManager requestFontfaceItemWithGenericResourceFetcher";
static NSString* const FONT_FACE_MANAGER_REQUEST_BY_FONT_PROVIDER =
    @"LynxFontFaceManager requestFontfaceByFontProvider";
static NSString* const FONT_FACE_MANAGER_REQUEST_BY_RESOURCE_PROVIDER =
    @"LynxFontFaceManager requestFontfaceByFontProvider";

static NSString* const SHADOW_NODE_OWNER_DID_LAYOUT_STATE =
    @"LynxShadowNodeOwner.didLayoutStartOnNode";
static NSString* const SHADOW_NODE_OWNER_DID_UPDATE_LAYOUT = @"LynxShadowNodeOwner.didUpdateLayout";

static NSString* const TEXT_RENDERER_INIT = @"LynxTextRenderer.init";
static NSString* const TEXT_RENDERER_LAYOUT = @"LynxTextRenderer.layout";
static NSString* const TEXT_RENDERER_ENSURE_LAYOUT = @"LynxTextRenderer.ensureLayout";
static NSString* const TEXT_SHADOW_NODE_ALIGN = @"LynxTextShadowNode.align";

static NSString* const LIST_DELEGATE_DID_SCROLL = @"LynxUIListDelegate::listDidScroll";
static NSString* const LIST_DELEGATE_WILL_BEGIN_DRAGGING =
    @"LynxUIListDelegate::listWillBeginDragging";
static NSString* const LIST_DELEGATE_DID_END_DRAGGING =
    @"LynxUIListDelegate::scrollerDidEndDragging";
static NSString* const LIST_DELEGATE_DID_END_DECELERATING =
    @"LynxUIListDelegate::listDidEndDecelerating";

static NSString* const SCROLLER_DELEGATE_DID_SCROLL = @"LynxUIScrollerDelegate::scrollerDidScroll";
static NSString* const SCROLLER_DELEGATE_DID_END_DECELERATING =
    @"LynxUIScrollerDelegate::scrollerDidEndDecelerating";
static NSString* const SCROLLER_DELEGATE_DID_END_DRAGGING =
    @"LynxUIScrollerDelegate::scrollerDidEndDragging";
static NSString* const SCROLLER_DELEGATE_WILL_BEGIN_DRAGGING =
    @"LynxUIScrollerDelegate::scrollerWillBeginDragging";
static NSString* const SCROLLER_DELEGATE_DID_END_SCROLL_ANIMATION =
    @"LynxUIScrollerDelegate::scrollerDidEndScrollingAnimation";

static NSString* const MEDIA_FETCHER_SHOULD_REDIRECT = @"MediaFetcher.shouldRedirectImageUrl";

static NSString* const FLUENCY_MONITOR_START_FLUENCY_TRACE = @"StartFluencyTrace";
static NSString* const FLUENCY_MONITOR_STOP_FLUENCY_TRACE = @"StopFluencyTrace";

static const char* const LYNX_VIEW_LIFECYCLE_ON_PIPER_INVOKED = "LynxViewLifeCycle::onPiperInvoked";
static const char* const LYNX_VIEW_LIFECYCLE_ON_PIPER_RESPONSE =
    "LynxViewLifeCycle::onPiperResponsed";
static const char* const LYNX_RESOURCE_SERVICE_FETCHER_FETCH_RESOURCE =
    "LynxResourceServiceFetcher::fetchResource";
static const char* const DYNAMIC_COMPONENT_FETCHER_LOAD_COMPONENT =
    "DynamicComponentFetcher::loadDynamicComponent";

static const char* const DEVTOOL_INIT_WITH_LYNX_VIEW = "LynxDevtool::initWithLynxView";
static const char* const DEVTOOL_ON_LOAD_FROM_LOCAL_FILE = "LynxDevtool::onLoadFromLocalFile";
static const char* const DEVTOOL_ON_LOAD_FROM_URL = "LynxDevtool::onLoadFromURL";
static const char* const DEVTOOL_ON_LOAD_FROM_BUNDLE = "LynxDevtool::onLoadFromBundle";
static const char* const EVENT_REPORTER_ON_EVENT = "LynxEventReporter::onEvent";
static const char* const EVENT_REPORTER_UPDATE_GENERIC_INFO =
    "LynxEventReporter::updateGenericInfo";
static const char* const EVENT_REPORTER_UPDATE_GENERIC_INFO_RUN =
    "LynxEventReporter::updateGenericInfo.run";
static const char* const EVENT_REPORTER_REMOVE_GENERIC_INFO =
    "LynxEventReporter::removeGenericInfo";
static const char* const EVENT_REPORTER_REMOVE_GENERIC_INFO_RUN =
    "LynxEventReporter::removeGenericInfo.run";
static const char* const EVENT_REPORTER_GET_GENERIC_INFO = "LynxEventReporter::getGenericInfo";
static const char* const EVENT_REPORTER_GET_GENERIC_INFO_RUN =
    "LynxEventReporter::getGenericInfo.run";
static const char* const EVENT_REPORTER_SET_EXTRA_PARAMS = "LynxEventReporter::setExtraParams";
static const char* const EVENT_REPORTER_CLEAR_CACHE_BY_INSTANCE_ID =
    "LynxEventReporter::clearCacheByInstanceId";
static const char* const EVENT_REPORTER_HANDLE_EVENT = "LynxEventReporter::handleEvent";

static const char* const TEMPLATE_RENDER_INIT_WITH_BUILDER_BLOCK =
    "LynxTemplateRender::initWithBuilderBlock";
static const char* const TEMPLATE_RENDER_CUSTOM_BUILDER = "LynxTemplateRender::customBuilder";
static const char* const TEMPLATE_RENDER_SETUP_SCREEN_SIZE =
    "LynxTemplateRender::setUpEnvWidthScreenSize";
static const char* const TEMPLATE_RENDER_SETUP_FRAME = "LynxTemplateRender::setUpFrame";
static const char* const TEMPLATE_RENDER_SETUP_EVENT_HANDLER =
    "LynxTemplateRender::setUpEventHandler";
static const char* const TEMPLATE_RENDER_SETUP_SHELL = "LynxTemplateRender::setUpLynxShell";
static const char* const TEMPLATE_RENDER_SETUP_RUNTIME = "LynxTemplateRender::setUpRuntime";
static const char* const TEMPLATE_RENDER_INIT_RUNTIME =
    "LynxTemplateRender::setUpRuntime:InitRuntime";
static const char* const MODULE_MANAGER_ADD_WRAPPERS = "ModuleManager::addWrappers";
static const char* const TEMPLATE_RENDER_LOAD_TEMPLATE_BUNDLE =
    "LynxTemplateRender::loadTemplateBundle";
static const char* const TEMPLATE_RENDER_INTERNAL_LOAD_TEMPLATE =
    "LynxTemplateRender::internalLoadTemplate";
static const char* const TEMPLATE_RENDER_CREATE_TEMPLATE_DATA = "CreateTemplateData";
static const char* const SERVICE_REPORT_ERROR_GLOBAL_CONTEXT_TAG = "reportErrorGlobalContextTag";
static const char* const TEMPLATE_RENDER_START_LOAD = "StartLoad";
static const char* const TEMPLATE_RENDER_PREPARE_SHELL = "LynxTemplateRender::prepareShell";
static const char* const TEMPLATE_RENDER_DID_START_LOADING =
    "LynxTemplateRender::dispatchViewDidStartLoading";
static const char* const TEMPLATE_RENDER_UPDATE_VIEWPORT = "LynxTemplateRender::updateViewport";
static const char* const TEMPLATE_RENDER_SET_EXTRA_TIMING = "LynxTemplateRender::setExtraTiming";
static const char* const TEMPLATE_RENDER_ATTACH_LYNX_VIEW = "LynxTemplateRender::attachLynxView";
static const char* const TEMPLATE_RENDER_PROCESS_RENDER = "LynxTemplateRender::processRender";
static const char* const TEMPLATE_RENDER_PROCESS_LAYOUT = "LynxTemplateRender::processLayout";
static const char* const TEMPLATE_RENDER_PROCESS_LAYOUT_WITH_TEMPLATE_BUNDLE =
    "LynxTemplateRender::processLayoutWithTemplateBundle";
static const char* const LYNX_VIEW_INIT_WITH_BUILDER_BLOCK = "LynxView::initWithBuilderBlock";
static const char* const LYNX_VIEW_INIT_WITHOUT_RENDER = "LynxView::initWithoutRender";
static const char* const LYNX_VIEW_INIT_LIFECYCLE_DISPATCHER = "LynxView::initLifecycleDispatcher";
static const char* const LYNX_VIEW_LIFECYCLE_REPORT_COMPONENT_INFO =
    "LynxViewLifeCycle::didReportComponentInfo";
static const char* const LYNX_VIEW_LOAD_TEMPLATE_WITH_LOAD_META =
    "LynxView::loadTemplateWithLynxLoadMeta";
static const char* const LYNX_VIEW_LOAD_TEMPLATE_FROM_URL = "LynxView::loadTemplateFromURL";
static const char* const LYNX_VIEW_LOAD_TEMPLATE_WITH_URL = "LynxView::loadTemplateWithURL";
static const char* const LYNX_VIEW_LOAD_TEMPLATE_BUNDLE = "LynxView::loadTemplateBundle";
static const char* const LYNX_VIEW_LAYOUT_SUBVIEWS = "LynxView::layoutSubviews";
static const char* const LYNX_VIEW_LIFECYCLE_CHANGE_CONTENT_SIZE =
    "LynxViewLifeCycle::DidChangeIntrinsicContentSize";
static const char* const LYNX_VIEW_LIFECYCLE_RECIEVE_ERROR = "LynxViewLifeCycle::didRecieveError";
static const char* const LYNX_VIEW_LIFECYCLE_FIRST_SCREEN =
    "LynxViewLifeCycle::lynxViewDidFirstScreen";
static const char* const LYNX_VIEW_LIFECYCLE_PAGE_UPDATE =
    "LynxViewLifeCycle::lynxViewDidPageUpdate";
static const char* const LYNX_VIEW_LIFECYCLE_LOAD_FINISHED_WITH_URL =
    "LynxViewLifecycle::didLoadFinishedWithUrl";
static const char* const LYNX_VIEW_LIFECYCLE_RECEIVE_FIRST_LOAD_PERF =
    "LynxViewLifecycle::didReceiveFirstLoadPerf";
static const char* const LYNX_VIEW_LIFECYCLE_RECEIVE_UPDATE_PERF =
    "LynxViewLifecycle::didReceiveUpdatePerf";
static const char* const LYNX_VIEW_LIFECYCLE_RECEIVE_LAZY_BUNDLE_PERF =
    "LynxViewLifecycle::didReceiveLazyBundlePerf";
static const char* const LYNX_VIEW_LIFECYCLE_INVOKE_METHOD = "LynxViewLifecycle::didInvokeMethod";
static const char* const LYNX_VIEW_LIFECYCLE_STARTED_WITH_LYNX_VIEW =
    "LynxViewLifecycle::onPageStartedWithLynxView";
static const char* const LYNX_VIEW_BUILDER_INIT = "LynxViewBuilder::init";
static const char* const LAYOUT_NODE_MEASURE = "LynxLayoutNode::measure";
static const char* const IMAGE_FETCHER_LOAD_IMAGE = "LynxImageFetcher::loadImageWithURL";

static const char* const LYNX_ENGINE_POOL_REGISTER_ENGINE = "LynxEnginePool::registerReuseEngine";
static const char* const LYNX_ENGINE_POOL_POLL_ENGINE = "LynxEnginePool::pollEngineWithRender";

static const char* const UI_IMAGE_REQUEST_IMAGE = "LynxUIImage::requestImage";
static const char* const EVALUATE_SCRIPT_STANDALONE = "EvaluateScriptStandalone";
static const char* const EVALUATE_SCRIPT_STANDALONE_FROM_BUNDLE =
    "EvaluateScriptStandaloneFromBundle";

#endif

NS_ASSUME_NONNULL_END
