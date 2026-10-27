// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_EVENT_FOCUS_MANAGER_H_
#define CLAY_UI_EVENT_FOCUS_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/fml/time/time_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {
class FocusNode;
class KeyEvent;

class FocusManager {
 public:
  explicit FocusManager(FocusNode* focus_scope_node = nullptr);
  ~FocusManager() = default;

  struct Preference {
    // if false, traversal until the find the most match node.
    bool pick_first = true;
    // if false, pick the right node if nodes' y()is same.
    // for example,
    //   node1  node2
    //       node3
    //  when move up from node3, node1 will get focus by default.
    bool pick_left_when_same_y = true;
    // This sets whether we can trigger multiple focus switches in a single
    // frame. If true, only the first switch in a frame will take effect.
    bool switch_up_to_once_per_frame = true;
  };

  void SetPreference(const Preference& preference) { preference_ = preference; }
  Preference& preference() { return preference_; }

  enum class Direction {
    kUnknown = 0,
    kUp,
    kDown,
    kLeft,
    kRight,
  };
  enum class TraversalType {
    kDefault = 0,  // Traversal by edge node's edge.
    kCenter,       // Traversal by edge node's center point.
    kNearest,      // Find nearest node.
  };

  struct TraversalResult {
    bool succeed = false;
    Direction direction = Direction::kUnknown;
  };

  void RegisterFocusableView(FocusNode*);
  void UnregisterFocusableView(FocusNode*);

  void RequestFocus(FocusNode*);
  void ClearFocus(FocusNode*);
  void SetFocusable(bool focusable);

  void SetIsRootScope();
  FocusManager* GetRootFocusManager();
  bool DispatchKeyEvent(const KeyEvent* event);
  bool HasFocusedNode() { return !!current_focus_view_; }
  // Note: If caller needs cache focus node, a FocusNodeListener should be
  // added and listening OnFocusNodeUnregistered to avoid dangling pointer.
  FocusNode* GetLeafFocusedNode();
  FloatRect GetFocusedNodeRect();

  void SetFirstFocusedNodeExpandRatio(float ratio) {
    first_node_expand_ratio_ = ratio;
  }

  // Set the trigger interval for changing the focus by pressing keys
  void SetTriggerInterval(int interval) { trigger_interval_ = interval; }

  // This will disable focus switching until the next frame
  // (`RestoreFocusSwitching` will be called in the next frame). There is a 1
  // second timeout to avoid potential problems (e.g. focus switching without
  // requesting a new frame).
  void DisableFocusUntilNextFrame();
  void RestoreFocusSwitching();

  TraversalResult DoTraversal(Direction direction);

 private:
  FRIEND_TEST(FocusManagerTest, SortViews);
  FRIEND_TEST(FocusManagerTest, FindNextFocusView);
  friend class FocusNode;

  TraversalResult DoTraversalInternal(Direction direction,
                                      TraversalType traversal_type);
  void OnTraversalEnd(TraversalResult result);
  void SortViews(Direction direction);
  void SortViewsByCenter(Direction direction);
  FocusNode* FindNextFocusView(Direction direction,
                               TraversalType traversal_type);
  FocusNode* FindFocusViewById(const std::string& id);
  void UpdateFocusRect();
  bool OnKeyEvent(const KeyEvent* event);

  Preference preference_;

  bool focusable_ = false;
  bool is_root_ = false;
  float first_node_expand_ratio_ = 0.f;
  // FocusManager is holden by focus_scope_node_.
  FocusNode* focus_scope_node_ = nullptr;
  std::list<FocusNode*> focusable_views_;
  FocusNode* current_focus_view_ = nullptr;

  int trigger_interval_ = 0;        // in milliseconds
  int64_t last_trigger_time_ = -1;  // in milliseconds
  // This is used to temporarily disable focus switching until the given time.
  int64_t disable_focus_until_time_ = 0;  // in milliseconds
};

}  // namespace clay
#endif  // CLAY_UI_EVENT_FOCUS_MANAGER_H_
