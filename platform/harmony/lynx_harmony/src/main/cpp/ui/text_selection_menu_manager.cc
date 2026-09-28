// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/text_selection_menu_manager.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/text_selection_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

constexpr float kMenuWidthVp = 144.f;
constexpr float kMenuHeightVp = 44.f;
constexpr float kMenuItemWidthVp = kMenuWidthVp / 2.f;
constexpr float kMenuGapVp = 8.f;
constexpr int32_t kMenuZIndex = 10000;
constexpr int64_t kDialogCloseDelayMs = 300;

}  // namespace
TextSelectionMenuManager::TextSelectionMenuManager(LynxContext* context,
                                                   Callbacks callbacks)
    : context_(context), callbacks_(std::move(callbacks)) {}

TextSelectionMenuManager::~TextSelectionMenuManager() { Dispose(); }

void TextSelectionMenuManager::Show(const LayoutInfo& layout) {
  layout_ = layout;
  visible_ = true;
  if (!UpdateLayout()) {
    Hide();
  }
}

void TextSelectionMenuManager::Update(const LayoutInfo& layout) {
  layout_ = layout;
  if (visible_ && !UpdateLayout()) {
    Hide();
  }
}

void TextSelectionMenuManager::Hide() {
  if (!visible_ && dialog_state_ != DialogState::kShown) {
    return;
  }
  const bool was_visible = visible_;
  visible_ = false;
  if (dialog_ && dialog_state_ == DialogState::kShown) {
    if (auto* dialog_api = NodeManager::DialogInstance()) {
      ScheduleDialogReady();
      dialog_api->close(dialog_);
    } else {
      dialog_state_ = DialogState::kHidden;
    }
  }
  if (was_visible && callbacks_.on_hidden) {
    callbacks_.on_hidden();
  }
}

void TextSelectionMenuManager::NodeEventReceiver(ArkUI_NodeEvent* event) {
  if (!event || OH_ArkUI_NodeEvent_GetTargetId(event) != LYNX_EVENT_ID ||
      OH_ArkUI_NodeEvent_GetEventType(event) != NODE_ON_CLICK) {
    return;
  }
  auto* extra = static_cast<EventExtra*>(OH_ArkUI_NodeEvent_GetUserData(event));
  if (extra && extra->owner) {
    extra->owner->HandleAction(extra->action);
  }
}

void TextSelectionMenuManager::DialogDismissReceiver(
    ArkUI_DialogDismissEvent* event) {
  auto* owner = static_cast<TextSelectionMenuManager*>(
      event ? OH_ArkUI_DialogDismissEvent_GetUserData(event) : nullptr);
  if (!owner || owner->dialog_state_ != DialogState::kShown) {
    return;
  }
  owner->visible_ = false;
  if (owner->callbacks_.on_hidden) {
    owner->callbacks_.on_hidden();
  }
  owner->ScheduleDialogReady();
  const int32_t reason = OH_ArkUI_DialogDismissEvent_GetDismissReason(event);
  if ((reason == DIALOG_DISMISS_TOUCH_OUTSIDE ||
       reason == DIALOG_DISMISS_BACK_PRESS) &&
      owner->callbacks_.on_dismiss) {
    owner->callbacks_.on_dismiss();
  }
}

void TextSelectionMenuManager::EnsureNodesCreated() {
  if (overlay_node_ && backdrop_node_ && menu_node_ && copy_item_node_ &&
      select_all_item_node_) {
    return;
  }

  overlay_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);
  backdrop_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);
  menu_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_ROW);
  copy_item_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_TEXT);
  select_all_item_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_TEXT);
  if (!overlay_node_ || !backdrop_node_ || !menu_node_ || !copy_item_node_ ||
      !select_all_item_node_) {
    Dispose();
    return;
  }

  auto& manager = NodeManager::Instance();
  manager.SetAttributeWithNumberValue(
      backdrop_node_, NODE_HIT_TEST_BEHAVIOR,
      static_cast<int32_t>(ARKUI_HIT_TEST_MODE_BLOCK));
  dismiss_event_extra_ = {this, Action::kDismiss};
  manager.AddNodeEventReceiver(backdrop_node_, NodeEventReceiver);
  manager.RegisterNodeEvent(backdrop_node_, NODE_ON_CLICK,
                            &dismiss_event_extra_);

  manager.SetAttributeWithNumberValue(menu_node_, NODE_WIDTH, kMenuWidthVp);
  manager.SetAttributeWithNumberValue(menu_node_, NODE_HEIGHT, kMenuHeightVp);
  manager.SetAttributeWithNumberValue(menu_node_, NODE_BACKGROUND_COLOR,
                                      0xFF262626u);
  manager.SetAttributeWithNumberValue(menu_node_, NODE_BORDER_RADIUS, 8.f);
  manager.SetAttributeWithNumberValue(menu_node_, NODE_Z_INDEX, kMenuZIndex);

  const auto configure_item = [&](ArkUI_NodeHandle node, const char* text,
                                  EventExtra* extra) {
    ArkUI_AttributeItem content{.string = text};
    manager.SetAttribute(node, NODE_TEXT_CONTENT, &content);
    manager.SetAttributeWithNumberValue(node, NODE_WIDTH, kMenuItemWidthVp);
    manager.SetAttributeWithNumberValue(node, NODE_HEIGHT, kMenuHeightVp);
    manager.SetAttributeWithNumberValue(node, NODE_PADDING, 12.f, 0.f, 12.f,
                                        0.f);
    manager.SetAttributeWithNumberValue(node, NODE_FONT_SIZE, 14.f);
    manager.SetAttributeWithNumberValue(node, NODE_FONT_COLOR, 0xFFFFFFFFu);
    manager.SetAttributeWithNumberValue(
        node, NODE_TEXT_ALIGN,
        static_cast<int32_t>(ARKUI_TEXT_ALIGNMENT_CENTER));
    manager.AddNodeEventReceiver(node, NodeEventReceiver);
    manager.RegisterNodeEvent(node, NODE_ON_CLICK, extra);
  };

  copy_event_extra_ = {this, Action::kCopy};
  select_all_event_extra_ = {this, Action::kSelectAll};
  const std::string copy_text =
      GetTextSelectionLocalizedString("lynx_text_selection_copy", "Copy");
  const std::string select_all_text = GetTextSelectionLocalizedString(
      "lynx_text_selection_select_all", "Select all");
  configure_item(copy_item_node_, copy_text.c_str(), &copy_event_extra_);
  configure_item(select_all_item_node_, select_all_text.c_str(),
                 &select_all_event_extra_);
  manager.InsertNode(menu_node_, copy_item_node_, 0);
  manager.InsertNode(menu_node_, select_all_item_node_, 1);
  manager.InsertNode(overlay_node_, backdrop_node_, 0);
  manager.InsertNode(overlay_node_, menu_node_, 1);
}

