// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UTILS_BASE_TASM_CONSTANTS_H_
#define CORE_RENDERER_UTILS_BASE_TASM_CONSTANTS_H_

#include <cstdint>

namespace lynx {
namespace tasm {

static constexpr const char* kOnDocumentUpdated = "OnDocumentUpdated";

// Constant string associated with LepusRuntime
static constexpr const char kGetPageData[] = "getPageData";
static constexpr const char* kGetTextInfo = "getTextInfo";
static constexpr const char* kFontSize = "fontSize";
static constexpr const char* kFontFamily = "fontFamily";
static constexpr const char* kMaxWidth = "maxWidth";
static constexpr const char* kMaxLine = "maxLine";
static constexpr const char kPixelRatio[] = "pixelRatio";

static constexpr const char* kSetTimeout = "setTimeout";
static constexpr const char* kClearTimeout = "clearTimeout";
static constexpr const char* kSetInterval = "setInterval";
static constexpr const char* kClearTimeInterval = "clearInterval";
static constexpr const char* kRequestAnimationFrame = "requestAnimationFrame";
static constexpr const char* kCancelAnimationFrame = "cancelAnimationFrame";
static constexpr const char* kReportError = "reportError";

// Constant string associated with tag
// TODO(WUJINTIAN): Those `static char[]` here will lead to duplicate redundant
// implementations across multiple cpp files. Optimize them to `char*` type.
static constexpr const char kElementViewTag[] = "view";
static constexpr const char kElementComponentTag[] = "component";
static constexpr const char kElementPageTag[] = "page";
static constexpr const char kElementImageTag[] = "image";
static constexpr const char kElementTextTag[] = "text";
static constexpr const char kElementXTextTag[] = "x-text";
static constexpr const char* kElementRawTextTag = "raw-text";
static constexpr const char kElementScrollViewTag[] = "scroll-view";
static constexpr const char kElementXScrollViewTag[] = "x-scroll-view";
static constexpr const char kElementXNestedScrollViewTag[] =
    "x-nested-scroll-view";
static constexpr const char kElementListTag[] = "list";
static constexpr const char kElementNoneElementTag[] = "none";
static constexpr const char kElementWrapperElementTag[] = "wrapper";
static constexpr const char kElementInlineTextTag[] = "inline-text";
static constexpr const char kElementXInlineTextTag[] = "x-inline-text";
static constexpr const char kElementInlineImageTag[] = "inline-image";
static constexpr const char* kElementXImageTag = "x-image";
static constexpr const char kElementXInlineImageTag[] = "x-inline-image";

// Constant string associated with Element Template
static constexpr const int kInvalidCssId = -1;
static constexpr const char* kElementID = "id";
static constexpr const char* kElementTempID = "tempID";
static constexpr const char kElementIdSelector[] = "idSelector";
static constexpr const char* kElementType = "type";
static constexpr const char* kElementChildren = "children";
static constexpr const char* kElementClass = "class";
static constexpr const char* kElementStyles = "styles";
static constexpr const char* kElementBuiltinAttributes = "builtinAttributes";
static constexpr const char* kElementAttributes = "attributes";
static constexpr const char* kElementEvents = "events";
static constexpr const char* kElementDataset = "dataset";
static constexpr const char* kElementIsComponent = "isComponent";
static constexpr const char* kElementParsedStyleKey = "parsedStyleKey";
static constexpr const char* kElementConfig = "config";
static constexpr const char* kElementParsedStyle = "parsedStyle";

static constexpr const char* kElementComponentName = "name";
static constexpr const char* kElementComponentPath = "path";
static constexpr const char* kElementCSSID = "cssID";
static constexpr const char kDirtyID[] = "dirtyID";
static constexpr const char kDepth[] = "depth";
static constexpr const char kCloneResolvedProps[] = "cloneResolvedProps";

// Constant string associated with tasm
static constexpr const char kRemoveComponents[] = "removeComponents";
static constexpr const char kUpdatePage[] = "updatePage";
static constexpr const char kComponentID[] = "ComponentID";
static constexpr const char kUpdateGlobalProps[] = "updateGlobalProps";

// Constant string associated with radon
static constexpr const char kRemoveComponentElement[] =
    "removeComponentElement";
static constexpr const char* kComponentCreated = "created";
static constexpr const char* kComponentAttached = "attached";
static constexpr const char* kComponentReady = "ready";
static constexpr const char* kComponentDetached = "detached";
static constexpr const char* kComponentMoved = "moved";
static constexpr const char* kComponentClassName = "className";
static constexpr const char* kComponentIdSelector = "id";
static constexpr const char* kRootCSSId = ":root";
static constexpr const uint32_t kRadonPageID = 0;

static constexpr const char kTransmitClassDirty[] = "transmit-class-change";

// Constant string for preload
static constexpr const char kPreLoadTemplate[] = "preLoadTemplate";
static constexpr const char kTriggerLifeCycle[] = "triggerLifeCycle";
// Constant string associated with UpdatePageOption
static constexpr const char kNativeUpdateDataOrder[] = "nativeUpdateDataOrder";
static constexpr const char kType[] = "type";
static constexpr const char kProcessData[] = "processData";
static constexpr const char kProcessorName[] = "processorName";
static constexpr const char kCacheData[] = "cacheData";
static constexpr const char kData[] = "data";
static constexpr const char kRenderPage[] = "renderPage";
static constexpr const char kInitPage[] = "initPage";
static constexpr const char* kHydrateMap = "hydrateMap";
// Constant string associated with PipelineOptions
static constexpr const char kPipelineOptions[] = "pipelineOptions";
static constexpr const char kPipelineID[] = "pipelineID";
static constexpr const char kPipelineOrigin[] = "pipelineOrigin";
static constexpr const char kPipelineNeedTimestamps[] = "needTimestamps";

// Constant string for worklet
static constexpr const char* kWorklet = "worklet";

// others
static constexpr const char kWidth[] = "width";
static constexpr const char kHeight[] = "height";
static constexpr const char kGlobalLynx[] = "lynx";
static constexpr const char kEnableParallelElement[] = "enableParallelElement";
static constexpr const char kNodeIndex[] = "nodeIndex";
static constexpr const char kContent[] = "content";
static constexpr const char* kTrue = "true";
static constexpr const char* kFalse = "false";

// Constant string for event
static constexpr const char kTarget[] = "target";
static constexpr const char kOrigin[] = "origin";
static constexpr const char kTimestamp[] = "timestamp";
static constexpr const char kLayout[] = "layout";

// Constant string for meta component
static constexpr const char kTextOverFlow[] = "text-overflow";
static constexpr const char kScrollX[] = "scroll-x";
static constexpr const char kScrollY[] = "scroll-y";
static constexpr const char kScrollXReverse[] = "scroll-x-reverse";
static constexpr const char kScrollYReverse[] = "scroll-y-reverse";
static constexpr const char kScrollOrientation[] = "scroll-orientation";
static constexpr const char kVerticalOrientation[] = "vertical-orientation";
static constexpr const char kHorizontal[] = "horizontal";
static constexpr const char kVertical[] = "vertical";
static constexpr const char kColumnCount[] = "column-count";
static constexpr const char kSpanCount[] = "span-count";

// storage API
static constexpr const char* kSetSessionStorageItem = "setSessionStorageItem";
static constexpr const char* kGetSessionStorageItem = "getSessionStorageItem";

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_TASM_CONSTANTS_H_
