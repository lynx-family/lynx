// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_adapter.h"

#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/macros.h"

namespace {
int64_t CalculateAverage(int64_t old_average, int64_t new_value) {
  if (old_average == 0) {
    return new_value;
  }
  return (old_average * 3 / 4) + (new_value / 4);
}
}  // namespace

namespace clay {

ListAdapter::Observer::Observer() = default;
ListAdapter::Observer::~Observer() = default;

ListAdapter::ListAdapter(BaseListView* list_view) : list_view_(list_view) {
  FML_DCHECK(list_view_);
}
ListAdapter::~ListAdapter() = default;

ListItemViewHolder* ListAdapter::CreateListItem(TypeId type) {
  TRACE_EVENT("clay", "ListAdapter::CreateListItem");
  ListItemViewHolder* res = OnCreateListItem(type);
  FML_DCHECK(res);
  res->SetVisibilityObserver(list_view_);

  // Delay the creation of view to match the behaviour of Lynx.
  return res;
}

// The list item is not recycled and should be deleted.
void ListAdapter::DeleteListItem(ListItemViewHolder* view_holder) {
  // We won't get here in new arch mode because there's no limit to the size of
  // recycling cache.
  FML_DCHECK(!IsNewArch());

  list_view_->OnDestroyItem(view_holder);
  OnDeleteListItem(view_holder);
}

void ListAdapter::BindListItem(ListItemViewHolder* item, int index) {
  TRACE_EVENT("LIST", "ListAdapter::BindListItem");
  FML_DCHECK(item);
  int prev_position = item->GetPosition();

  // NOTE(Xietong): this should be done before setting the new position.
  if (prev_position != index && item->GetView()) {
    // Mark the original view to invisible before assigning to a new position.
    item->SetViewVisible(false);
  }

  item->SetPosition(index);
  // TODO(Xietong): cleanup the set flag logic.
  item->SetFlags(
      ListItemViewHolder::kFlagBound,
      static_cast<ListItemViewHolder::Flag>(
          ListItemViewHolder::kFlagBound | ListItemViewHolder::kFlagUpdate |
          ListItemViewHolder::kFlagInvalid |
          ListItemViewHolder::kFlagAdapterPositionUnknown));
  // When the new architecture of Lynx is enabled, the creation logic is handled
  // in LynxListAdapter::OnBindListItem
  if (!IsNewArch() && !item->GetView()) {
    list_view_->OnCreateItem(item, index);
    FML_DCHECK(item->GetView());

    // Set the initial visibility to false because the just created item is not
    // attached yet.
    // We don't call `SetViewVisible()` here because we don't want to send
    // NodeDisappear() notification for a newly created item.
    item->GetView()->SetVisible(false);

    OnBindListItem(item, ListItemViewHolder::kNoPosition, index, true);
    // Notify the list view so that list view can update the focus information.
    list_view_->OnBindListItem(item, ListItemViewHolder::kNoPosition, index,
                               true);
  } else {
    OnBindListItem(item, prev_position, index, false);
    // Notify the list view so that list view can update the focus information.
    list_view_->OnBindListItem(item, prev_position, index, false);
  }
  item->ClearPayloads();
}

void ListAdapter::OnRecycleItem(ListItemViewHolder* item) {
  FML_DCHECK(item->GetView());
  item->SetViewVisible(false);
}

void ListAdapter::UpdateBindTime(TypeId type, fml::TimeDelta duration) {
  PerformData& perform_data = id_to_perform_data_[type];
  perform_data.average_bind_time_ns = CalculateAverage(
      perform_data.average_bind_time_ns, duration.ToNanoseconds());
}

fml::TimeDelta ListAdapter::GetAverageBindTime(TypeId type) {
  return fml::TimeDelta::FromNanoseconds(
      id_to_perform_data_[type].average_bind_time_ns);
}

void ListAdapter::NotifyItemMoved(int from_position, int to_position) {
  if (observer_) {
    observer_->OnItemMoved(from_position, to_position);
  }
}
void ListAdapter::NotifyItemRangeInserted(int position_start, int item_count) {
  if (observer_) {
    observer_->OnItemRangeInserted(position_start, item_count);
  }
}
void ListAdapter::NotifyItemRangeRemoved(int position_start, int item_count) {
  if (observer_) {
    observer_->OnItemRangeRemoved(position_start, item_count);
  }
}
void ListAdapter::NotifyItemRangeChanged(int position_start, int item_count,
                                         std::unique_ptr<Payload> payload) {
  if (observer_) {
    observer_->OnItemRangeChanged(position_start, item_count,
                                  std::move(payload));
  }
}

void ListAdapter::NotifyDataSetChanged() {
  if (observer_) {
    observer_->OnChanged();
  }
}

int ListAdapter::FindPreviousFullSpan(int position) const {
  for (int i = position; i >= 0; i--) {
    if (IsItemFullSpan(i)) {
      return i;
    }
  }
  return ListItemViewHolder::kNoPosition;
}

int ListAdapter::FindNextFullSpan(int position) const {
  for (int i = position; i < GetItemCount(); i++) {
    if (IsItemFullSpan(i)) {
      return i;
    }
  }
  return ListItemViewHolder::kNoPosition;
}

}  // namespace clay
