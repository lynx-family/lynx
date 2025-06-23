// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_LIST_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_LIST_H_

#include <arkui/native_animate.h>
#include <native_vsync/native_vsync.h>

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_component.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/base_scroll_container.h"

namespace lynx {
namespace tasm {
namespace harmony {

static constexpr ArkUI_NodeEventType LIST_NODE_EVENT_TYPES[] = {
    NODE_SCROLL_EVENT_ON_SCROLL, NODE_SCROLL_EVENT_ON_SCROLL_START,
    NODE_SCROLL_EVENT_ON_SCROLL_STOP, NODE_SCROLL_EVENT_ON_WILL_SCROLL};
class AutoScroller;
class UIList : public BaseScrollContainer,
               public UIComponent::NodeReadyListener {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIList(context, sign, tag);
  }
  ~UIList() override;

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void OnNodeReady() override;
  void UpdateProps(PropBundleHarmony* props) override;
  void AddChild(UIBase* child, int index) override;
  void RemoveChild(UIBase* child) override;
  void OnMeasure(ArkUI_LayoutConstraint* layout_constraint) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;
  bool IsList() const override { return true; }
  // -----------  ui-list -> list-element   start   --------------//
  void ScrollByListContainer(float content_offset_x, float content_offset_y,
                             float original_x, float original_y);

  void ScrollToPosition(int index, float offset, int align, bool smooth);
  void ScrollStopped();
  // --------------------   ui-list -> list-element   end    ---------------//

  // --------------------   list-element -> ui-list   start    ---------------//
  void UpdateContentSizeAndOffset(float content_size, float delta_x,
                                  float delta_y, bool is_init_scroll_offset,
                                  bool from_layout);
  void UpdateScrollInfo(bool smooth, float estimated_offset, bool scrolling);
  void InsertListItemNode(UIComponent* childSign);
  void RemoveListItemNode(UIComponent* childSign);

  void OnLayoutFinish(UIBase* component, int64_t operation_id) override;
  // --------------------   list-element -> ui-list   end    ---------------//
  void AutoScrollStopped() override;
  void OnComponentNodeReady(UIComponent* ui_component) override;

  std::vector<float> ScrollBy(float delta_x, float delta_y) override;

 protected:
  UIList(LynxContext* context, int sign, const std::string& tag);
  void UpdateContentSize(float width, float height) override;

 private:
  void UpdateStickyComponentInfoForUpdateAction(UIComponent* component);
  void UpdateStickyComponentForRemoveAction(UIComponent* component);
  UIComponent* GetStickyItemWithIndex(int index, bool isStickyTop);
  void ResetStickyItem(UIComponent* component);
  void UpdateListContainerInfo(const lepus::Value& value);
  void UpdateStickyTopView(float scrollY);
  void UpdateStickyBottomView(float scrollY);

  int GetIndexFromItemKey(const std::string& item_key) const;
  void SetScrollState(list::ScrollState state);
  void HandleScrollStartEvent();
  void HandleScrollEvent();
  void HandleScrollStopEvent();
  void HandleWillScrollEvent(ArkUI_NodeComponentEvent* component_event);
  bool ShouldCallScroll(float delta_x, float delta_y);
  void StartScroller();
  void ScrollToAsync(float delta_x, float delta_y);
  void GenerateStickyItemKeySet(
      std::unordered_set<std::string>& sticky_item_key_set,
      const std::vector<int>& sticky_indexes);
  void UpdateStickyItemMap(
      UIComponent* ui_component,
      std::unordered_map<std::string, UIComponent*>& sticky_item_map);
  void InvokeAutoScroll(
      float rate, bool start, bool auto_stop,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  fml::RefPtr<lepus::CArray> GetVisibleCells() const;
  void SendScrollEndEvent();

 private:
  ArkUI_NodeHandle container_layout_{nullptr};
  UIComponent* prev_sticky_top_item_{nullptr};
  UIComponent* prev_sticky_bottom_item_{nullptr};
  std::vector<std::string> item_keys_{};
  std::vector<int> sticky_top_indexes_{};
  std::vector<int> sticky_bottom_indexes_{};
  std::unordered_map<int, UIComponent*> sticky_top_items_;
  std::unordered_map<int, UIComponent*> sticky_bottom_items_;
  bool enable_batch_render_{false};
  bool enable_list_sticky_{false};
  bool enable_need_visible_item_info{false};
  bool enable_recycle_sticky_item_{true};
  float sticky_offset_{0};
  bool update_sticky_for_diff_{true};
  std::unordered_set<std::string> sticky_top_item_key_set_;
  std::unordered_set<std::string> sticky_bottom_item_key_set_;
  std::unordered_map<std::string, UIComponent*> sticky_top_item_map_;
  std::unordered_map<std::string, UIComponent*> sticky_bottom_item_map_;
  list::ScrollState scroll_state_{list::ScrollState::kIdle};
  bool should_block_scroll_{false};
  float delta_offset_{0.f};
  float pending_delta_offset_{0.f};
  float init_smooth_scroll_start_offset_{0};
  float init_smooth_scroll_end_estimated_offset_{-1};
  float content_offset_percent_during_scroll_{1};
  float snap_factor_{-1};
  float snap_offset_{0};
  std::pair<float, float> last_scroll_offset_{0.0, 0.0};

  std::shared_ptr<AutoScroller> auto_scroller_;
  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> scroll_callback_{
      nullptr};
  bool IsVisibleCellVertical(UIComponent* component) const;
  bool IsVisibleCellHorizontal(UIComponent* component) const;
  UIComponent* GetItemAtIndex(int32_t index);
  void DetectSnapScroll(int32_t action);
  std::tuple<int32_t, float, float> CalcSnapScroll(bool forward,
                                                   bool has_velocity);
  float DistanceToItem(UIComponent* list_item, bool vertical, float factor,
                       float offset, float viewport_width,
                       float viewport_height, float scroll_x, float scroll_y);
  bool IsListItem(UIBase* list_item) const;

  std::shared_ptr<animation::basic::LynxBasicAnimator>
      smooth_scroller_animator_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_LIST_H_
