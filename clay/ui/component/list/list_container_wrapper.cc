// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_container_wrapper.h"

#include <math.h>

#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_scroller.h"
#include "clay/ui/component/scrollbar/scrollbar_orientation_helper.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {
namespace {

const std::unordered_set<KeywordID> kProxyAttributes = {
    KeywordID::kSticky,
    KeywordID::kStickyOffset,
    KeywordID::kExperimentalRecycleStickyItem,
    KeywordID::kExperimentalBatchRenderStrategy,
    KeywordID::kExperimentalUpdateStickyForDiff,
    KeywordID::kListContainerInfo,
    KeywordID::kScrollX,
    KeywordID::kScrollY,
    KeywordID::kLowerThreshold,
    KeywordID::kUpperThreshold,
    KeywordID::kEnableScroll,
    KeywordID::kEnableNestedScroll,
    KeywordID::kScrollTop,
    KeywordID::kScrollLeft,
    KeywordID::kScrollToIndex,
    KeywordID::kDirection,
    KeywordID::kInitialScrollOffset,
    KeywordID::kInitialScrollIndex,
    KeywordID::kScrollOrientation,
    KeywordID::kScrollForwardMode,
    KeywordID::kScrollBackwardMode,
    KeywordID::kPrevScrollable,
    KeywordID::kNextScrollable,
    KeywordID::kBounce,
    KeywordID::kBounces,
    KeywordID::kScrollToId,
    KeywordID::kScrollEventThrottle};
constexpr char kListContainerWrapperTag[] = "list-container-wrapper";

LYNX_UI_METHOD_BEGIN(ListContainerWrapper) {
  LYNX_UI_METHOD(ListContainerWrapper, scrollToPosition);
  LYNX_UI_METHOD(ListContainerWrapper, getVisibleItemsPositions);
  LYNX_UI_METHOD(ListContainerWrapper, getVisibleCells);
  LYNX_UI_METHOD(ListContainerWrapper, getScrollInfo);
}
LYNX_UI_METHOD_END(ListContainerWrapper);
}  // namespace

ListContainerWrapper::ListContainerWrapper(int32_t id, PageView* page_view)
    : WithTypeInfo(id, ScrollDirection::kVertical, kListContainerWrapperTag,
                   page_view) {
  view_ = new ListContainerView(-1, page_view);
  view_->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  view_->SetRepaintBoundary(true);
  GetListContainerView()->SetDelegate(this);
  GetListContainerView()->AddScrollListener(this);
  BaseView::AddChild(view_, 0);
}

void ListContainerWrapper::OnScrollByListContainer(float offset_x,
                                                   float offset_y,
                                                   float original_x,
                                                   float original_y) {
  if (auto delegate = page_view_->GetUIComponentDelegate()) {
    delegate->OnScrollByListContainer(id_, offset_x, offset_y, original_x,
                                      original_y);
  }
}

void ListContainerWrapper::OnScrollStopped() {
  if (auto delegate = page_view_->GetUIComponentDelegate()) {
    delegate->OnScrollStopped(id_);
  }
}

void ListContainerWrapper::OnListViewDidLayout() { UpdateScrollbarIfNeeded(); }

void ListContainerWrapper::OnDestroy() {
  ScrollbarWrapper::OnDestroy();
  GetListContainerView()->SetDelegate(nullptr);
  GetListContainerView()->RemoveScrollListener(this);
}

void ListContainerWrapper::scrollToPosition(
    const LynxModuleValues& args, const LynxUIMethodCallback& callback) {
  bool smooth = false;
  int position = -1;
  int index = -1;
  float offset = 0;
  std::string align_to_str;
  std::string id;
  CastNamedLynxModuleArgs(
      {"smooth", "position", "index", "offset", "alignTo", "id"}, args, smooth,
      position, index, offset, align_to_str, id);
  if (isnan(offset) || isinf(offset) || (index == -1 && position == -1)) {
    if (callback) {
      callback(LynxUIMethodResult::kParamInvalid, clay::Value());
    }
    return;
  }

  AlignTo align_to = ListScroller::StringToAlign(align_to_str);
  GetListContainerView()->ScrollToPosition(
      smooth, index == -1 ? position : index, FromLogical(offset), align_to, id,
      [callback](uint32_t result, const std::string&) {
        if (callback) {
          callback(static_cast<LynxUIMethodResult>(result), clay::Value());
        }
      });
}

void ListContainerWrapper::UpdateContentOffsetForListContainer(
    float content_size, float target_content_offset_x,
    float target_content_offset_y) {
  GetListContainerView()->UpdateContentOffsetForListContainer(
      content_size, target_content_offset_x, target_content_offset_y);
}

