// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_TYPES_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_TYPES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/log/logging.h"

#define NLIST_LOGD(msg) LOGD("[List] " << msg)
#define NLIST_LOGV(msg) LOGV("[List] " << msg)
#define NLIST_LOGI(msg) LOGI("[List] " << msg)
#define NLIST_LOGW(msg) LOGW("[List] " << msg)
#define NLIST_LOGE(msg) LOGE("[List] " << msg)

namespace lynx {
namespace tasm {

class ItemHolder;

namespace list {

using ItemKeySet = std::unordered_set<std::string>;
using ItemHolderMap =
    std::unordered_map<std::string, std::unique_ptr<lynx::tasm::ItemHolder>>;
using ItemHolderPtrMap =
    std::unordered_map<std::string, lynx::tasm::ItemHolder*>;

// custom-list-name
static constexpr const char kCustomLisName[] = "custom-list-name";
static constexpr const char kListContainer[] = "list-container";

// props
static constexpr const char* const kEnableScroll = "enable-scroll";
static constexpr const char* const kEnableNestedScroll = "enable-nested-scroll";
static constexpr const char* const kListPlatformInfo = "list-platform-info";
static constexpr const char kListContainerInfo[] = "list-container-info";
static constexpr const char* const kFiberListDiffInfo = "update-list-info";
static constexpr const char* const kBounces = "bounces";
static constexpr const char* const kListType = "list-type";
static constexpr const char* const kListVerticalOrientation =
    "vertical-orientation";
static constexpr const char* const kListMainAxisGap = "list-main-axis-gap";
static constexpr const char* const kListCrossAxisGap = "list-cross-axis-gap";
static constexpr const char* const kColumnCount = "column-count";
static constexpr const char* const kSpanCount = "span-count";
static constexpr const char* const kAnchorPriority = "anchor-priority";
static constexpr const char* const kAnchorVisibility = "anchor-visibility";
static constexpr const char* const kAnchorAlign = "anchor-align";
static constexpr const char* const kNeedsVisibleCells = "needs-visible-cells";
static constexpr const char* const kNeedVisibleItemInfo =
    "need-visible-item-info";
static constexpr const char* const kLowerThresholdItemCount =
    "lower-threshold-item-count";
static constexpr const char* const kUpperThresholdItemCount =
    "upper-threshold-item-count";
static constexpr const char* const kScrollEventThrottle =
    "scroll-event-throttle";
static constexpr const char kExperimentalBatchRenderStrategy[] =
    "experimental-batch-render-strategy";
static constexpr const char* const kInitialScrollIndex = "initial-scroll-index";
static constexpr const char kEstimatedHeightPx[] = "estimated-height-px";
static constexpr const char kEstimatedMainAxisSizePx[] =
    "estimated-main-axis-size-px";
static constexpr const char* const kSticky = "sticky";
static constexpr const char kStickyTop[] = "sticky-top";
static constexpr const char kStickyBottom[] = "sticky-bottom";
static constexpr const char* const kStickyOffset = "sticky-offset";
static constexpr const char* const kEnablePreloadSection =
    "experimental-enable-preload-section";
static constexpr const char* const kScrollOrientation = "scroll-orientation";
static constexpr const char* const kPreloadBufferCount = "preload-buffer-count";

// constant value
static constexpr int kInvalidIndex = -1;
static constexpr int kInvalidDimensionSize = -1.f;
static constexpr const char* const kList = "list";
static constexpr const char* const kListTypeSingle = "single";
static constexpr const char* const kListTypeFlow = "flow";
static constexpr const char* const kListTypeWaterFall = "waterfall";
static constexpr const char* const kAnchorPriorityFromBegin = "fromBegin";
static constexpr const char* const kAnchorPriorityFromEnd = "fromEnd";
static constexpr const char* const kAnchorAlignToBottom = "toBottom";
static constexpr const char* const kAnchorAlignTop = "toTop";
static constexpr const char* const kAnchorVisibilityHide = "hide";
static constexpr const char* const kAnchorVisibilityShow = "show";
static constexpr const char kFiberInsertAction[] = "insertAction";
static constexpr const char kFiberRemoveAction[] = "removeAction";
static constexpr const char kFiberUpdateAction[] = "updateAction";
static constexpr const char kPosition[] = "position";
static constexpr const char kItemKey[] = "item-key";
static constexpr const char kFullSpan[] = "full-span";
static constexpr const char kFrom[] = "from";
static constexpr const char kTo[] = "to";
static constexpr const char kFlush[] = "flush";
static constexpr const char* const kInsertions = "insertions";
static constexpr const char* const kRemovals = "removals";
static constexpr const char* const kUpdateFrom = "updateFrom";
static constexpr const char* const kUpdateTo = "updateTo";
static constexpr const char* const kMoveFrom = "moveFrom";
static constexpr const char* const kMoveTo = "moveTo";
static constexpr const char kDiffResult[] = "diffResult";
static constexpr const char kDataSourceItemKeys[] = "itemkeys";
static constexpr const char* const kDataSourceEstimatedHeightPx =
    "estimatedHeightPx";
static constexpr const char* const kDataSourceEstimatedMainAxisSizePx =
    "estimatedMainAxisSizePx";
static constexpr const char* const kDataSourceFullSpan = "fullspan";
static constexpr const char kDataSourceStickyTop[] = "stickyTop";
static constexpr const char kDataSourceStickyBottom[] = "stickyBottom";
static constexpr const char* const kEnableScrollBar = "scroll-bar-enable";
static constexpr const char* const kItemSnap = "item-snap";

// native storage
static constexpr const char* const kShouldRequestStateRestore =
    "should-request-state-restore";

// event
static constexpr const char* const kScroll = "scroll";
static constexpr const char* const kScrollToUpper = "scrolltoupper";
static constexpr const char* const kScrollToUpperEdge = "scrolltoupperedge";
static constexpr const char* const kScrollToLower = "scrolltolower";
static constexpr const char* const kScrollToLowerEdge = "scrolltoloweredge";
static constexpr const char* const kScrollToNormalState = "scrolltonormalstate";
static constexpr const char* const kLayoutComplete = "layoutcomplete";
static constexpr const char* const kNeedLayoutCompleteInfo =
    "need-layout-complete-info";
static constexpr const char* const kListDebugInfoLevel =
    "list-debug-info-level";
static constexpr const char kListDebugInfoLevelError[] =
    "[Native List Debug] Error";
static constexpr const char kListDebugInfoLevelInfo[] =
    "[Native List Debug] Info";
static constexpr const char kListDebugInfoLevelVerbose[] =
    "[Native List Debug] Verbose";
static constexpr const char* const kListDebugInfoEvent = "listdebuginfo";
static constexpr const char* const kNodeAppear = "nodeappear";
static constexpr const char* const kNodeDisappear = "nodedisappear";
static constexpr const char* const kScrollStateChange = "scrollstatechange";
static constexpr const char* const kScrollEnd = "scrollend";
static constexpr const char kEventSource[] = "eventSource";
static constexpr const char kLayoutID[] = "layout-id";
static constexpr const char kVisibleItemBeforeUpdate[] =
    "visibleItemBeforeUpdate";
static constexpr const char kVisibleItemAfterUpdate[] =
    "visibleItemAfterUpdate";
static constexpr const char kScrollInfo[] = "scrollInfo";
static constexpr const char kEventUnit[] = "eventUnit";
static constexpr const char kEventUnitPx[] = "px";
static constexpr const char kScrollLeft[] = "scrollLeft";
static constexpr const char kScrollTop[] = "scrollTop";
static constexpr const char kScrollWith[] = "scrollWith";
static constexpr const char kScrollHeight[] = "scrollHeight";
static constexpr const char kListWidth[] = "listWith";
static constexpr const char kListHeight[] = "listHeight";
static constexpr const char kDeltaX[] = "deltaX";
static constexpr const char kDeltaY[] = "deltaY";
static constexpr const char kAttachedCells[] = "attachedCells";
static constexpr const char* const kSnap = "snap";

// list timing
static constexpr const char* const kListItemLifecycleStatistic =
    "lynxsdk_list_item_lifecycle_statistic";
static constexpr const char* const kListIdSelector = "list_id_selector";
static constexpr const char* const kListItemUpdateDuration =
    "list_item_update_duration";
static constexpr const char* const kListItemRenderDuration =
    "list_item_render_duration";
static constexpr const char* const kListItemDispatchDuration =
    "list_item_dispatch_duration";
static constexpr const char* const kListItemLayoutDuration =
    "list_item_layout_duration";

// LayoutDirection
enum class LayoutDirection : int32_t {
  kLayoutToStart = -1,
  kLayoutToEnd = 1,
};

// FrameDirection
enum class FrameDirection : uint32_t {
  kLeft = 0,
  kTop,
  kRight,
  kBottom,
};

// LayoutType
enum class LayoutType {
  kSingle = 0,
  kFlow,
  kWaterFall,
};

// Orientation
enum class Orientation { kHorizontal = 0, kVertical };

// Direction
enum class Direction { kNormal = 0, kRTL };

// InitialScrollIndexStatus
enum class InitialScrollIndexStatus {
  kUnset = 0,
  kSet,
  kScrolled,
};

// DiffStatus
enum class DiffStatus {
  kValid = 0,
  kRemoved,
  kUpdateTo,
  kUpdatedFrom,
  kMoveTo,
  kMoveFrom
};

// AnchorVisibility
enum class AnchorVisibility {
  kAnchorVisibilityNoAdjustment,
  kAnchorVisibilityShow,
  kAnchorVisibilityHide
};

// ScrollingInfoAlignment
enum class ScrollingInfoAlignment { kTop, kMiddle, kBottom };

// Debug event log level
enum class ListDebugInfoLevel {
  kListDebugInfoLevelNone = 0,
  kListDebugInfoLevelError,
  kListDebugInfoLevelInfo,
  kListDebugInfoLevelVerbose,
};

// Event source
enum class EventSource {
  kDiff = 0,
  kLayout,
  kScroll,
};

// Scroll position state
enum class ListScrollState {
  kMiddle = 0,  // not upper and not lower
  kUpper,
  kLower,
  kBothEdge,  // on upper and lower
};

// Batch render strategy
enum class BatchRenderStrategy {
  kDefault = 0,
  kBatchRender = 1,
  kAsyncResolveProperty = 2,
  kAsyncResolvePropertyAndElementTree = 3
};

// ScrollState
enum class ScrollState {
  kNone = 0,
  kIdle,
  kDragging,
  kFling,
  kScrollAnimation
};

}  // namespace list
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_TYPES_H_
