// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_HELPER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_HELPER_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/ui/component/list/list_adapter.h"

namespace clay {

class ListAdapterHelper {
 public:
  class Consumer {
   public:
    Consumer();
    virtual ~Consumer();

    virtual void OffsetPositionsForInsert(int position_start, int item_count) {}
    virtual void OffsetPositionsForRemove(int position_start, int item_count) {}
    virtual void MarkViewHoldersChanged(
        int position_start, int item_count,
        std::unique_ptr<ListAdapter::Payload> payload) {}
    virtual void OffsetPositionsForMove(int from, int to) {}
  };

  enum class Type {
    kInsert = 1,
    kRemove = 1 << 1,
    kChange = 1 << 2,
    kMove = 1 << 3,
  };

  explicit ListAdapterHelper(Consumer* consumer);
  ~ListAdapterHelper();

  void OnItemRangeInserted(int position_start, int item_count);
  void OnItemRangeRemoved(int position_start, int item_count);
  void OnItemRangeChanged(int position_start, int item_count,
                          std::unique_ptr<ListAdapter::Payload> payload);
  void OnItemMoved(int from_position, int to_position);

  bool ConsumeUpdates();

  bool HasPendingUpdates() { return !pending_updates_.empty(); }

  void Reset();

 private:
  struct UpdateOp {
    Type type;
    int position_start;
    int item_count;  // For Move, this represents the to_position.
    std::unique_ptr<ListAdapter::Payload> payload;

    UpdateOp(Type t, int start, int count,
             std::unique_ptr<ListAdapter::Payload> p = nullptr)
        : type(t),
          position_start(start),
          item_count(count),
          payload(std::move(p)) {}
  };

  // Not owned.
  Consumer* consumer_ = nullptr;
  std::vector<UpdateOp> pending_updates_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_HELPER_H_
