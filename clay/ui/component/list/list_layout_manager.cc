// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_layout_manager.h"

#include <algorithm>
#include <optional>

#include "clay/fml/logging.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager_grid.h"
#include "clay/ui/component/list/list_layout_manager_linear.h"
#include "clay/ui/component/list/list_layout_manager_staggered_grid.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

ListLayoutManager::ListLayoutManager(ScrollDirection orientation) {
  SetOrientation(orientation);
  length_cache_ = std::make_unique<ListItemLengthCache>();
}

ListLayoutManager::~ListLayoutManager() = default;

// static
std::unique_ptr<ListLayoutManager> ListLayoutManager::Create(
    const std::string& type, int span_count) {
  // NOTE(hanhaoshen): Focus and `scroll_to_position` are not fully supported on
  // grid / staggered grid.
  if (type == attr_value::kListTypeSingle) {
    return std::make_unique<ListLayoutManagerLinear>();
  } else if (type == attr_value::kListTypeFlow) {
    return std::make_unique<ListLayoutManagerGrid>(span_count);
  } else if (type == attr_value::kListTypeWaterFall) {
    return std::make_unique<ListLayoutManagerStaggeredGrid>(span_count);
  }
  FML_DLOG(ERROR) << "Invalid list layout type " << type
                  << ". Set layout type to linear.";
  return std::make_unique<ListLayoutManagerLinear>();
}

void ListLayoutManager::SetListView(BaseListView* list_view) {
  list_view_ = list_view;
  FML_DCHECK(list_view_);
}

void ListLayoutManager::SetChildrenHelper(ListChildrenHelper* children_helper) {
  children_helper_ = children_helper;
  FML_DCHECK(children_helper_);
}

float ListLayoutManager::GetWidth() {
  return list_view_->Width() - list_view_->BorderLeft() -
         list_view_->BorderRight();
}

float ListLayoutManager::GetHeight() {
  return list_view_->Height() - list_view_->BorderTop() -
         list_view_->BorderBottom();
}

float ListLayoutManager::GetPaddingLeft() { return list_view_->PaddingLeft(); }

float ListLayoutManager::GetPaddingRight() {
  return list_view_->PaddingRight();
}

float ListLayoutManager::GetPaddingTop() { return list_view_->PaddingTop(); }

float ListLayoutManager::GetPaddingBottom() {
  return list_view_->PaddingBottom();
}

// This is similar to `RemoveAndScrapChildren` but for floating items
void ListLayoutManager::ScrapFloatingItems(ListRecycler* recycler) {
  if (auto item = children_helper_->floating_top_item()) {
    item->ClearAdjustedLocation();
    recycler->ScrapItem(item);
    children_helper_->set_floating_top_item(nullptr);
  }
  if (auto item = children_helper_->floating_bottom_item()) {
    item->ClearAdjustedLocation();
    recycler->ScrapItem(item);
    children_helper_->set_floating_bottom_item(nullptr);
  }
}

