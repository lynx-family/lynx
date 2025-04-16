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
 * @trace_description:  Layout of <text> element's platform layout node, where the element's
 * characters are @args{characters} or the first 50 characters are @args{first_fifty_characters}.
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

#endif

NS_ASSUME_NONNULL_END
