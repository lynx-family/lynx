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

#define DLIST_LOGD(msg) LOGD("[List] " << msg)
#define DLIST_LOGV(msg) LOGV("[List] " << msg)
#define DLIST_LOGI(msg) LOGI("[List] " << msg)
#define DLIST_LOGW(msg) LOGW("[List] " << msg)
#define DLIST_LOGE(msg) LOGE("[List] " << msg)

class ItemHolder;

using ItemKeySet = std::unordered_set<std::string>;
using ItemHolderMap =
    std::unordered_map<std::string, std::unique_ptr<list::ItemHolder>>;
using ItemHolderPtrMap = std::unordered_map<std::string, list::ItemHolder*>;

// props
static constexpr const char* const kPropCustomListName = "custom-list-name";
static constexpr const char kPropEnableDecoupledList[] =
    "enable-decoupled-list";
static constexpr const char* const kPropScrollOrientation =
    "scroll-orientation";
static constexpr const char* const kPropVerticalOrientation =
    "vertical-orientation";
static constexpr const char* const kPropUpdateAnimation = "update-animation";
static constexpr const char* const kPropListType = "list-type";
static constexpr const char* const kPropSpanCount = "span-count";
static constexpr const char* const kPropColumnCount = "column-count";
static constexpr const char* const kPropListMainAxisGap = "list-main-axis-gap";
static constexpr const char* const kPropListCrossAxisGap =
    "list-cross-axis-gap";
static constexpr const char* const kPropSticky = "sticky";
static constexpr const char* const kPropStickyOffset = "sticky-offset";
static constexpr const char* const kPropEnableNestedScroll =
    "enable-nested-scroll";
static constexpr const char* const kPropEnableScroll = "enable-scroll";
static constexpr const char* const kPropRadonListPlatformInfo =
    "list-platform-info";
static constexpr const char* const kPropFiberUpdateListInfo =
    "update-list-info";
static constexpr const char* const kPropPreloadBufferCount =
    "preload-buffer-count";
static constexpr const char* const kPropEnableInsertPlatformViewOperation =
    "enable-insert-platform-view-operation";
static constexpr const char* const kPropBounces = "bounces";
static constexpr const char* const kPropEnableDynamicSpanCount =
    "enable-dynamic-span-count";
static constexpr const char* const kPropStickyBufferCount =
    "sticky-buffer-count";
static constexpr const char* const kPropNeedVisibleItemInfo =
    "need-visible-item-info";
static constexpr const char* const kPropNeedsVisibleCells =
    "needs-visible-cells";
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
static constexpr const char* const kPropExperimentalBatchRenderStrategy =
    "experimental-batch-render-strategy";
static constexpr const char* const kPropAnchorPriority = "anchor-priority";
static constexpr const char* const kPropAnchorVisibility = "anchor-visibility";
static constexpr const char* const kPropAnchorAlign = "anchor-align";
static constexpr const char* const kPropEnablePreloadSection =
    "experimental-enable-preload-section";
static constexpr const char* const kPropExperimentalContinuousResolveTree =
    "experimental-continuous-resolve-tree";
static constexpr const char* const kPropListDebugInfoLevel =
    "list-debug-info-level";
static constexpr const char* const kPropExperimentalSearchRefAnchorStrategy =
    "experimental-search-ref-anchor-strategy";

// prop value
static constexpr const char* const kPropValueTrue = "true";
static constexpr const char* const kPropValueListTypeSingle = "single";
static constexpr const char* const kPropValueListTypeFlow = "flow";
static constexpr const char* const kPropValueListTypeWaterFall = "waterfall";
static constexpr const char* const kPropValueListContainer = "list-container";
static constexpr const char* const kPropValueUpdateAnimationDefault = "default";
static constexpr const char* const kPropValueScrollOrientationHorizontal =
    "horizontal";
static constexpr const char* const kPropValueScrollOrientationVertical =
    "vertical";
static constexpr const char* const kPropValueAnchorPriorityFromBegin =
    "fromBegin";
