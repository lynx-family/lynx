// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_wrapper.h"

#include <math.h>

#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_view.h"
#include "clay/ui/component/scrollbar/scrollbar_orientation_helper.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {
namespace {

const std::unordered_set<KeywordID> kProxyAttributes = {
    KeywordID::kListType,
    KeywordID::kColumnCount,
    KeywordID::kLowerThreshold,
    KeywordID::kUpperThreshold,
    KeywordID::kLowerThresholdItemCount,
    KeywordID::kUpperThresholdItemCount,
    KeywordID::kScrollEventThrottle,
    KeywordID::kEnableScroll,
    KeywordID::kEnableNestedScroll,
    KeywordID::kNeedsVisibleCells,
    KeywordID::kUpdateAnimation,
    KeywordID::kAlignFocus,
    KeywordID::kSticky,
    KeywordID::kFocusSmoothScroll,
    KeywordID::kScrollWithoutFocus,
    KeywordID::kScrollDirection,
    KeywordID::kUpdateListInfo,
    KeywordID::kListCrossAxisGap,
    KeywordID::kListMainAxisGap,
    KeywordID::kVerticalOrientation,
    KeywordID::kScrollOrientation,
    KeywordID::kBounce,
    KeywordID::kBounces,
    KeywordID::kInitialScrollIndex,
    KeywordID::kStickyOffset,
};

LYNX_UI_METHOD_BEGIN(ListWrapper) {
  LYNX_UI_METHOD(ListWrapper, scrollToPosition);
  LYNX_UI_METHOD(ListWrapper, getVisibleItemsPositions);
  LYNX_UI_METHOD(ListWrapper, getVisibleCells);
}
LYNX_UI_METHOD_END(ListWrapper);
}  // namespace

ListWrapper::ListWrapper(BaseListView* inner_view, int32_t id, std::string tag,
                         PageView* page_view)
    : WithTypeInfo(id, ScrollDirection::kVertical, std::move(tag), page_view) {
  inner_view->SetDelegate(this);
  inner_view->AddScrollListener(this);

  view_ = inner_view;
  view_->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  view_->SetRepaintBoundary(true);
  BaseView::AddChild(view_, 0);
}

ListWrapper::ListWrapper(int32_t id, PageView* page_view)
    : WithTypeInfo(id, ScrollDirection::kVertical, "list", page_view) {
  BaseListView* inner_view = new ListView(-1, id, page_view);
  inner_view->SetDelegate(this);
  inner_view->AddScrollListener(this);

  view_ = inner_view;
  view_->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  view_->SetRepaintBoundary(true);
  BaseView::AddChild(view_, 0);
}

ListWrapper::~ListWrapper() {}

void ListWrapper::OnDestroy() {
  ScrollbarWrapper::OnDestroy();
  static_cast<BaseListView*>(view_)->SetDelegate(nullptr);
  static_cast<BaseListView*>(view_)->RemoveScrollListener(this);
}

void ListWrapper::OnLayout(LayoutContext* context) {
  if (view_->NeedsLayout()) {
    view_->Layout(context);
  }
}

void ListWrapper::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kProxyAttributes.find(kw) != kProxyAttributes.end()) {
    view_->SetAttribute(attr_c, value);
  } else {
    ScrollbarWrapper::SetAttribute(attr_c, value);
  }
}

void ListWrapper::DidUpdateAttributes() {
  BaseView::DidUpdateAttributes();
  view_->DidUpdateAttributes();
}

void ListWrapper::scrollToPosition(const LynxModuleValues& args,
                                   const LynxUIMethodCallback& callback) {
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
    FML_DLOG(ERROR)
        << "scrollToPosition error! offset or position/index is invalid!";
    if (callback) {
      callback(LynxUIMethodResult::kParamInvalid, clay::Value());
    }
    return;
  }

  AlignTo align_to = ListScroller::StringToAlign(align_to_str);
  GetListView()->ScrollToPosition(
      smooth, index == -1 ? position : index, FromLogical(offset), align_to, id,
      [callback](uint32_t result, const std::string&) {
        if (callback) {
          callback(static_cast<LynxUIMethodResult>(result), clay::Value());
        }
      });
}

