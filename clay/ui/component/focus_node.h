// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FOCUS_NODE_H_
#define CLAY_UI_COMPONENT_FOCUS_NODE_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "clay/gfx/geometry/axis.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/point.h"
#include "clay/ui/event/focus_manager.h"

/*
 * The front end wants to maintain focus by itself. Make all views not focusable
 * by default, so that arrow key events will not be handled by FocusManager to
 * change focus state, instead, reported to the front end.
 */
#define FORCE_TEXTVIEW_FOCUSABLE 0
#define FORCE_IMAGEVIEW_FOCUSABLE 0

namespace clay {
class FocusManager;
class KeyEvent;

// Defines when focus is traversed onto self, how to deal with.
// Doesn't affect behavior when RequestFocus.
enum class FocusBehavior {
  // Default behavior. Just accept focus for self.
  kFocus = 0,
  // Pass focus to the first child. Like "step into" when debug.
  // Effective only if is a focus scope.
  // This usually used by a list.
  kStepIntoChild,
  // Skip to next sibling. Usually used when node is invisible.
  kSkip,
};

class FocusNode {
 public:
  FocusNode();
  virtual ~FocusNode();

  class LifecycleListener {
   public:
    virtual void OnFocusNodeUnregistered(FocusNode*) {}
    virtual void OnFocusNodeDestructed(FocusNode*) {}
  };

  virtual FocusManager* GetFocusManager();
  FocusManager* GetRootFocusManager();

  virtual void SetFocusable(bool focusable);
  bool GetFocusable() const { return pending_focusable_; }

  // store and restore focusable attribute for focus scope when registering and
  // unregistering child focusable views respectively
  void StoreFocusable() { focusable_store_ = focusable_; }
  void RestoreFocusable() { SetFocusable(focusable_store_); }

  // |is_leaf| indicate whether the focus / blur node is the leaf node (not the
  // focus manager node that get focus/blur recursively).
  void RequestFocus(bool is_leaf = true);
  void ClearFocus();

  virtual FocusManager* GetParentFocusManager() = 0;
  //
  virtual void FocusHasChanged(bool focus, bool is_leaf) {}
  virtual void SetHasDefaultFocusRing(bool) {}
  virtual bool DispatchKeyEventOnFocusNode(const KeyEvent* event) = 0;
  virtual FloatRect GetContentVisibleRect() = 0;
  virtual FloatSize GetThicknessOffset() = 0;
  virtual FocusManager::TraversalResult OnTraversalOnScope(
      FocusManager::Direction direction,
      FocusManager::TraversalType traversal_type);

  bool IsFocused() { return is_focused_; }
  void SetIsFocusScope();
  void SetIsFocusFence();
  bool IsFocusScope() const { return !!focus_manager_; }
  bool IsFocusFence() const { return is_fence_; }
  virtual bool allow_escape() { return false; }
  FloatRect focus_rect() { return focus_rect_; }
  void UpdateFocusRect();

  void AddLifecycleListener(LifecycleListener* listener) {
    if (!life_listeners_) {
      life_listeners_.reset(new std::set<LifecycleListener*>());
    }
    life_listeners_->insert(listener);
  }

  void RemoveLifecycleListener(LifecycleListener* listener) {
    if (life_listeners_) {
      life_listeners_->erase(listener);
    }
  }

  void SetFocusIndex(const Point& index) { focus_index_ = index; }

  int GetFocusIndex(Axis axis) const;
  // If index < 0, nodes cannot be focused while traversal focus, for example,
  // by arrow keys.
  bool CanAcceptFocusOnAxis(Axis axis) const;

  // Although node is focusable, there still has some cases that should skip
  // focus while traversed by arrow keys, mostly because it is not visible.
  virtual FocusBehavior GetFocusBehavior() const {
    return FocusBehavior::kFocus;
  }

  void SetNextFocusUp(std::string id) { next_focus_up_ = std::move(id); }
  void SetNextFocusDown(std::string id) { next_focus_down_ = std::move(id); }
  void SetNextFocusLeft(std::string id) { next_focus_left_ = std::move(id); }
  void SetNextFocusRight(std::string id) { next_focus_right_ = std::move(id); }
  void SetNextFocusFallback(bool enabled) { next_focus_fallback_ = enabled; }
  bool NextFocusFallback() const { return next_focus_fallback_; }
  const std::string& GetNextFocusId(FocusManager::Direction direction) const;
  virtual const std::string& FocusId() const = 0;

  // A fence node is interested with these events.
  virtual void OnFocusEscaping(FocusManager::Direction direction) {}
  virtual void OnFocusEntering(FocusManager::Direction direction) {}

 protected:
  virtual FloatRect CalcFocusRect() const = 0;
  void SetFocusableInternal(bool focusable);
  void UpdateFocusRing();
  void SetSkipFocusTraversal(bool skip_focus_traversal) {
    skip_focus_traversal_ = skip_focus_traversal;
  }
  void OnFocusAttach();
  void OnFocusDetach();

  FloatRect focus_rect_;
  // Effective only on FocusScope like Overlay / ListView / ScrollView
  // If not set, skip focus traversal if it is focus scope.
  std::optional<bool> skip_focus_traversal_;
  bool focusable_ = false;
  std::string next_focus_up_;
  std::string next_focus_down_;
  std::string next_focus_left_;
  std::string next_focus_right_;
  // Determines whether to fallback to automatic traversal if the node cannot be
  // found using the specified `next-focus-*`.
  bool next_focus_fallback_ = false;
  bool pending_focusable_ = false;
  bool pending_request_focus_ = false;
  bool restoring_focus_ = false;
  // store the original focusable for the focus scope
  bool focusable_store_ = false;
  bool is_focused_ = false;
  // A focus fence means while moving focus, it cannot cross over a fence
  // automatically.
  bool is_fence_ = false;
  // Same as web tabindex, but 2D.
  Point focus_index_;
  // In most cases it is unused. So make unique_ptr to save memory.
  std::unique_ptr<std::set<LifecycleListener*>> life_listeners_;
  std::unique_ptr<FocusManager> focus_manager_;
};
}  // namespace clay
#endif  // CLAY_UI_COMPONENT_FOCUS_NODE_H_