static constexpr const char* const kPropValueAnchorPriorityFromEnd = "fromEnd";
static constexpr const char* const kPropValueAnchorAlignToBottom = "toBottom";
static constexpr const char* const kPropValueAnchorAlignToTop = "toTop";
static constexpr const char* const kPropValueAnchorVisibilityHide = "hide";
static constexpr const char* const kPropValueAnchorVisibilityShow = "show";

// list-container-info
static constexpr const char* kListContainerInfo = "list-container-info";
static constexpr const char* kListContainerInfoStickyStart = "stickyStart";
static constexpr const char* kListContainerInfoStickyEnd = "stickyEnd";
static constexpr const char* kListContainerInfoItemKeys = "itemkeys";

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
static constexpr const char* const kEventListDebugInfo = "listdebuginfo";
static constexpr const char* const kEventParamDetail = "detail";

// event layout complete info
static constexpr const char* const kLayoutInfoLayoutId = "layout-id";
static constexpr const char* const kLayoutInfoScrollInfo = "scrollInfo";
static constexpr const char* const kLayoutInfoDiffResult = "diffResult";
static constexpr const char* const kLayoutInfoEventUnit = "eventUnit";
static constexpr const char* const kLayoutInfoVisibleItemBeforeUpdate =
    "visibleItemBeforeUpdate";
static constexpr const char* const kLayoutInfoVisibleItemAfterUpdate =
    "visibleItemAfterUpdate";
static constexpr const char* const kLayoutInfoEventUnitValuePx = "px";

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

// event diff info
static constexpr const char* const kDiffInfoInsertion = "insertions";
static constexpr const char* const kDiffInfoRemoval = "removals";
static constexpr const char* const kDiffInfoUpdateFrom = "update_from";
static constexpr const char* const kDiffInfoUpdateTo = "update_to";
static constexpr const char* const kDiffInfoMoveFrom = "move_from";
static constexpr const char* const kDiffInfoMoveTo = "move_to";

// event debug info
static constexpr const char* const kDebugInfoDiffResult = "diffResult";
static constexpr const char* const kDebugInfoAnchorInfo = "anchor_info";
static constexpr const char* const kDebugInfoAnchorIndex = "anchor_index";
static constexpr const char* const kDebugInfoAnchorStartOffset = "start_offset";
static constexpr const char* const kDebugInfoAnchorStartAlignmentDelta =
    "start_alignment_delta";
static constexpr const char* const kDebugInfoAnchorDirty = "dirty";
static constexpr const char* const kDebugInfoAnchorBinding = "binding";

// radon arch data
static constexpr const char* const kRadonDataDiffResult = "diffResult";
static constexpr const char* const kRadonDataInsertions = "insertions";
static constexpr const char* const kRadonDataRemovals = "removals";
static constexpr const char* const kRadonDataUpdateFrom = "updateFrom";
static constexpr const char* const kRadonDataUpdateTo = "updateTo";
static constexpr const char* const kRadonDataMoveFrom = "moveFrom";
static constexpr const char* const kRadonDataMoveTo = "moveTo";
static constexpr const char* const kRadonDataEstimatedHeightPx =
    "estimatedHeightPx";
static constexpr const char* const kRadonDataEstimatedMainAxisSizePx =
    "estimatedMainAxisSizePx";
static constexpr const char* const kRadonDataFullSpan = "fullspan";
static constexpr const char* const kRadonDataStickyTop = "stickyTop";
static constexpr const char* const kRadonDataStickyBottom = "stickyBottom";
static constexpr const char* const kRadonDataItemKeys = "itemkeys";

// fiber arch data
static constexpr const char* const kFiberDataInsertAction = "insertAction";
static constexpr const char* const kFiberDataRemoveAction = "removeAction";
static constexpr const char* const kFiberDataUpdateAction = "updateAction";
static constexpr const char* const kFiberDataPosition = "position";
static constexpr const char* const kFiberDataItemKey = "item-key";
static constexpr const char* const kFiberDataFullSpan = "full-span";
static constexpr const char* const kFiberDataStickyTop = "sticky-top";
static constexpr const char* const kFiberDataStickyBottom = "sticky-bottom";
static constexpr const char* const kFiberDataEstimatedHeightPx =
    "estimated-height-px";
