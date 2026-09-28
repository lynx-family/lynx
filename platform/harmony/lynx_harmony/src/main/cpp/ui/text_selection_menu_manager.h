// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_MANAGER_H_

#include <arkui/native_dialog.h>

#include <memory>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/text_selection_menu.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxContext;

class TextSelectionMenuManager
    : public TextSelectionMenu,
      public std::enable_shared_from_this<TextSelectionMenuManager> {
 public:
  TextSelectionMenuManager(LynxContext* context, Callbacks callbacks);
  ~TextSelectionMenuManager() override;

  void Show(const LayoutInfo& layout) override;
  void Update(const LayoutInfo& layout) override;
  void Hide() override;

  bool IsVisible() const override { return visible_; }
  ArkUI_NodeHandle OverlayNode() const override { return overlay_node_; }
  ArkUI_NodeHandle BackdropNode() const override { return backdrop_node_; }

 private:
  enum class Action {
    kCopy,
    kSelectAll,
    kDismiss,
  };

  enum class DialogState {
    kHidden,
    kShown,
    kClosing,
  };

  struct EventExtra {
    TextSelectionMenuManager* owner{nullptr};
    Action action{Action::kCopy};
  };

  static void NodeEventReceiver(ArkUI_NodeEvent* event);
  static void DialogDismissReceiver(ArkUI_DialogDismissEvent* event);

  void EnsureNodesCreated();
  void EnsureDialogCreated();
  void Dispose();
  void ScheduleDialogReady();
  bool UpdateLayout();
  void HandleAction(Action action);
  LynxContext* context_{nullptr};
  Callbacks callbacks_;
  LayoutInfo layout_;
  ArkUI_NativeDialogHandle dialog_{nullptr};
  ArkUI_NodeHandle overlay_node_{nullptr};
  ArkUI_NodeHandle backdrop_node_{nullptr};
  ArkUI_NodeHandle menu_node_{nullptr};
  ArkUI_NodeHandle copy_item_node_{nullptr};
  ArkUI_NodeHandle select_all_item_node_{nullptr};
  EventExtra dismiss_event_extra_{};
  EventExtra copy_event_extra_{};
  EventExtra select_all_event_extra_{};
  DialogState dialog_state_{DialogState::kHidden};
  bool visible_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_MENU_MANAGER_H_
