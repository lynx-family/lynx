// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_callback/list_event_callback_manager.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/time/timer.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"

namespace clay {
namespace {

constexpr fml::TimeDelta kItemAppearanceEventThrottle =
    fml::TimeDelta::FromMilliseconds(50);

constexpr fml::TimeDelta kDefaultEventThrottle =
    fml::TimeDelta::FromMilliseconds(200);

using ItemAppearanceEvent = ListEventCallbackManager::ItemAppearanceEvent;

void FilterDuplication(std::vector<ItemAppearanceEvent>& events) {
  std::vector<ItemAppearanceEvent> res;
  std::vector<bool> should_send(events.size(), false);
  std::unordered_map<std::string, int> key_to_index;

  for (size_t i = 0; i < events.size(); ++i) {
    const std::string& key = events[i].item_key;
    FML_DCHECK(key.length());
    auto itr = key_to_index.find(key);
    if (itr == key_to_index.end()) {
      should_send[i] = true;
      key_to_index[key] = i;
    } else {
      should_send[i] = false;
      should_send[itr->second] = false;
      key_to_index.erase(key);
    }
  }

  for (size_t i = 0; i < events.size(); ++i) {
    if (should_send[i]) {
      res.emplace_back(std::move(events[i]));
    }
  }

  std::swap(events, res);
}

void FilterDuplication(std::vector<ItemAppearanceEvent>& gen1,
                       std::vector<ItemAppearanceEvent>& gen2) {
  std::unordered_set<std::string> gen1_keys;
  gen1_keys.reserve(gen1.size());
  std::unordered_set<std::string> gen2_keys;
  gen2_keys.reserve(gen2.size());

  for (auto& i : gen1) {
    gen1_keys.emplace(i.item_key);
  }
  for (auto& i : gen2) {
    const std::string& key = i.item_key;
    if (gen1_keys.find(key) != gen1_keys.end()) {
      gen1_keys.erase(key);
    } else {
      gen2_keys.emplace(key);
    }
  }

  if (gen1_keys.size() == gen1.size()) {
    FML_DCHECK(gen2_keys.size() == gen2.size());
    return;
  }

  std::vector<ItemAppearanceEvent> new_gen1;
  new_gen1.reserve(gen1_keys.size());
  for (auto& i : gen1) {
    if (gen1_keys.find(i.item_key) != gen1_keys.end()) {
      new_gen1.emplace_back(std::move(i));
    }
  }

  std::vector<ItemAppearanceEvent> new_gen2;
  new_gen2.reserve(gen2_keys.size());
  for (auto& i : gen2) {
    if (gen2_keys.find(i.item_key) != gen2_keys.end()) {
      new_gen2.emplace_back(std::move(i));
    }
  }

  std::swap(new_gen1, gen1);
  std::swap(new_gen2, gen2);
}

}  // namespace

ListEventCallbackManager::ListEventCallbackManager(BaseView* view,
                                                   int32_t callback_id)
    : ScrollEventCallbackManager(view, callback_id) {
  SetScrollEventThrottle(kDefaultEventThrottle);
}

ListEventCallbackManager::~ListEventCallbackManager() = default;

bool ListEventCallbackManager::AddEventCallback(const std::string& event) {
  if (ScrollEventCallbackManager::AddEventCallback(event)) {
    return true;
  }
  if (event.compare(event_attr::kEventScrollStateChange) == 0) {
    enable_scroll_state_change_event_ = true;
    return true;
  }

  return false;
}

void ListEventCallbackManager::OnScrollStateChange(ScrollState state) {
  if (!enable_scroll_state_change_event_) {
    return;
  }
  FML_DCHECK(view_ != nullptr);
  FML_DCHECK(manager_ != nullptr);
  SendScrollStateChangeEvent(state);
}

static bool ShouldSendItemAppearanceEvent(bool appear,
                                          ListItemViewHolder* item) {
  if (!item) {
    return false;
  }
  return !item->GetView()->ItemKey().empty() &&
         (appear ? item->GetView()->HasEvent(event_attr::kEventNodeAppear)
                 : item->GetView()->HasEvent(event_attr::kEventNodeDisappear));
}

void ListEventCallbackManager::OnItemAppear(ListItemViewHolder* item) {
  if (!item) {
    return;
  }

  FML_DCHECK(item->GetView() != nullptr);
  if (ShouldSendItemAppearanceEvent(true, item)) {
    SendItemAppearanceEvent(true, item);
  }
}

void ListEventCallbackManager::OnItemDisappear(ListItemViewHolder* item) {
  if (!item) {
    return;
  }

  FML_DCHECK(item->GetView() != nullptr);
  if (ShouldSendItemAppearanceEvent(false, item)) {
    SendItemAppearanceEvent(false, item);
  }
}

void ListEventCallbackManager::NotifyScrolled(
    const FloatPoint& scrolled, const FloatPoint& offset,
    const BorderStatus& current_status, const bool is_dragging) {
  HandleScrolled(scrolled, offset, current_status);
}

void ListEventCallbackManager::SendScrollEvent(const char* event_name,
                                               const FloatPoint& scrolled,
                                               const FloatPoint& offset,
                                               const FloatSize& content,
                                               const bool is_dragging) const {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  size_t visible_count = 0;
  if (needs_visible_cells_) {
    visible_count =
        manager_->GetVisibleItemsInfo(top_array, bottom_array, left_array,
                                      right_array, position_array, id_array);
  }
  clay::Value::Map args;
  args["scrollLeft"] = clay::Value(0.f);
  args["scrollTop"] = clay::Value(offset.y());
  args["deltaX"] = clay::Value(0.f);
  args["deltaY"] = clay::Value(scrolled.y());
  clay::Value::Array cells_array(visible_count);
  for (size_t i = 0; i < visible_count; i++) {
    clay::Value::Map cell;
    cell["id"] = clay::Value(id_array[i]);
    cell["position"] = clay::Value(position_array[i]);
    cell["left"] = clay::Value(left_array[i]);
    cell["right"] = clay::Value(right_array[i]);
    cell["top"] = clay::Value(top_array[i]);
    cell["bottom"] = clay::Value(bottom_array[i]);
    cells_array[i] = clay::Value(std::move(cell));
  }
  args["attachedCells"] = clay::Value(std::move(cells_array));
  page_view_->SendCustomEvent(callback_id_, event_name, std::move(args));
}

void ListEventCallbackManager::SendScrollStateChangeEvent(ScrollState state) {
  std::vector<float> left_array, right_array, top_array, bottom_array;
  std::vector<int> position_array;
  std::vector<std::string> id_array;
  size_t visible_count = 0;
  if (needs_visible_cells_) {
    visible_count =
        manager_->GetVisibleItemsInfo(top_array, bottom_array, left_array,
                                      right_array, position_array, id_array);
  }

  clay::Value::Map args;
  args["state"] = clay::Value(static_cast<int>(state));
  clay::Value::Array cells_array(visible_count);
  for (size_t i = 0; i < visible_count; i++) {
    clay::Value::Map cell;
    cell["id"] = clay::Value(id_array[i]);
    cell["position"] = clay::Value(position_array[i]);
    cell["left"] = clay::Value(left_array[i]);
    cell["right"] = clay::Value(right_array[i]);
    cell["top"] = clay::Value(top_array[i]);
    cell["bottom"] = clay::Value(bottom_array[i]);
    cells_array[i] = clay::Value(std::move(cell));
  }
  args["attachedCells"] = clay::Value(std::move(cells_array));
  page_view_->SendCustomEvent(callback_id_, event_attr::kEventScrollStateChange,
                              std::move(args));
}

void ListEventCallbackManager::SendItemAppearanceEvent(
    bool appear, ListItemViewHolder* item) {
  int position = item->GetPosition();
  if (item->IsRemoved()) {
    position = item->GetLastValidPosition();
  }
  appearance_events_gen1_.emplace_back(appear, item->GetView()->id(), position,
                                       item->GetView()->ItemKey());
  StartItemAppearanceEventTimer();
}

void ListEventCallbackManager::StartItemAppearanceEventTimer() {
  if (appearance_event_timer_) {
    return;
  }
  appearance_event_timer_ =
      std::make_unique<fml::OneshotTimer>(page_view_->GetTaskRunner());
  appearance_event_timer_->Start(kItemAppearanceEventThrottle,
                                 [this]() { FlushItemAppearanceEvents(); });
}

void ListEventCallbackManager::FlushItemAppearanceEvents() {
  appearance_event_timer_ = nullptr;
  const bool should_continue = appearance_events_gen1_.size() > 0;

  // Before sending the events, we go through two passes:
  // 1. Filter the generation 1 events so it won't have events for the same
  // item.
  // 2. Filter generation 1 with generation 2. The events left in generation 2
  // are ready to send.
  FilterDuplication(appearance_events_gen1_);
  FilterDuplication(appearance_events_gen1_, appearance_events_gen2_);

  for (const ItemAppearanceEvent& event : appearance_events_gen2_) {
    page_view_->SendEvent(event.view_id,
                          event.appeared ? event_attr::kEventNodeAppear
                                         : event_attr::kEventNodeDisappear,
                          {"position", "key"}, event.position, event.item_key);
  }

  // The previous generation 1 events becomes generation 2.
  appearance_events_gen2_.clear();
  std::swap(appearance_events_gen1_, appearance_events_gen2_);

  if (should_continue) {
    StartItemAppearanceEventTimer();
  }
}

}  // namespace clay
