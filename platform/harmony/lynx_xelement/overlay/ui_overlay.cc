// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/overlay/ui_overlay.h"

#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <deviceinfo.h>

#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/config.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {

static std::vector<UIOverlay*>
    global_dialogs;  // Due to API restrictions, only global dialogs can be used
                     // for OnDismiss event
static constexpr int kDialogLevelModeSupportVersion = 15;

UIBase* UIOverlay::Make(LynxContext* context, int sign,
                        const std::string& tag) {
  return new UIOverlay(context, sign, tag);
}

void UIOverlay::OnPropUpdate(const std::string& name,
                             const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (name == "visible") {
    is_visible_props_ = value.Bool();
  } else if (name == "events-pass-through") {
    is_event_pass_through_ = value.Bool();
  } else if (name == "mode") {
    harmony_level_mode_ = ARKUI_LEVEL_MODE_OVERLAY;
    if (value.IsString()) {
      auto mode = value.StdString();
      if (mode == "page") {
        harmony_level_mode_ = ARKUI_LEVEL_MODE_EMBEDDED;
      }
    }
  }
}

void UIOverlay::ApplyLevelMode() {
  if (!native_dialog_) {
    return;
  }
  if (OH_GetSdkApiVersion() < kDialogLevelModeSupportVersion) {
    return;
  }
  auto* dialog_api = NodeManager::DialogInstance2();
  if (!dialog_api) {
    return;
  }
  dialog_api->setLevelMode(native_dialog_, harmony_level_mode_);
}

void UIOverlay::ShowDialog(bool show_dialog) {
  if (show_dialog) {
    if (!is_show_ && native_dialog_) {
      ApplyLevelMode();
      NodeManager::DialogInstance()->setContent(native_dialog_, stack_);
      NodeManager::DialogInstance()->show(native_dialog_, false);
      CustomEvent event{Sign(), "showoverlay", "detail", lepus_value()};
      context_->SendEvent(event);
      context_->GetUIOwner()->UpdateRootTarget(this);
      is_show_ = true;
    } else {
      LOGE("overlay show skipped: is_show_=" << is_show_ << ", native_dialog_="
                                             << native_dialog_);
    }
  } else {
    if (is_show_ && native_dialog_) {
      is_show_ = false;
      RestoreRootTarget();
      NodeManager::DialogInstance()->setContent(native_dialog_, nullptr);
      NodeManager::DialogInstance()->close(native_dialog_);
      CustomEvent event{Sign(), "dismissoverlay", "detail", lepus_value()};
      context_->SendEvent(event);
      is_show_ = false;
    } else {
      LOGE("overlay dismiss skipped: is_show_="
           << is_show_ << ", native_dialog_=" << native_dialog_);
    }
  }
}

