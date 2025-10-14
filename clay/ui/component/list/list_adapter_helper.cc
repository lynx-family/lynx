// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_adapter_helper.h"

#include <numeric>
#include <utility>

namespace clay {

ListAdapterHelper::Consumer::Consumer() = default;

ListAdapterHelper::Consumer::~Consumer() = default;

ListAdapterHelper::ListAdapterHelper(Consumer* consumer)
    : consumer_(consumer) {}

ListAdapterHelper::~ListAdapterHelper() = default;

void ListAdapterHelper::OnItemRangeInserted(int position_start,
                                            int item_count) {
  pending_updates_.emplace_back(Type::kInsert, position_start, item_count);
}

void ListAdapterHelper::OnItemRangeRemoved(int position_start, int item_count) {
  pending_updates_.emplace_back(Type::kRemove, position_start, item_count);
}

void ListAdapterHelper::OnItemRangeChanged(
    int position_start, int item_count,
    std::unique_ptr<ListAdapter::Payload> payload) {
  pending_updates_.emplace_back(Type::kChange, position_start, item_count,
                                std::move(payload));
}

void ListAdapterHelper::OnItemMoved(int from_position, int to_position) {
  pending_updates_.emplace_back(Type::kMove, from_position, to_position);
}

bool ListAdapterHelper::ConsumeUpdates() {
  bool has_pending_updates = !pending_updates_.empty();
  if (consumer_ != nullptr) {
    for (UpdateOp& op : pending_updates_) {
      switch (op.type) {
        case Type::kInsert:
          consumer_->OffsetPositionsForInsert(op.position_start, op.item_count);
          break;
        case Type::kRemove:
          consumer_->OffsetPositionsForRemove(op.position_start, op.item_count);
          break;
        case Type::kChange:
          consumer_->MarkViewHoldersChanged(op.position_start, op.item_count,
                                            std::move(op.payload));
          break;
        case Type::kMove:
          consumer_->OffsetPositionsForMove(op.position_start, op.item_count);
          break;
      }
    }
  }
  pending_updates_.clear();
  return has_pending_updates;
}

void ListAdapterHelper::Reset() { pending_updates_.clear(); }

}  // namespace clay
