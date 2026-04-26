// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_OVERLAY_MANAGER_H_
#define CLAY_UI_COMMON_OVERLAY_MANAGER_H_

#include <list>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "clay/ui/component/overlay_view.h"

namespace clay {

class PageView;
class OverlayView;

class OverlayManager {
 public:
  explicit OverlayManager(PageView* page_view);

  std::string GenerateNextOverlayId();

  OverlayView* GetOverlay(std::string_view overlay_id) const;

  void OnHideOverlay(OverlayView* overlay);
  void OnShowOverlay(OverlayView* overlay);

  bool HasOverlay() const { return !overlays_.empty(); }

  bool DispatchKeyEvent(const KeyEvent* event);
  bool HitTest(const PointerEvent& event, HitTestResult& result,
               bool& is_pass_through, PointerEvent& converted_position);
  BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                    FloatPoint* relative_position,
                                    bool& is_pass_through,
                                    FloatPoint& converted_position,
                                    int platform_try_hit_id = -1);

  void OnReportTopViewEvent(const PointerEvent& event, ClayEventType type);

  void SaveFocus();
  void RestoreFocus();

 private:
  FRIEND_TEST(OverlayViewTest, ShowAndHide);
  FRIEND_TEST(OverlayViewTest, Level);

  // FIXME(yulitao): There will still be some bug remain. For example,
  //  there is a view tree like: page -> modal1 -> modal2 -> modal3.
  //  Now focus is on modal3. Meanwhile making modal1 invisible,
  //  focus will be assigned to modal2 rather than not changed.
  // To resolve this, make association between saved focus node and overlay.
  class FocusSaver {
   public:
    void Save(FocusNode* node) {
      saved_focus_stack_.emplace(node);
      if (node) {
        node->ClearFocus();
      }
    }

    void Restore() {
      if (!saved_focus_stack_.empty() && saved_focus_stack_.top()) {
        saved_focus_stack_.top()->RequestFocus();
        saved_focus_stack_.pop();
      }
    }

   private:
    // Helper class for monitor lifecycle of a FocusNode
    class FocusNodeCacheWrapper : public FocusNode::LifecycleListener {
     public:
      explicit FocusNodeCacheWrapper(FocusNode* node) {
        node_ = node;
        if (node_) node_->AddLifecycleListener(this);
      }

      virtual ~FocusNodeCacheWrapper() {
        if (node_) node_->RemoveLifecycleListener(this);
      }

      FocusNodeCacheWrapper& operator=(FocusNodeCacheWrapper&&) = delete;
      FocusNodeCacheWrapper& operator=(const FocusNodeCacheWrapper&) = delete;
      FocusNodeCacheWrapper(FocusNodeCacheWrapper&&) = delete;
      FocusNodeCacheWrapper(const FocusNodeCacheWrapper&) = delete;

      FocusNode* operator->() {
        FML_DCHECK(node_);
        return node_;
      }
      bool operator==(const FocusNode* other) { return node_ == other; }
      bool operator!=(const FocusNode* other) { return !operator==(other); }
      operator bool() { return !!node_; }

     private:
      void OnFocusNodeUnregistered(FocusNode* node) override {
        FML_DCHECK(node_ == node);
        if (node_) {
          node_->RemoveLifecycleListener(this);
        }
        node_ = nullptr;
      }

      FocusNode* node_ = nullptr;
    };

    std::stack<FocusNodeCacheWrapper> saved_focus_stack_;
  };

  void RequestCloseOverlay(std::string_view overlay_id);

  // Send callback to lynx
  void SendOverlayEvent(OverlayView* overlay, const char* event_name) const;

  PageView* page_view_;

  FocusSaver focus_saver_;

  bool ignore_up_event_once_ = false;
  uint32_t current_id_ = 0;

  // All visible overlay will manager by `OverlayManager`
  std::list<OverlayView*> overlays_;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_OVERLAY_MANAGER_H_