void TextSelectionMenuManager::EnsureDialogCreated() {
  EnsureNodesCreated();
  if (dialog_ || !overlay_node_) {
    return;
  }

  auto* dialog_api = NodeManager::DialogInstance();
  if (!dialog_api) {
    return;
  }

  dialog_ = dialog_api->create();
  if (!dialog_) {
    return;
  }

  if (dialog_api->setContent(dialog_, overlay_node_) != 0 ||
      dialog_api->registerOnWillDismissWithUserData(
          dialog_, this, DialogDismissReceiver) != 0) {
    dialog_api->setContent(dialog_, nullptr);
    dialog_api->dispose(dialog_);
    dialog_ = nullptr;
    return;
  }
  dialog_api->enableCustomStyle(dialog_, true);
  dialog_api->enableCustomAnimation(dialog_, true);
  dialog_api->setBackgroundColor(dialog_, 0x00000000u);
  dialog_api->setModalMode(dialog_, true);
  dialog_api->setAutoCancel(dialog_, true);
  dialog_api->setMask(dialog_, 0x00000000u, nullptr);
}

void TextSelectionMenuManager::Dispose() {
  visible_ = false;
  dialog_state_ = DialogState::kHidden;
  auto* dialog_api = NodeManager::DialogInstance();
  if (dialog_ && dialog_api) {
    dialog_api->setContent(dialog_, nullptr);
    dialog_api->close(dialog_);
    dialog_api->dispose(dialog_);
    dialog_ = nullptr;
  }
  if (dialog_) {
    return;
  }

  const auto dispose_item = [](ArkUI_NodeHandle& node) {
    if (!node) {
      return;
    }
    NodeManager::Instance().UnregisterNodeEvent(node, NODE_ON_CLICK);
    NodeManager::Instance().RemoveNodeEventReceiver(node, NodeEventReceiver);
    NodeManager::Instance().DisposeNode(node);
    node = nullptr;
  };

  if (overlay_node_) {
    if (backdrop_node_) {
      NodeManager::Instance().RemoveNode(overlay_node_, backdrop_node_);
    }
    if (menu_node_) {
      NodeManager::Instance().RemoveNode(overlay_node_, menu_node_);
    }
  }
  if (menu_node_) {
    if (copy_item_node_) {
      NodeManager::Instance().RemoveNode(menu_node_, copy_item_node_);
    }
    if (select_all_item_node_) {
      NodeManager::Instance().RemoveNode(menu_node_, select_all_item_node_);
    }
  }
  dispose_item(copy_item_node_);
  dispose_item(select_all_item_node_);
  dispose_item(backdrop_node_);
  if (menu_node_) {
    NodeManager::Instance().DisposeNode(menu_node_);
    menu_node_ = nullptr;
  }
  if (overlay_node_) {
    NodeManager::Instance().DisposeNode(overlay_node_);
    overlay_node_ = nullptr;
  }
}

