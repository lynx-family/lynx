// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_GRID_LAYOUT_MANAGER_H_
#define CORE_LIST_DECOUPLED_GRID_LAYOUT_MANAGER_H_

#include "core/list/decoupled_linear_layout_manager.h"

namespace lynx {
namespace list {

class GridLayoutManager : public LinearLayoutManager {
 public:
  GridLayoutManager(ListContainerImpl* list_container_impl);
  ~GridLayoutManager() override = default;

 protected:
  void LayoutChunk(LayoutChunkResult& result, LayoutState& layout_state,
                   bool preload_section = false) override;
  void UpdateLayoutStateToFillStart(
      LayoutState& layout_state,
      const ListAnchorManager::AnchorInfo& anchor_info) override;
  void UpdateLayoutStateToFillEnd(
      LayoutState& layout_state,
      const ListAnchorManager::AnchorInfo& anchor_info) override;
  void LayoutInvalidItemHolder(int first_invalid_index) override;
  bool ShouldRecycleItemHolder(const ItemHolder* item_holder) const override;
  float GetTargetContentSize() override;
  void UpdateLayoutStateToFillPreloadBuffer(
      LayoutState& layout_state, int index, float offset,
      LayoutDirection layout_direction) override;
  int GetTargetIndexForPreloadBuffer(int start_index,
                                     LayoutDirection layout_direction) override;
#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) const override;
#endif

 private:
  // get the column size which the item holder occupies
  int getSpanSize(int index);
  // calculate the largest itemHolder‘s index in this row.
  float LargestMainSizeInRowWithItemHolder(const ItemHolder* item_holder) const;
  int GetStartIndexOfNextRow(LayoutDirection direction,
                             const int start_index) const;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_GRID_LAYOUT_MANAGER_H_
