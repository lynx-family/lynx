// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_VIEW_H_
#define CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_VIEW_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "clay/ui/component/component.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view_callback/list_event_callback_manager.h"

namespace clay {

class ListContainerView : public WithTypeInfo<ListContainerView, ScrollView>,
                          public Component::NodeReadyListener {
 public:
  class Delegate {
   public:
    virtual void OnListViewDidLayout() = 0;
    virtual void OnScrollByListContainer(float offset_x, float offset_y,
                                         float original_x,
                                         float original_y) = 0;
    virtual void OnScrollToPosition(int position, float offset, int align,
                                    bool smooth) = 0;
    virtual void OnScrollStopped() = 0;
  };

  using ScrollState = ListEventCallbackManager::ScrollState;

  ListContainerView(int32_t id, PageView* page_view);

  void UpdateContentOffsetForListContainer(float content_size,
                                           float target_content_offset_x,
                                           float target_content_offset_y);

  void UpdateScrollInfo(bool smooth, float estimated_offset, bool scrolling);

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  void ScrollToPosition(
      bool smooth, int position, float offset, AlignTo align_to,
      const std::string& id,
      const std::function<void(uint32_t, const std::string&)>& callback);

  void AddChild(BaseView* child, int position) override;

  void InsertListItemPaintingNode(BaseView* view);
  void RemoveListItemPaintingNode(BaseView* view);
  // Scroller::Delegate
  void OnScrollUpdate(float scroll_offset) override;
  void OnScrollAnimationStart(float start, float delta, int duration) override;
  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  size_t GetVisibleItemsInfo(std::vector<float>& top_array,
                             std::vector<float>& bottom_array,
                             std::vector<float>& left_array,
                             std::vector<float>& right_array,
                             std::vector<int>& position,
                             std::vector<std::string>& id_array,
                             std::vector<std::string>& item_key_array);

  clay::Value::Array GetVisibleCells();

 protected:
  void CalculateOverFlow() override;

 private:
  void DidScroll() override;
  void DidUpdateAttributes() override;

  void OnLayoutFinish(BaseView* view) override;
  void InsertListItemPaintingNodeInternal(BaseView* view);

  void OnScrollStatusChange(ScrollStatus old_status) override;
  void SetScrollState(ScrollState state);

  void SetMaxContent(float value);
  int GetIndexFromItemKey(std::string itemKey);

  // sticky
  void UpdateStickyInfoForInsertedChild(
      BaseView* child, std::unordered_map<int, Component*>& sticky_items,
      std::vector<int>& sticky_indexes, int index);
  void UpdateStickyInfoForDeletedChild(
      BaseView* child, std::unordered_map<int, Component*>& sticky_items);
  void UpdateStickyInfoForUpdatedChild(
      Component* child, std::unordered_map<int, Component*> sticky_items,
      const std::vector<int>& sticky_indexes, int index);
  void ResetStickyItem(Component* child);
  void ApplyChildTranslateZ(Component* child);
  void ApplyChildTranslateZ(Component* child, float translateZ);
  void UpdateStickyStarts(float offset_x, float offset_y);
  void UpdateStickyEnds(float offset_x, float offset_y);
  Component* GetStickyItemWithIndex(int index, bool is_sticky_top);
  void UpdateStickyItemMap(
      Component* component,
      std::unordered_map<std::string, Component*>& sticky_item_map,
      bool is_sticky_item);
  void GenerateStickyItemKeySet(
      std::unordered_set<std::string>& sticky_item_key_set,
      std::unordered_map<std::string, Component*>& sticky_item_map,
      const std::vector<int>& sticky_item_indexes);
  void EraseStickyItem(BaseView* view);

  // NodeReadyListener
  void OnComponentNodeReady(Component* component) override;

  void OnNodeReady() override;

  std::unordered_map<std::string, int> item_key_map_;

  float max_content_ = 0.f;

  bool should_block_did_scroll_ = false;
  bool enable_batch_render_strategy_ = false;

  float initial_scrolling_estimated_offset_ = 0.f;
  float scrolling_estimated_offset_ = 0.f;
  bool scroll_to_lower_ = false;
  bool is_scroll_animating_ = false;
  ScrollState scroll_state_ = ScrollState::kIdle;

  Delegate* delegate_;

  std::vector<std::string> item_keys_;

  bool enable_list_sticky_ = false;
  int sticky_offset_ = 0;

  bool need_visible_item_info_ = false;

  std::unordered_map<int, Component*> sticky_top_items_;
  std::unordered_map<int, Component*> sticky_bottom_items_;

  std::vector<int> sticky_top_indexes_;
  std::vector<int> sticky_bottom_indexes_;

  Component* prev_sticky_top_item_ = nullptr;
  Component* prev_sticky_bottom_item_ = nullptr;

  bool enable_recycle_sticky_item_ = true;
  bool update_sticky_for_diff_ = true;
  bool enable_insert_platform_view_operation_ = false;

  std::unordered_set<std::string> sticky_top_item_key_set_;
  std::unordered_set<std::string> sticky_bottom_item_key_set_;

  std::unordered_map<std::string, Component*> sticky_top_item_map_;
  std::unordered_map<std::string, Component*> sticky_bottom_item_map_;

  friend class ListContainerWrapper;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_VIEW_H_