void TextSelectionMenuManager::ScheduleDialogReady() {
  if (dialog_state_ == DialogState::kClosing) {
    return;
  }
  dialog_state_ = DialogState::kClosing;
  if (!context_) {
    dialog_state_ = DialogState::kHidden;
    return;
  }
  const auto& task_runner = context_->GetUITaskRunner();
  if (!task_runner) {
    dialog_state_ = DialogState::kHidden;
    return;
  }

  std::weak_ptr<TextSelectionMenuManager> weak_self = weak_from_this();
  task_runner->PostDelayedTask(
      [weak_self]() {
        auto self = weak_self.lock();
        if (!self || self->dialog_state_ != DialogState::kClosing) {
          return;
        }
        self->dialog_state_ = DialogState::kHidden;
        if (self->visible_ && !self->UpdateLayout()) {
          self->Hide();
        }
      },
      fml::TimeDelta::FromMilliseconds(kDialogCloseDelayMs));
}

bool TextSelectionMenuManager::UpdateLayout() {
  if (!visible_) {
    return true;
  }
  if (dialog_state_ == DialogState::kClosing) {
    return true;
  }

  EnsureDialogCreated();
  auto* dialog_api = NodeManager::DialogInstance();
  if (!dialog_ || !overlay_node_ || !menu_node_ || !backdrop_node_ ||
      !dialog_api || !layout_.text_node || !layout_.root_node) {
    return false;
  }

  const float density = layout_.density > 0.f ? layout_.density : 1.f;
  ArkUI_IntOffset text_offset{};
  ArkUI_IntOffset root_offset{};
  if (OH_ArkUI_NodeUtils_GetPositionWithTranslateInWindow(layout_.text_node,
                                                          &text_offset) != 0 ||
      OH_ArkUI_NodeUtils_GetPositionWithTranslateInWindow(layout_.root_node,
                                                          &root_offset) != 0) {
    return false;
  }

  const ArkUI_IntSize root_size =
      NodeManager::Instance().GetMeasuredSize(layout_.root_node);
  const float root_width_vp = root_size.width / density;
  const float root_height_vp = root_size.height / density;
  if (root_width_vp <= 0.f || root_height_vp <= 0.f) {
    return false;
  }

  auto& manager = NodeManager::Instance();
  manager.SetAttributeWithNumberValue(overlay_node_, NODE_WIDTH, root_width_vp);
  manager.SetAttributeWithNumberValue(overlay_node_, NODE_HEIGHT,
                                      root_height_vp);
  manager.SetAttributeWithNumberValue(backdrop_node_, NODE_WIDTH,
                                      root_width_vp);
  manager.SetAttributeWithNumberValue(backdrop_node_, NODE_HEIGHT,
                                      root_height_vp);
  manager.SetAttributeWithNumberValue(backdrop_node_, NODE_POSITION, 0.f, 0.f);
  manager.SetMeasuredSize(overlay_node_, root_size.width, root_size.height);
  manager.SetMeasuredSize(backdrop_node_, root_size.width, root_size.height);
  manager.SetLayoutPosition(backdrop_node_, 0, 0);

  const float text_left_vp = (text_offset.x - root_offset.x) / density;
  const float text_top_vp = (text_offset.y - root_offset.y) / density;
  const float selection_center_vp =
      (layout_.selection_left_px + layout_.selection_right_px) /
      (2.f * density);
  float menu_left = text_left_vp + layout_.content_left_vp +
                    selection_center_vp - kMenuWidthVp / 2.f;
  float menu_top = text_top_vp + layout_.content_top_vp +
                   layout_.selection_top_px / density - kMenuHeightVp -
                   kMenuGapVp;
  if (menu_top < 0.f) {
    menu_top = text_top_vp + layout_.content_top_vp +
               layout_.selection_bottom_px / density + kMenuGapVp;
  }
  menu_left =
      std::clamp(menu_left, 0.f, std::max(0.f, root_width_vp - kMenuWidthVp));
  menu_top =
      std::clamp(menu_top, 0.f, std::max(0.f, root_height_vp - kMenuHeightVp));
  manager.SetAttributeWithNumberValue(menu_node_, NODE_POSITION, menu_left,
                                      menu_top);
  if (dialog_state_ == DialogState::kShown) {
    return true;
  }

  dialog_api->setContentAlignment(dialog_, ARKUI_ALIGNMENT_TOP_START,
                                  root_offset.x / density,
                                  root_offset.y / density);
  if (callbacks_.on_overlay_will_show) {
    callbacks_.on_overlay_will_show();
  }
  if (dialog_api->show(dialog_, false) != 0) {
    return false;
  }
  dialog_state_ = DialogState::kShown;
  return true;
}

void TextSelectionMenuManager::HandleAction(Action action) {
  switch (action) {
    case Action::kCopy: {
      const std::string selected_text = callbacks_.get_selected_text
                                            ? callbacks_.get_selected_text()
                                            : std::string();
      if (CopyTextSelectionToPasteboard(selected_text)) {
        if (callbacks_.on_copy_succeeded) {
          callbacks_.on_copy_succeeded();
        }
      } else {
        Hide();
      }
      break;
    }
    case Action::kSelectAll:
      if (callbacks_.on_select_all) {
        callbacks_.on_select_all();
      }
      break;
    case Action::kDismiss:
      if (callbacks_.on_dismiss) {
        callbacks_.on_dismiss();
      }
      break;
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