void ListLayoutManager::LayoutSticky(ListRecycler* recycler,
                                     ListAdapter* adapter) {
  if (children_helper_->GetChildCount() == 0) {
    return;
  }

  {
    int header_position = ListItemViewHolder::kNoPosition;
    int header_index = -1;

    // find any possible header in visible area.
    for (int i = GetChildCount() - 1; i >= 0; i--) {
      if (adapter->IsItemStickyTop(GetChildAt(i)->GetPosition())) {
        ListItemViewHolder* item = GetChildAt(i);
        if (item->GetTop() < sticky_offset_) {
          header_position = item->GetPosition();
          header_index = i;
        }
      }
    }

    // find any possible header in invisible area.
    if (header_position == ListItemViewHolder::kNoPosition) {
      header_position =
          adapter->FindPreviousFullSpan(GetFirstChild()->GetPosition());
    }

    if (!adapter->IsItemStickyTop(header_position)) {
      header_position = ListItemViewHolder::kNoPosition;
    }

    if (header_position != ListItemViewHolder::kNoPosition) {
      ListItemViewHolder* header_item = nullptr;
      if (header_index != -1) {
        header_item = GetChildAt(header_index);
      } else {
        // If the header item is not visible, then it's a floating item. We need
        // to retrieve and add it to list view.
        header_item = recycler->GetItemForPosition(header_position);
        header_item->Measure(FullSpanConstraint());
        header_item->Layout({});
        list_view_->OnAddItem(header_item, list_view_->child_count());
        children_helper_->set_floating_top_item(header_item);
      }

      // Adjust the location to make it sticky at the top of list.
      float sticky_top_offset = sticky_offset_;
      if (auto next_fullspan = children_helper_->FindChildByPosition(
              adapter->FindNextFullSpan(header_position + 1))) {
        // Handle the case of it being pushed up by the next full-span item.
        sticky_top_offset =
            std::min(sticky_top_offset, next_fullspan->GetTop() -
                                            header_item->GetView()->Height());
      }
      header_item->SetAdjustedLocation(sticky_top_offset);
      header_item->FlushLayout();
    }
  }
  {
    // Find the current footer item
    int footer_position = ListItemViewHolder::kNoPosition;
    int footer_index = -1;

    // find any possible footer in visible area.
    for (int i = 0; i < GetChildCount(); i++) {
      if (adapter->IsItemStickyBottom(GetChildAt(i)->GetPosition())) {
        ListItemViewHolder* item = GetChildAt(i);
        if (item->GetBottom() + sticky_offset_ > GetHeight()) {
          footer_position = item->GetPosition();
          footer_index = i;
        }
      }
    }

    // find any possible header in invisible area.
    if (footer_position == ListItemViewHolder::kNoPosition) {
      footer_position =
          adapter->FindNextFullSpan(GetLastChild()->GetPosition());
    }

    if (!adapter->IsItemStickyBottom(footer_position)) {
      footer_position = ListItemViewHolder::kNoPosition;
    }

    if (footer_position != ListItemViewHolder::kNoPosition) {
      ListItemViewHolder* footer_item = nullptr;
      if (footer_index != -1) {
        footer_item = GetChildAt(footer_index);
      } else {
        // If the footer item is not visible, then it's a floating item. We
        // need to retrieve and add it to list view.
        footer_item = recycler->GetItemForPosition(footer_position);
        footer_item->Measure(FullSpanConstraint());
        footer_item->Layout({});
        list_view_->OnAddItem(footer_item, list_view_->child_count());
        children_helper_->set_floating_bottom_item(footer_item);
      }

      // Adjust the location to make it sticky at the bottom of list.
      float sticky_bottom_offset = sticky_offset_;
      if (auto prev_fullspan = children_helper_->FindChildByPosition(
              adapter->FindPreviousFullSpan(footer_position - 1))) {
        // Handle the case of it being pushed down by the previous full-span
        // item.
        sticky_bottom_offset = std::min(
            sticky_bottom_offset, GetHeight() - prev_fullspan->GetBottom() -
                                      footer_item->GetView()->Height());
      }
      footer_item->SetAdjustedLocation(GetHeight() -
                                       footer_item->GetView()->Height() -
                                       sticky_bottom_offset);
      footer_item->FlushLayout();
    }
  }
}

void ListLayoutManager::SetStickyOffset(double offset) {
  if (sticky_offset_ != offset) {
    sticky_offset_ = offset;
    RequestLayout();
  }
}

void ListLayoutManager::OffsetChildren(float x, float y) {
  LIST_LOG << "OffsetChildren: " << x << ", " << y;
  list_view_->OffsetChildren(x, y);
}

bool ListLayoutManager::SetOrientation(ScrollDirection orientation) {
  if (orientation_ == orientation && orientation_helper_ != nullptr) {
    return false;
  }

  orientation_ = orientation;
  orientation_helper_ =
      ListOrientationHelper::CreateOrientationHelper(this, orientation);
  return true;
}

void ListLayoutManager::SetCrossAxisGap(float cross_axis_gap) {
  if (cross_axis_gap != cross_axis_gap_) {
    cross_axis_gap_ = std::max(cross_axis_gap, 0.f);
    RequestLayout();
  }
}

void ListLayoutManager::SetMainAxisGap(float main_axis_gap) {
  if (main_axis_gap != main_axis_gap_) {
    main_axis_gap_ = std::max(main_axis_gap, 0.f);
    RequestLayout();
  }
}

void ListLayoutManager::RequestLayout() {
  if (list_view_) {
    list_view_->MarkNeedsLayout();
  }
}

void ListLayoutManager::AddItem(ListItemViewHolder* item, int index) {
  BaseView::LayoutIgnoreHelper helper(list_view_);
  children_helper_->AddChild(item, index);
  list_view_->OnAddItem(item, index);
}

MeasureResult ListLayoutManager::MeasureItem(
    ListItemViewHolder* item, const MeasureConstraint& constraint) {
  MeasureResult res = item->Measure(constraint);
  if (orientation_ == ScrollDirection::kVertical) {
    length_cache_->SetLength(item->GetPosition(), res.height);
  } else {
    length_cache_->SetLength(item->GetPosition(), res.width);
  }
  return res;
}

void ListLayoutManager::LayoutItem(ListItemViewHolder* item,
                                   const FloatPoint& top_left) {
  if (item->FullSpan()) {
    FloatPoint new_location =
        orientation_helper_->CalculateFullSpanLocation(top_left, item);
    item->Layout(new_location);
  } else {
    item->Layout(top_left);
  }
}

int ListLayoutManager::GetChildCount() const {
  return children_helper_->GetChildCount();
}

ListItemViewHolder* ListLayoutManager::GetChildAt(int index) {
  return children_helper_->GetChildAt(index);
}