void ListWrapper::getVisibleItemsPositions(
    const LynxModuleValues&, const LynxUIMethodCallback& callback) {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  size_t visible_count = 0;
  auto* layout_manager = GetListView()->GetLayoutManager();
  visible_count = layout_manager->GetVisibleItemsInfo(top_array, bottom_array,
                                                      left_array, right_array,
                                                      position_array, id_array);

  clay::Value::Array visible_position_array(visible_count);
  for (size_t i = 0; i < visible_count; ++i) {
    visible_position_array[i] = clay::Value(position_array[i]);
  }
  callback(LynxUIMethodResult::kSuccess,
           clay::Value(std::move(visible_position_array)));
}

void ListWrapper::getVisibleCells(const LynxModuleValues&,
                                  const LynxUIMethodCallback& callback) {
  std::vector<float> lefts, rights, tops, bottoms;
  std::vector<int> positions;
  std::vector<std::string> ids;
  auto* layout_manager = GetListView()->GetLayoutManager();
  size_t visible_count = layout_manager->GetVisibleItemsInfo(
      tops, bottoms, lefts, rights, positions, ids);

  clay::Value::Array cells(visible_count);
  for (size_t i = 0; i < visible_count; ++i) {
    clay::Value::Map cell;
    cell["id"] = clay::Value(ids[i]);
    cell["position"] = clay::Value(positions[i]);
    cell["top"] = clay::Value(tops[i]);
    cell["bottom"] = clay::Value(bottoms[i]);
    cell["left"] = clay::Value(lefts[i]);
    cell["right"] = clay::Value(rights[i]);
    cells[i] = clay::Value(std::move(cell));
  }
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(cells)));
}

void ListWrapper::WillUpdateScrollbar() {}

float ListWrapper::GetScrollbarScrollOffset() {
  return GetListView()->GetScrollbarScrollOffset();
}

float ListWrapper::GetTotalLength() {
  return GetListView()->GetTotalScrollLength();
}

void ListWrapper::OnScrollableScrolled() {
  UpdateScrollbarIfNeeded();
  scrollbar_->NotifyScrollViewScrolled();
}

void ListWrapper::OnListViewDidLayout() { UpdateScrollbarIfNeeded(); }

void ListWrapper::UpdateScrollbarIfNeeded() {
  if (scrollbar_enabled_) {
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

// Override ScrollbarView::Delegate
void ListWrapper::OnScrollbarScrolled(float old_position, float new_position,
                                      bool by_interaction, bool smooth) {
  // Note: smooth scroll for listview's scrollbar is not implemented
  if (by_interaction) {
    const float delta =
        (new_position - old_position) *
        (scrollbar_->GetTotalLength() - scrollbar_->GetVisibleLength());
    if (OrientationHelper().GetDirection() == ScrollDirection::kVertical) {
      GetListView()->OnScrollBy({0.f, -delta});
    } else {
      GetListView()->OnScrollBy({-delta, 0.f});
    }
  }
}

#ifdef ENABLE_ACCESSIBILITY
int32_t ListWrapper::GetSemanticsActions() const {
  return GetListView()->GetSemanticsActions();
}

int32_t ListWrapper::GetSemanticsFlags() const {
  return GetListView()->GetSemanticsFlags();
}

int32_t ListWrapper::GetA11yScrollChildren() const {
  return GetListView()->GetA11yScrollChildren();
}

bool ListWrapper::IsAccessibilityElement() const {
  bool result = BaseView::IsAccessibilityElement();
  if (!result) {
    for (auto child : GetListView()->GetChildren()) {
      if (child->IsAccessibilityElement()) {
        result = true;
        break;
      }
    }
  }
  return result;
}

FloatRect ListWrapper::GetSemanticsBounds() const {
  return GetListView()->GetSemanticsBounds();
}
#endif
}  // namespace clay