UIOverlay::~UIOverlay() {
  LOGI("overlay destruction sign=" << Sign() << " this="
                                   << static_cast<const void*>(this));
  if (are_gestures_attached_) {
    context_->DetachGesturesFromRoot(this);
    are_gestures_attached_ = false;
  }
  if (native_dialog_ != nullptr) {
    LOGI("overlay destruction close dialog sign="
         << Sign() << " this=" << static_cast<const void*>(this)
         << " dialog=" << static_cast<const void*>(native_dialog_));
    NodeManager::DialogInstance()->setContent(native_dialog_, nullptr);
    NodeManager::DialogInstance()->close(native_dialog_);
    NodeManager::DialogInstance()->dispose(native_dialog_);
    native_dialog_ = nullptr;
  }
  auto it = std::find(global_dialogs.begin(), global_dialogs.end(), this);
  if (it != global_dialogs.end()) {
    global_dialogs.erase(it);
  }

  NodeManager::Instance().RemoveNodeEventReceiver(stack_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().RemoveNodeCustomEventReceiver(
      stack_, UIBase::CustomEventReceiver);
  NodeManager::Instance().UnregisterNodeEvent(stack_, NODE_EVENT_ON_ATTACH);
  NodeManager::Instance().UnregisterNodeEvent(stack_, NODE_EVENT_ON_DETACH);
  NodeManager::Instance().UnregisterNodeEvent(stack_, NODE_ON_TOUCH_INTERCEPT);
  NodeManager::Instance().UnregisterNodeEvent(stack_, NODE_TOUCH_EVENT);
  NodeManager::Instance().DisposeNode(stack_);
  stack_ = nullptr;
}

void UIOverlay::RemoveNode(UIBase* child) {
  if (!child || !stack_) {
    return;
  }
  auto draw = child->DrawNode();
  if (!draw) {
    return;
  }
  NodeManager::Instance().RemoveNode(stack_, draw);
}

UIOverlay::UIOverlay(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {
  LOGI("overlay construction sign=" << Sign() << " tag=" << tag << " this="
                                    << static_cast<const void*>(this));
  stack_ = NodeManager::Instance().CreateNode(ARKUI_NODE_STACK);
  NodeManager::Instance().SetAttributeWithNumberValue(
      stack_, NODE_HIT_TEST_BEHAVIOR,
      static_cast<int32_t>(ARKUI_HIT_TEST_MODE_TRANSPARENT));
  NodeManager::Instance().RegisterNodeEvent(stack_, NODE_TOUCH_EVENT, this);
  NodeManager::Instance().AddNodeEventReceiver(stack_, UIBase::EventReceiver);
  NodeManager::Instance().AddNodeCustomEventReceiver(
      stack_, UIBase::CustomEventReceiver);
  NodeManager::Instance().RegisterNodeEvent(stack_, NODE_EVENT_ON_ATTACH, this);
  NodeManager::Instance().RegisterNodeEvent(stack_, NODE_EVENT_ON_DETACH, this);
  NodeManager::Instance().RegisterNodeEvent(stack_, NODE_ON_TOUCH_INTERCEPT,
                                            this);
  native_dialog_ = NodeManager::DialogInstance()->create();
  LOGI("overlay construction create dialog sign="
       << Sign() << " tag=" << tag << " this=" << static_cast<const void*>(this)
       << " dialog=" << static_cast<const void*>(native_dialog_));
  NodeManager::DialogInstance()->enableCustomStyle(native_dialog_, true);
  NodeManager::DialogInstance()->setAutoCancel(native_dialog_, false);
  NodeManager::DialogInstance()->setModalMode(native_dialog_, true);
  NodeManager::DialogInstance()->enableCustomAnimation(native_dialog_, true);
  NodeManager::DialogInstance()->setMask(native_dialog_, 0x00000000, nullptr);
  NodeManager::DialogInstance()->setContentAlignment(
      native_dialog_, ARKUI_ALIGNMENT_TOP_START, 0.f, 0.f);
  ApplyLevelMode();
  // register onWillDismiss event
  NodeManager::DialogInstance()->registerOnWillDismiss(
      native_dialog_, [](int32_t reason) -> bool {
        if (!global_dialogs.empty()) {
          auto dialog = global_dialogs.back();
          dialog->onRequestClose();
          return false;
        }
        return true;
      });

  global_dialogs.push_back(this);
}

void UIOverlay::UpdateLayout(float left, float top, float width, float height,
                             const float* paddings, const float* margins,
                             const float* sticky, float max_height,
                             uint32_t node_index) {
  const float screen_width =
      NodeManager::GetScreenWidth() / context_->ScaledDensity();
  const float screen_height =
      NodeManager::GetScreenHeight() / context_->ScaledDensity();
  NodeManager::Instance().SetAttributeWithNumberValue(stack_, NODE_HEIGHT,
                                                      screen_height);
  NodeManager::Instance().SetAttributeWithNumberValue(stack_, NODE_WIDTH,
                                                      screen_width);
  UIBase::UpdateLayout(0, 0, screen_width, screen_height, paddings, margins,
                       sticky, max_height, node_index);
}

void UIOverlay::FrameDidChanged() {
  // overlay does not have any size
}

void UIOverlay::OnNodeEvent(ArkUI_NodeEvent* event) {
  if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_ON_TOUCH_INTERCEPT) {
    context_->UpdateNativeInteractionEnabledForTree(this);
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_TOUCH_EVENT) {
    if (ArkUI_UIInputEvent* touch_event =
            OH_ArkUI_NodeEvent_GetInputEvent(event);
        OH_ArkUI_UIInputEvent_GetAction(touch_event) ==
        UI_TOUCH_EVENT_ACTION_DOWN) {
      size_t num = OH_ArkUI_PointerEvent_GetPointerCount(touch_event);
      for (size_t i = 0; i < num; ++i) {
        if (OH_ArkUI_PointerEvent_GetPointerId(touch_event, i) == 0) {
          consume_event_self_ = false;
          child_event_through_ = false;
          float page_point[2] = {
              OH_ArkUI_PointerEvent_GetXByIndex(touch_event, i),
              OH_ArkUI_PointerEvent_GetYByIndex(touch_event, i)};
          auto target = UIBase::HitTest(page_point);
          consume_event_self_ = target == this;
          float target_point[2] = {page_point[0], page_point[1]};
          UIBase* target_ui =
              target->HasUI() ? static_cast<UIBase*>(target)
                              : static_cast<UIBase*>(target->FirstUITarget());
          LynxUIHelper::ConvertPointFromAncestorToDescendant(
              target_point, this, target_ui, page_point);
          child_event_through_ = target->EventThrough(target_point);
          break;
        }
      }
    }
    if (child_event_through_ ||
        (is_event_pass_through_ && consume_event_self_)) {
      context_->OnTouchEvent(OH_ArkUI_NodeEvent_GetInputEvent(event),
                             context_->Root(), true);
    } else {
      context_->OnTouchEvent(OH_ArkUI_NodeEvent_GetInputEvent(event), this);
    }
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_ATTACH) {
    is_root_attached_ = true;
    context_->ResumeExposure();
  } else if (OH_ArkUI_NodeEvent_GetEventType(event) == NODE_EVENT_ON_DETACH) {
    is_root_attached_ = false;
    auto dict = lepus::Dictionary::Create();
    dict->SetValue("sendEvent", false);
    context_->StopExposure(lepus::Value(dict));
  }
}

void UIOverlay::onRequestClose() {
  CustomEvent event{Sign(), "requestclose", "detail", lepus_value()};
  context_->SendEvent(event);
}

bool UIOverlay::ShouldHitTest() { return false; }

bool UIOverlay::EventThrough(float point[2]) {
  bool is_event_through = false;
  if (event_through_ == LynxEventPropStatus::kEnable) {
    is_event_through = true;
  }

  if (event_through_active_regions_.empty()) {
    return is_event_through;
  }

  bool is_hit_event_through_active_regions = false;
  for (const auto& region : event_through_active_regions_) {
    float x = region[0].GetValue(width_);
    float y = region[1].GetValue(height_);
    float w = region[2].GetValue(width_);
    float h = region[3].GetValue(height_);
    is_hit_event_through_active_regions =
        base::FloatsLargerOrEqual(point[0], x) &&
        base::FloatsLargerOrEqual(x + w, point[0]) &&
        base::FloatsLargerOrEqual(point[1], y) &&
        base::FloatsLargerOrEqual(y + h, point[1]);
    if (is_hit_event_through_active_regions) {
      break;
    }
  }
  return is_hit_event_through_active_regions ? is_event_through
                                             : !is_event_through;
}

bool UIOverlay::IgnoreFocus() {
  if (ignore_focus_ == LynxEventPropStatus::kEnable) {
    return true;
  }
  return false;
}

LynxPointerEventsValue UIOverlay::PointerEvents() {
  if (pointer_events_ == LynxPointerEventsValue::kNone) {
    return pointer_events_;
  }

  return LynxPointerEventsValue::kAuto;
}

void UIOverlay::InsertNode(UIBase* child, int index) {
  NodeManager::Instance().InsertNode(stack_, child->DrawNode(), index);
}

void UIOverlay::OnNodeReady() {
  UIBase::OnNodeReady();

  if (Parent() == nullptr) {
    ShowDialog(false);
  } else {
    ShowDialog(is_visible_props_);
  }

  if (!are_gestures_attached_) {
    context_->AttachGesturesToRoot(this);
    are_gestures_attached_ = true;
  }
}

void UIOverlay::RestoreRootTarget() {
  if (!context_) {
    return;
  }
  auto* ui_owner = context_->GetUIOwner();
  if (!ui_owner) {
    return;
  }

  UIOverlay* top = nullptr;
  for (auto it = global_dialogs.rbegin(); it != global_dialogs.rend(); ++it) {
    if (*it != this && (*it)->IsVisible()) {
      top = *it;
      break;
    }
  }

  if (top) {
    ui_owner->UpdateRootTarget(top);
  } else {
    if (auto* root = context_->Root()) {
      ui_owner->UpdateRootTarget(root);
    }
  }
}

bool UIOverlay::IsVisible() { return is_root_attached_ && UIBase::IsVisible(); }

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