ListItemViewHolder* ListLayoutManager::FindChildByPosition(int position) {
  return children_helper_->FindChildByPosition(position);
}

void ListLayoutManager::UpdateItemsVisibility() {
  const float start = 0.f;
  const float end = orientation_helper_->GetEnd();
  children_helper_->ForEachWithSticky(
      [this, start, end](ListItemViewHolder* item) {
        FML_DCHECK(item->GetViewAttached());
        const float item_start = orientation_helper_->GetDecoratedStart(item);
        const float item_end = orientation_helper_->GetDecoratedEnd(item);
        item->SetViewVisible(item_end > start && item_start < end);
        return false;
      });
}

void ListLayoutManager::RemoveAndRecycleAllViews(ListRecycler& recycler) {
  const int child_count = GetChildCount();
  for (int i = 0; i < child_count; ++i) {
    RemoveAndRecycleViewAt(recycler, 0);
  }
}

void ListLayoutManager::RemoveAndRecycleViewAt(ListRecycler& recycler,
                                               int index) {
  ListItemViewHolder* child = children_helper_->RemoveChild(index);
  list_view_->OnRemoveItem(child);
  recycler.RecycleItem(child);
}

void ListLayoutManager::RemoveAndScrapChildren(ListRecycler& recycler) {
  const int child_count = GetChildCount();
  for (int i = 0; i < child_count; ++i) {
    RemoveAndScrapChild(recycler, 0);
  }
}

void ListLayoutManager::RemoveAndScrapChild(ListRecycler& recycler, int index) {
  // remove view_holder from `children_helper_`.
  ListItemViewHolder* child = children_helper_->RemoveChild(index);
  if (child->IsInvalid() && !child->IsRemoved()) {
    LIST_LOG << "Remove child: " << child->ToString();
    // remove view_holder'view from layout tree and render tree.
    list_view_->OnRemoveItem(child);
    // move view_holder flagged as removed to 2nd layer cache.
    recycler.RecycleItem(child);
  } else {
    LIST_LOG << "Scrap child: " << child->ToString();
    // NOTE: We don't remove the item here, unlike Android's RecyclerView.
    // Because in clay it is costly to remove and re-add the item's view
    // (which would mark all items to be repainted). We will remove items that
    // are no longer displayed at the end of the layout.

    // move view_holder to 1st layer cache.
    recycler.ScrapItem(child);
  }
}

void ListLayoutManager::OnLayoutCompleted(ListRecycler& recycler,
                                          const ListViewState& state) {
  recycler.RecycleScrappedItems(list_view_);
}

bool ListLayoutManager::IsItemFullSpan(int position) const {
  return list_view_->IsItemFullSpan(position);
}

size_t ListLayoutManager::GetVisibleItemsInfo(
    std::vector<float>& top_array, std::vector<float>& bottom_array,
    std::vector<float>& left_array, std::vector<float>& right_array,
    std::vector<int>& position, std::vector<std::string>& id_array) {
  position = GetVisibleItemPositions();
  if (children_helper_->floating_top_item()) {
    position.push_back(children_helper_->floating_top_item()->GetPosition());
  }
  if (children_helper_->floating_bottom_item()) {
    position.push_back(children_helper_->floating_bottom_item()->GetPosition());
  }

  // Sort and erase duplicated item
  std::sort(position.begin(), position.end());
  auto last = std::unique(position.begin(), position.end());
  position.erase(last, position.end());

  children_helper_->ForEachInPositions(
      position,
      [page_view = list_view_->page_view(), &top_array, &bottom_array,
       &left_array, &right_array, &id_array](ListItemViewHolder* item) {
        if (item == nullptr) {
          return false;
        }
        top_array.push_back(
            page_view->ConvertTo<kPixelTypeLogical>(item->GetTop()));
        bottom_array.push_back(
            page_view->ConvertTo<kPixelTypeLogical>(item->GetBottom()));
        left_array.push_back(
            page_view->ConvertTo<kPixelTypeLogical>(item->GetLeft()));
        right_array.push_back(
            page_view->ConvertTo<kPixelTypeLogical>(item->GetRight()));
        id_array.push_back(item->GetView()->GetIdSelector());

        return false;
      });

  LIST_LOG << "Get " << top_array.size() << " VisibleItemsInfo ";
  return top_array.size();
}

MeasureConstraint ListLayoutManager::FullSpanConstraint() {
  MeasureConstraint constraint;
  if (orientation_ == ScrollDirection::kVertical) {
    constraint.width_mode = MeasureMode::kDefinite;
    constraint.height_mode = MeasureMode::kIndefinite;
    constraint.width = GetWidth();
  } else {
    constraint.height_mode = MeasureMode::kDefinite;
    constraint.width_mode = MeasureMode::kIndefinite;
    constraint.height = GetHeight();
  }
  return constraint;
}

}  // namespace clay