static constexpr const char* const kFiberDataEstimatedMainAxisSizePx =
    "estimated-main-axis-size-px";
static constexpr const char* const kFiberDataRecyclable = "recyclable";
static constexpr const char* const kFiberDataFrom = "from";
static constexpr const char* const kFiberDataTo = "to";
static constexpr const char* const kFiberDataFlush = "flush";

// constant value
static constexpr int kInvalidIndex = -1;
static constexpr int kInvalidItemCount = -1;
static constexpr int kInvalidDimensionSize = -1.f;
static constexpr int kStickyItemSetCapacityForSyncMode = 1;
static constexpr int kStickyItemSetCapacityForASyncMode = 2;
static constexpr int kDefaultMainAxisItemSize = 200;

enum class LayoutDirection : int32_t {
  kLayoutToStart = -1,
  kLayoutToEnd = 1,
};

enum class FrameDirection : uint32_t {
  kLeft = 0,
  kTop,
  kRight,
  kBottom,
};

enum class LayoutType {
  kSingle = 0,
  kFlow,
  kWaterFall,
};

enum class Orientation { kHorizontal = 0, kVertical };

enum class Direction { kNormal = 0, kRTL };

enum class InitialScrollIndexStatus {
  kUnset = 0,
  kSet,
  kScrolled,
};

enum class DiffStatus {
  kValid = 0,
  kRemoved,
  kUpdateTo,
  kUpdatedFrom,
  kMoveTo,
  kMoveFrom
};

enum class AnchorVisibility {
  kAnchorVisibilityNoAdjustment,
  kAnchorVisibilityShow,
  kAnchorVisibilityHide
};

enum class ScrollingInfoAlignment { kTop, kMiddle, kBottom };

enum class EventSource {
  kDiff = 0,
  kLayout,
  kScroll,
};

enum class ScrollPositionState {
  kMiddle = 0,  // not upper and not lower
  kUpper,
  kLower,
  kBothEdge,  // on upper and lower
};

enum class BatchRenderStrategy {
  kDefault = 0,
  kBatchRender = 1,
  kAsyncResolveProperty = 2,
  kAsyncResolvePropertyAndElementTree = 3
};

enum class ScrollState {
  kNone = 0,
  kIdle,
  kDragging,
  kFling,
  kScrollAnimation
};

enum class ListDebugInfoLevel {
  kListDebugInfoLevelNone = 0,
  kListDebugInfoLevelError,
  kListDebugInfoLevelInfo,
  kListDebugInfoLevelVerbose,
};

enum class ListTiming {
  kRenderChildrenStart = 0,
  kRenderChildrenEnd,
  kFullFillRenderChildrenEnd
};

enum class SearchRefAnchorStrategy {
  kNone = 0,
  kToStart,
  kToEnd,
};

enum class ItemHolderAnimationType { kNone, kTransform, kOpacity };

enum class ListAnimationType { kNone, kRemove, kInsert, kUpdate };

enum class ListAdapterDiffResult {
  kNone = 0,
  kMove = 1,
  kInsert = 1 << 1,
  kRemove = 1 << 2,
  kUpdate = 1 << 3,
};

constexpr inline ListAdapterDiffResult operator|(ListAdapterDiffResult a,
                                                 ListAdapterDiffResult b) {
  return static_cast<ListAdapterDiffResult>(static_cast<unsigned int>(a) |
                                            static_cast<unsigned int>(b));
}

constexpr inline ListAdapterDiffResult operator&(ListAdapterDiffResult a,
                                                 ListAdapterDiffResult b) {
  return static_cast<ListAdapterDiffResult>(static_cast<unsigned int>(a) &
                                            static_cast<unsigned int>(b));
}

constexpr inline ListAdapterDiffResult& operator|=(ListAdapterDiffResult& a,
                                                   ListAdapterDiffResult b) {
  a = a | b;
  return a;
}

constexpr inline ListAdapterDiffResult& operator&=(ListAdapterDiffResult& a,
                                                   ListAdapterDiffResult b) {
  a = a & b;
  return a;
}

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_TYPES_H_
