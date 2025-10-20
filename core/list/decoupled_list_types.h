// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_TYPES_H_
#define CORE_LIST_DECOUPLED_LIST_TYPES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/log/logging.h"

namespace lynx {
namespace list {

#define DLIST_LOGD(msg) LOGD("[DecoupledList] " << msg)
#define DLIST_LOGV(msg) LOGV("[DecoupledList] " << msg)
#define DLIST_LOGI(msg) LOGI("[DecoupledList] " << msg)
#define DLIST_LOGW(msg) LOGW("[DecoupledList] " << msg)
#define DLIST_LOGE(msg) LOGE("[DecoupledList] " << msg)

class ItemHolder;

using ItemKeySet = std::unordered_set<std::string>;
using ItemHolderMap =
    std::unordered_map<std::string, std::unique_ptr<list::ItemHolder>>;
using ItemHolderPtrMap = std::unordered_map<std::string, list::ItemHolder*>;

// props
static constexpr const char* const kPropEnableScroll = "enable-scroll";
static constexpr const char* const kPropCustomListName = "custom-list-name";
static constexpr const char* const kPropScrollOrientation =
    "scroll-orientation";
static constexpr const char* const kPropListType = "list-type";
static constexpr const char* const kPropSpanCount = "span-count";
static constexpr const char* const kPropListMainAxisGap = "list-main-axis-gap";
static constexpr const char* const kPropListCrossAxisGap =
    "list-cross-axis-gap";
static constexpr const char* const kPropSticky = "sticky";
static constexpr const char* const kPropStickyOffset = "sticky-offset";
static constexpr const char* const kPropEnableNestedScroll =
    "enable-nested-scroll";
static constexpr const char* const kPropListPlatformInfo = "list-platform-info";
static constexpr const char* const kPropFiberListDiffInfo = "update-list-info";
static constexpr const char* const kPropPreloadBufferCount =
    "preload-buffer-count";
static constexpr const char* const kPropBounces = "bounces";
static constexpr const char* const kPropEnableDynamicSpanCount =
    "enable-dynamic-span-count";
static constexpr const char* const kPropStickyBufferCount =
    "sticky-buffer-count";
static constexpr const char* const kPropNeedVisibleItemInfo =
    "need-visible-item-info";
static constexpr const char* const kPropLowerThresholdItemCount =
    "lower-threshold-item-count";
static constexpr const char* const kPropUpperThresholdItemCount =
    "upper-threshold-item-count";
static constexpr const char* const kPropScrollEventThrottle =
    "scroll-event-throttle";
static constexpr const char* const kPropInitialScrollIndex =
    "initial-scroll-index";
static constexpr const char* const kPropNeedLayoutCompleteInfo =
    "need-layout-complete-info";
static constexpr const char* const kPropLayoutId = "layout-id";
static constexpr const char* const kPropShouldRequestStateRestore =
    "should-request-state-restore";
static constexpr const char* const kPropExperimentalRecycleStickyItem =
    "experimental-recycle-sticky-item";
static constexpr const char* const kPropExperimentalUpdateStickyForDiff =
    "experimental-update-sticky-for-diff";
static constexpr const char* const
    kPropExperimentalRecycleAvailableItemBeforeLayout =
        "experimental-recycle-available-item-before-layout";

// event
static constexpr const char* const kEventScroll = "scroll";
static constexpr const char* const kEventScrollToUpper = "scrolltoupper";
static constexpr const char* const kEventScrollToUpperEdge =
    "scrolltoupperedge";
static constexpr const char* const kEventScrollToLower = "scrolltolower";
static constexpr const char* const kEventScrollToLowerEdge =
    "scrolltoloweredge";
static constexpr const char* const kEventScrollToNormalState =
    "scrolltonormalstate";
static constexpr const char* const kEventLayoutComplete = "layoutcomplete";
static constexpr const char* const kEventNodeAppear = "nodeappear";
static constexpr const char* const kEventNodeDisappear = "nodedisappear";
static constexpr const char* const kEventScrollStateChange =
    "scrollstatechange";
static constexpr const char* const kEventScrollEnd = "scrollend";
static constexpr const char* const kEventParamDetail = "detail";
static constexpr const char* const kScrollInfo = "scrollInfo";
static constexpr const char* const kEventUnit = "eventUnit";
static constexpr const char* const kEventUnitPx = "px";
// event layout complete info
static constexpr const char* const kLayoutInfoLayoutId = "layout-id";
static constexpr const char* const kLayoutInfoScrollInfo = "scrollInfo";
static constexpr const char* const kLayoutInfoDiffResult = "diffResult";
static constexpr const char* const kLayoutInfoVisibleItemBeforeUpdate =
    "visibleItemBeforeUpdate";
static constexpr const char* const kLayoutInfoVisibleItemAfterUpdate =
    "visibleItemAfterUpdate";
static constexpr const char* const kDiffResultInsertions = "insertions";
static constexpr const char* const kDiffResultRemovals = "removals";
static constexpr const char* const kDiffResultUpdateFrom = "update_from";
static constexpr const char* const kDiffResultUpdateTo = "update_to";
static constexpr const char* const kDiffResultMoveFrom = "move_from";
static constexpr const char* const kDiffResultMoveTo = "move_to";
// event scroll info
static constexpr const char* const kScrollInfoEventSource = "eventSource";
static constexpr const char* const kScrollInfoAttachedCells = "attachedCells";
static constexpr const char* const kScrollInfoScrollLeft = "scrollLeft";
static constexpr const char* const kScrollInfoScrollTop = "scrollTop";
static constexpr const char* const kScrollInfoScrollWidth = "scrollWidth";
static constexpr const char* const kScrollInfoScrollHeight = "scrollHeight";
static constexpr const char* const kScrollInfoListWidth = "listWidth";
static constexpr const char* const kScrollInfoListHeight = "listHeight";
static constexpr const char* const kScrollInfoDeltaX = "deltaX";
static constexpr const char* const kScrollInfoDeltaY = "deltaY";
// event cell info
static constexpr const char* const kCellInfoIdSelector = "id";
static constexpr const char* const kCellInfoItemKey = "itemKey";
static constexpr const char* const kCellInfoIndex = "index";
static constexpr const char* const kCellInfoPosition = "position";
static constexpr const char* const kCellInfoTop = "top";
static constexpr const char* const kCellInfoLeft = "left";
static constexpr const char* const kCellInfoBottom = "bottom";
static constexpr const char* const kCellInfoRight = "right";
static constexpr const char* const kCellInfoOriginX = "originX";
static constexpr const char* const kCellInfoOriginY = "originY";
static constexpr const char* const kCellInfoWidth = "width";
static constexpr const char* const kCellInfoHeight = "height";
static constexpr const char* const kCellInfoIsBinding = "isBinding";
static constexpr const char* const kCellInfoUpdated = "updated";

// constant value
static constexpr int kInvalidIndex = -1;
static constexpr int kInvalidItemCount = -1;
static constexpr int kInvalidDimensionSize = -1.f;
static constexpr int kStickyItemSetCapacityForSyncMode = 1;
static constexpr int kStickyItemSetCapacityForASyncMode = 2;
static constexpr int kDefaultMainAxisItemSize = 200;
static constexpr const char kListContainer[] = "list-container";
static constexpr const char kListContainerInfo[] = "list-container-info";
static constexpr const char kEstimatedMainAxisSizePx[] =
    "estimated-main-axis-size-px";
static constexpr const char kEstimatedHeightPx[] = "estimated-height-px";
static constexpr const char kRecyclable[] = "recyclable";
static constexpr const char kStickyTop[] = "sticky-top";
static constexpr const char kStickyBottom[] = "sticky-bottom";
static constexpr const char kFiberInsertAction[] = "insertAction";
static constexpr const char kFiberRemoveAction[] = "removeAction";
static constexpr const char kFiberUpdateAction[] = "updateAction";
static constexpr const char kPosition[] = "position";
static constexpr const char kItemKey[] = "item-key";
static constexpr const char kFullSpan[] = "full-span";
static constexpr const char kFrom[] = "from";
static constexpr const char kTo[] = "to";
static constexpr const char kFlush[] = "flush";
static constexpr const char kDiffResult[] = "diffResult";
static constexpr const char kDataSourceItemKeys[] = "itemkeys";
static constexpr const char kDataSourceStickyTop[] = "stickyTop";
static constexpr const char kDataSourceStickyBottom[] = "stickyBottom";
static constexpr const char kDataSourceStickyStart[] = "stickyStart";
static constexpr const char kDataSourceStickyEnd[] = "stickyEnd";
static constexpr const char kDataSourceEstimatedHeightPx[] =
    "estimatedHeightPx";
static constexpr const char kDataSourceEstimatedMainAxisSizePx[] =
    "estimatedMainAxisSizePx";
static constexpr const char kDataSourceFullSpan[] = "fullspan";
static constexpr const char* const kList = "list";
static constexpr const char* const kListTypeSingle = "single";
static constexpr const char* const kListTypeFlow = "flow";
static constexpr const char* const kListTypeWaterFall = "waterfall";
static constexpr const char* const kInsertions = "insertions";
static constexpr const char* const kRemovals = "removals";
static constexpr const char* const kUpdateFrom = "updateFrom";
static constexpr const char* const kUpdateTo = "updateTo";
static constexpr const char* const kMoveFrom = "moveFrom";
static constexpr const char* const kMoveTo = "moveTo";
static constexpr const char* const kEnableScrollBar = "scroll-bar-enable";
static constexpr const char* const kItemSnap = "item-snap";
static constexpr const char* const kSnap = "snap";

// static constexpr const char* const kAnchorPriorityFromBegin = "fromBegin";
// static constexpr const char* const kAnchorPriorityFromEnd = "fromEnd";
// static constexpr const char* const kAnchorAlignToBottom = "toBottom";
// static constexpr const char* const kAnchorAlignTop = "toTop";
// static constexpr const char* const kAnchorVisibilityHide = "hide";
// static constexpr const char* const kAnchorVisibilityShow = "show";

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
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_TYPES_H_