void ListContainerWrapper::InsertListItemPaintingNode(BaseView* view) {
  GetListContainerView()->InsertListItemPaintingNode(view);
}

void ListContainerWrapper::RemoveListItemPaintingNode(BaseView* view) {
  GetListContainerView()->RemoveListItemPaintingNode(view);
}

void ListContainerWrapper::SetAttribute(const char* attr_c,
                                        const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kProxyAttributes.find(kw) != kProxyAttributes.end()) {
    view_->SetAttribute(attr_c, value);
    if (kw == KeywordID::kScrollX || kw == KeywordID::kScrollY ||
        kw == KeywordID::kScrollOrientation ||
        kw == KeywordID::kScrollDirection) {
      scrollbar_->SetScrollDirection(
          GetListContainerView()->GetScrollDirection());
    }
  } else {
    ScrollbarWrapper::SetAttribute(attr_c, value);
  }
}

void ListContainerWrapper::DidUpdateAttributes() {
  BaseView::DidUpdateAttributes();
  view_->DidUpdateAttributes();
}

void ListContainerWrapper::OnNodeReady() {
  GetListContainerView()->OnNodeReady();
}

void ListContainerWrapper::OnScrollToPosition(int position, float offset,
                                              int align, bool smooth) {
  if (auto delegate = page_view_->GetUIComponentDelegate()) {
    delegate->OnScrollToPosition(id_, position, offset, align, smooth);
  }
}

void ListContainerWrapper::getVisibleItemsPositions(
    const LynxModuleValues&, const LynxUIMethodCallback& callback) {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  std::vector<std::string> item_key_array;
  size_t visible_count = 0;
  visible_count = GetListContainerView()->GetVisibleItemsInfo(
      top_array, bottom_array, left_array, right_array, position_array,
      id_array, item_key_array);

  clay::Value::Array visible_position_array(visible_count);
  for (size_t i = 0; i < visible_count; ++i) {
    visible_position_array[i] = clay::Value(position_array[i]);
  }
  callback(LynxUIMethodResult::kSuccess,
           clay::Value(std::move(visible_position_array)));
}

void ListContainerWrapper::getScrollInfo(const LynxModuleValues& args,
                                         const LynxUIMethodCallback& callback) {
  if (callback) {
    FloatSize offset = view_->GetScrollOffset();
    FloatSize zoomed_content = page_view_->ConvertTo<kPixelTypeLogical>(
        FloatSize(view_->ContentWidth(), view_->ContentHeight()));
    FloatSize zoomed_offset = page_view_->ConvertTo<kPixelTypeLogical>(offset);
    clay::Value::Map map;
    map.emplace("scrollTop", zoomed_offset.height());
    map.emplace("scrollLeft", zoomed_offset.width());
    map.emplace("scrollHeight", zoomed_content.height());
    map.emplace("scrollWidth", zoomed_content.width());
    map.emplace("isDragging",
                static_cast<ScrollView*>(view_)->GetScrollStatus() ==
                    ScrollView::ScrollStatus::kDragging);
    callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(map)));
  }
}

void ListContainerWrapper::getVisibleCells(
    const LynxModuleValues&, const LynxUIMethodCallback& callback) {
  auto cells = GetListContainerView()->GetVisibleCells();
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(cells)));
}

void ListContainerWrapper::WillUpdateScrollbar() {}

void ListContainerWrapper::OnLayoutFinish(clay::BaseView* view) {
  view_->OnLayoutFinish(view);
}

void ListContainerWrapper::UpdateScrollInfo(bool smooth, float estimated_offset,
                                            bool scrolling) {
  GetListContainerView()->UpdateScrollInfo(smooth, estimated_offset, scrolling);
}

float ListContainerWrapper::GetScrollbarScrollOffset() {
  return OrientationHelper().GetLength(view_->GetScrollOffset());
}

float ListContainerWrapper::GetTotalLength() {
  return GetListContainerView()->max_content_;
}

void ListContainerWrapper::OnScrollableScrolled() {
  UpdateScrollbarIfNeeded();
  scrollbar_->NotifyScrollViewScrolled();
}

void ListContainerWrapper::UpdateScrollbarIfNeeded() {
  if (scrollbar_enabled_) {
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

// Override ScrollbarView::Delegate
void ListContainerWrapper::OnScrollbarScrolled(float old_position,
                                               float new_position,
                                               bool by_interaction,
                                               bool smooth) {
  // Note: smooth scroll for listview's scrollbar is not implemented
  if (by_interaction) {
    const float delta =
        (new_position - old_position) *
        (scrollbar_->GetTotalLength() - scrollbar_->GetVisibleLength());
    GetListContainerView()->ScrollWithDelta(false, delta);
  }
}
}  // namespace clay
