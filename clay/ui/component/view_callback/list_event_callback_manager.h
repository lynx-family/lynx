// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_EVENT_CALLBACK_MANAGER_H_
#define CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_EVENT_CALLBACK_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/ui/component/view_callback/scroll_event_callback_manager.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class ListLayoutManager;
class ListItemViewHolder;
class ListView;

class ListEventCallbackManager : public ScrollEventCallbackManager {
 public:
  struct ItemAppearanceEvent {
    bool appeared;
    int view_id;
    int position;
    std::string item_key;
    ItemAppearanceEvent(bool _appeared, int _id, int _pos,
                        const std::string& _key)
        : appeared(_appeared), view_id(_id), position(_pos), item_key(_key) {}
    ItemAppearanceEvent(ItemAppearanceEvent&& other)
        : appeared(other.appeared),
          view_id(other.view_id),
          position(other.position),
          item_key(std::move(other.item_key)) {}
  };

  ListEventCallbackManager(BaseView* view, int32_t callback_id);
  ~ListEventCallbackManager();

  // Should keep the same to
  // com.lynx.tasm.behavior.ui.ScrollStateChangeListener.java so that we have
  // the same protocol
  enum class ScrollState {
    kIdle = 1,  // Not scrolling
    kDragging,  // Dragged by outside input
    kSettling,  // Animating to a final position while not under outside control
  };

  bool AddEventCallback(const std::string& event) override;

  // Scroll state changed notification
  void OnScrollStateChange(ScrollState state);

  // Item appear/disappear notification
  void OnItemAppear(ListItemViewHolder* item);
  void OnItemDisappear(ListItemViewHolder* item);

  void SetLayoutManager(ListLayoutManager* manager) { manager_ = manager; }
  void SetNeedsVisibleCells(bool enable) { needs_visible_cells_ = enable; }

  void NotifyScrolled(const FloatSize& scrolled, const FloatSize& offset,
                      const BorderStatus& current_status,
                      const bool is_dragging = false) override;

  void SendScrollEvent(const char* event_name, const FloatSize& scrolled,
                       const FloatSize& offset, const FloatSize& content,
                       const bool is_dragging = false) const override;

 private:
  FRIEND_TEST(FocusListViewTest, AppearDisappearEvent);
  FRIEND_TEST(FocusListViewTest, EventFindFocus);
  FRIEND_TEST(FocusListViewTest, EventThrottling);

  void SendScrollStateChangeEvent(ScrollState state);
  void SendItemAppearanceEvent(bool appear, ListItemViewHolder* item);
  void StartItemAppearanceEventTimer();
  void FlushItemAppearanceEvents();

  bool enable_scroll_state_change_event_ = false;
  bool needs_visible_cells_ = false;

  ListLayoutManager* manager_;

  // We use a timer to throttle the list item appearance events.
  std::unique_ptr<fml::OneshotTimer> appearance_event_timer_;
  // An event is sent only when it "survives" after two generations (two timer
  // periods).
  // During the period, if an item appears and then disappears we will discard
  // the item's events and they won't be sent.
  std::vector<ItemAppearanceEvent> appearance_events_gen1_;
  std::vector<ItemAppearanceEvent> appearance_events_gen2_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_VIEW_CALLBACK_LIST_EVENT_CALLBACK_MANAGER_H_
