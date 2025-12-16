// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LINEAR_LAYOUT_MANAGER_H_
#define CORE_LIST_DECOUPLED_LINEAR_LAYOUT_MANAGER_H_

#include <string>

#include "base/include/debug/lynx_assert.h"
#include "core/list/decoupled_list_layout_manager.h"

namespace lynx {
namespace list {

class LinearLayoutManager : public ListLayoutManager {
 protected:
  class LayoutState {
   public:
    bool ValidPreload() const {
      return preload_min_index_ != kInvalidIndex ||
             preload_max_index_ != kInvalidIndex;
    }

    void ResetPreloadIndex() {
      preload_min_index_ = kInvalidIndex;
      preload_max_index_ = kInvalidIndex;
    }

    std::string ToString() const {
      return base::FormatString(
          "LayoutState: index=%d "
          "layout_direction=%d, offset=%f, available=%f, extra=%f",
          next_bind_index_, static_cast<int32_t>(layout_direction_),
          next_layout_offset_, available_, extra_);
    }

    // Min index to bind after filling to start.
    int min_layout_chunk_index_{-1};
    // Next index to bind
    int next_bind_index_{0};
    // The min bind index in preload.
    int preload_min_index_{kInvalidIndex};
    // The max bind index in preload.
    int preload_max_index_{kInvalidIndex};
    // Layout direction
    LayoutDirection layout_direction_{LayoutDirection::kLayoutToEnd};
    // Layout start coordinate.
    float next_layout_offset_{0.f};
    // Number of pixels that we should fill, in the layout direction.
    float available_{0.f};
    // Extra available area.
    float extra_{0.f};
    // The latest updated content offset.
    float latest_updated_content_offset_{0.f};
  };

  class LayoutChunkResult {
   public:
    void Reset() {
      finished_ = false;
      consumed_ = 0.f;
    }

    // Whether it is necessary to interrupt this fill.
    bool finished_{false};
    // The size of the remaining space for this fill.
    float consumed_{0.f};
  };

 public:
  LinearLayoutManager(ListContainerImpl* list_container_impl);
  ~LinearLayoutManager() override = default;

  void OnBatchLayoutChildren() override;
  void OnLayoutChildren(bool is_component_finished = false,
                        int component_index = -1) override;

 protected:
  void ScrollByInternal(float content_offset, float original_offset,
                        bool from_platform) override;
  void LayoutInvalidItemHolder(int first_invalid_index) override;
  float GetTargetContentSize() override;
  void PreloadSection() override;
  // Render and layout one ItemHolder, GridLayoutManager overrides this function
  // to render column-count ItemHolders or a full-span ItemHolder.
  virtual void LayoutChunk(LayoutChunkResult& result, LayoutState& layout_state,
                           bool preload_section = false);
  // Update layout state to fill to start, GridLayoutManager overrides this
  // function to handle LayoutState by it self.
  virtual void UpdateLayoutStateToFillStart(
      LayoutState& layout_state,
      const ListAnchorManager::AnchorInfo& anchor_info);
  // Update layout state to fill to end, GridLayoutManager overrides this
  // function to handle LayoutState by it self.
  virtual void UpdateLayoutStateToFillEnd(
      LayoutState& layout_state,
      const ListAnchorManager::AnchorInfo& anchor_info);
  virtual void UpdateLayoutStateToFillPreloadBuffer(
      LayoutState& layout_state, int index, float offset,
      LayoutDirection layout_direction);
  virtual int GetTargetIndexForPreloadBuffer(int start_index,
                                             LayoutDirection layout_direction);
#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) const override;
#endif

 private:
  void OnLayoutChildrenInternal(ListAnchorManager::AnchorInfo& anchor_info,
                                LayoutState& layout_state);
  void OnLayoutAfter(LayoutState& layout_state);
  void OnScrollAfter(LayoutState& layout_state, float original_offset);
  void FillWithAnchor(LayoutState& layout_state,
                      const ListAnchorManager::AnchorInfo& anchor_info);
  void Fill(LayoutState& layout_state);
  bool HasMore(int current_index, float remaining) const;
  bool HasMore(const LayoutState& layout_state, int target_index) const;
  void UpdateScrollAnchorInfo(ListAnchorManager::AnchorInfo& anchor_info,
                              const ItemHolderSet& on_screen_children,
                              const float content_offset);
  void HandleLayoutOrScrollResult(LayoutState& layout_state, bool is_layout);

  // Implement preload.
  void HandlePreloadIfNeeded(LayoutState& layout_state,
                             ListAnchorManager::AnchorInfo& anchor_info,
                             bool from_layout);
  bool Preload(LayoutState& layout_state);
  void PreloadInternal(LayoutState& layout_state, int target_index,
                       bool preload_section = false);
  void RecycleOffPreloadItemHolders(bool recycle_to_end, int target_index);
  void PreloadSectionOnNextFrame();
  void PreloadSection(LayoutState& layout_state);
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LINEAR_LAYOUT_MANAGER_H_
