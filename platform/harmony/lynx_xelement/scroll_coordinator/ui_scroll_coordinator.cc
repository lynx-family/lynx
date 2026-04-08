// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/scroll_coordinator/ui_scroll_coordinator.h"

#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/float_comparison.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIScrollCoordinatorToolBar::UIScrollCoordinatorToolBar(LynxContext* context,
                                                       int sign,
                                                       const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

void UIScrollCoordinatorToolBar::UpdateLayout(
    float left, float top, float width, float height, const float* paddings,
    const float* margins, const float* sticky, float max_height,
    uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (fold_view_ != nullptr) {
    fold_view_->UpdateFoldViewLayout();
  }
}

void UIScrollCoordinatorToolBar::GetOriginRect(float origin_rect[4]) {
  UIView::GetOriginRect(origin_rect);
  origin_rect[1] = 0;
}

float UIScrollCoordinatorToolBar::OffsetYForCalcPosition() {
  return fold_view_ ? -fold_view_->ScrollY() : 0;
}

UIScrollCoordinatorToolBar::~UIScrollCoordinatorToolBar() {
  fold_view_ = nullptr;
}

UIScrollCoordinatorHeader::UIScrollCoordinatorHeader(LynxContext* context,
                                                     int sign,
                                                     const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

void UIScrollCoordinatorHeader::UpdateLayout(
    float left, float top, float width, float height, const float* paddings,
    const float* margins, const float* sticky, float max_height,
    uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (fold_view_ != nullptr) {
    fold_view_->UpdateFoldViewLayout();
  }
}

void UIScrollCoordinatorHeader::GetOriginRect(float origin_rect[4]) {
  UIView::GetOriginRect(origin_rect);
  origin_rect[1] = 0;
}

UIScrollCoordinatorHeader::~UIScrollCoordinatorHeader() {
  fold_view_ = nullptr;
}

UIScrollCoordinatorSlot::UIScrollCoordinatorSlot(LynxContext* context, int sign,
                                                 const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

void UIScrollCoordinatorSlot::UpdateLayout(float left, float top, float width,
                                           float height, const float* paddings,
                                           const float* margins,
                                           const float* sticky,
                                           float max_height,
                                           uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (fold_view_ != nullptr) {
    fold_view_->UpdateFoldViewLayout();
  }
}

void UIScrollCoordinatorSlot::GetOriginRect(float origin_rect[4]) {
  UIView::GetOriginRect(origin_rect);
  origin_rect[1] = fold_view_->GetHeaderHeight();
}

UIScrollCoordinatorSlot::~UIScrollCoordinatorSlot() { fold_view_ = nullptr; }

UIScrollCoordinatorSlotDrag::UIScrollCoordinatorSlotDrag(LynxContext* context,
                                                         int sign,
                                                         const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
}

UIScrollCoordinator::UIScrollCoordinator(LynxContext* context, int sign,
                                         const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  overflow_ = {false, false};
  foldview_ = NodeManager::Instance().CreateNode(ARKUI_NODE_SCROLL);
  NodeManager::Instance().SetAttributeWithNumberValue(
      foldview_, NODE_ALIGNMENT,
      static_cast<int32_t>(ARKUI_ALIGNMENT_TOP_START));
  NodeManager::Instance().SetAttributeWithNumberValue(
      foldview_, NODE_SCROLL_SCROLL_DIRECTION,
      static_cast<int32_t>(ARKUI_SCROLL_DIRECTION_VERTICAL));

  NodeManager::Instance().SetAttributeWithNumberValue(
      foldview_, NODE_SCROLL_NESTED_SCROLL,
      static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY),
      static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY));
  NodeManager::Instance().SetAttributeWithNumberValue(
      foldview_, NODE_SCROLL_BAR_DISPLAY_MODE,
      static_cast<int32_t>(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF));
  NodeManager::Instance().InsertNode(Node(), foldview_, 0);

  column_ = NodeManager::Instance().CreateNode(ARKUI_NODE_COLUMN);

  NodeManager::Instance().InsertNode(foldview_, column_, 0);
  NodeManager::Instance().AddNodeEventReceiver(foldview_,
                                               UIBase::EventReceiver);
  for (auto eventType : kFoldViewNodeEventTypes) {
    NodeManager::Instance().RegisterNodeEvent(foldview_, eventType, this);
  }
}

bool UIScrollCoordinator::IsToolbarTag(const std::string& tag) const {
  return tag == "scroll-coordinator-toolbar";
}

bool UIScrollCoordinator::IsHeaderTag(const std::string& tag) const {
  return tag == "scroll-coordinator-header";
}

bool UIScrollCoordinator::IsSlotTag(const std::string& tag) const {
  return tag == "scroll-coordinator-slot";
}

std::string UIScrollCoordinator::RequiredChildrenErrorMessage() const {
  return "scroll-coordinator-header and scroll-coordinator-slot must be set !";
}

UIScrollCoordinator::~UIScrollCoordinator() {
  for (auto eventType : kFoldViewNodeEventTypes) {
    NodeManager::Instance().UnregisterNodeEvent(foldview_, eventType);
  }

  NodeManager::Instance().RemoveNodeEventReceiver(foldview_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().DisposeNode(foldview_);
  NodeManager::Instance().DisposeNode(column_);

  foldview_ = nullptr;
  column_ = nullptr;
  header_ = nullptr;
  slot_ = nullptr;
  toolbar_ = nullptr;
}

bool UIScrollCoordinator::IsAtBorder(bool isStart) {
  if (isStart) {
    return ScrollY() <= 0;
  } else {
    return ScrollY() >= fold_distance_;
  }
}

// for gesture
std::vector<float> UIScrollCoordinator::GestureScrollBy(float delta_x,
                                                        float delta_y) {
  std::vector<float> res(4, 0);
  auto last_scroll_offset = ScrollY();
  float scroll_y = last_scroll_offset + delta_y;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, 0, scroll_y <= 0 ? 0 : scroll_y, 0,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
  res[0] = 0;
  res[1] = ScrollY() - last_scroll_offset;
  res[2] = delta_x;
  res[3] = delta_y - res[1];
  return res;
}

// for worklet
std::vector<float> UIScrollCoordinator::ScrollBy(float delta_x, float delta_y) {
  GestureRecognized();
  return GestureScrollBy(delta_x, delta_y);
}

void UIScrollCoordinator::OnPropUpdate(const std::string& name,
                                       const lepus::Value& value) {
  UIView::OnPropUpdate(name, value);
  if (name == "granularity") {
    granularity_ = value.Number();
  } else if (name == "enable-scroll" || name == "scroll-enable") {
    NodeManager::Instance().SetAttributeWithNumberValue(
        foldview_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION,
        static_cast<int32_t>(value.Bool()));
  } else if (name == "header-over-slot") {
    header_over_slot_ = value.Bool();
  }
}

void UIScrollCoordinator::InsertNode(UIBase* child, int index) {
  if (IsToolbarTag(child->Tag())) {
    NodeManager::Instance().InsertNode(Node(), child->DrawNode(), 1);
    static_cast<UIScrollCoordinatorToolBar*>(child)->fold_view_ = this;
    toolbar_ = child;
  } else if (IsHeaderTag(child->Tag())) {
    NodeManager::Instance().InsertNode(column_, child->DrawNode(), index);
    static_cast<UIScrollCoordinatorHeader*>(child)->fold_view_ = this;
    header_ = child;
  } else if (IsSlotTag(child->Tag())) {
    NodeManager::Instance().InsertNode(column_, child->DrawNode(), index);
    static_cast<UIScrollCoordinatorSlot*>(child)->fold_view_ = this;
    slot_ = child;
  } else {
    auto error = lynx::base::LynxError(
        error::E_EXCEPTION_PLATFORM,
        "a " + child->Tag() + " can not be added to the " + Tag() + " !", "",
        base::LynxErrorLevel::Error);
    lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
  }
}

void UIScrollCoordinator::RemoveNode(UIBase* child) {
  if (IsToolbarTag(child->Tag())) {
    NodeManager::Instance().RemoveNode(Node(), child->DrawNode());
    toolbar_ = nullptr;
  } else if (IsHeaderTag(child->Tag())) {
    NodeManager::Instance().RemoveNode(column_, child->DrawNode());
    header_ = nullptr;
  } else if (IsSlotTag(child->Tag())) {
    NodeManager::Instance().RemoveNode(column_, child->DrawNode());
    slot_ = nullptr;
  }
}

void UIScrollCoordinator::UpdateLayout(float left, float top, float width,
                                       float height, const float* paddings,
                                       const float* margins,
                                       const float* sticky, float max_height,
                                       uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_FOLD_VIEW_UPDATE_LAYOUT);
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
}

void UIScrollCoordinator::UpdateFoldViewLayout() {
  if (slot_ != nullptr && header_ != nullptr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_FOLD_VIEW_UPDATE_FOLD_VIEW_LAYOUT);
    NodeManager::Instance().SetAttributeWithNumberValue(header_->DrawNode(),
                                                        NODE_POSITION, 0, 0);
    NodeManager::Instance().SetAttributeWithNumberValue(
        slot_->DrawNode(), NODE_POSITION, 0, header_->height_);
    fold_distance_ = header_->height_ - (toolbar_ ? toolbar_->height_ : 0);

    int32_t toolbar_height = toolbar_ ? toolbar_->height_ : 0;

    fold_distance_ = header_->height_ - toolbar_height;

    NodeManager::Instance().SetAttributeWithNumberValue(
        column_, NODE_HEIGHT, slot_->height_ + header_->height_);

    const int current_offset = NodeManager::Instance().GetAttribute<float>(
        foldview_, NODE_SCROLL_OFFSET, 1);

    if (current_offset > fold_distance_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          foldview_, NODE_SCROLL_OFFSET, fold_distance_);
    }
  }
}

float UIScrollCoordinator::GetHeaderHeight() const {
  if (header_ != nullptr) {
    return header_->height_;
  }
  return 0;
}

bool UIScrollCoordinator::IsScrollable() { return true; }

float UIScrollCoordinator::ScrollY() {
  return NodeManager::Instance().GetAttribute<float>(foldview_,
                                                     NODE_SCROLL_OFFSET, 1);
}

void UIScrollCoordinator::OnNodeReady() {
  UIView::OnNodeReady();
  if (header_ == nullptr || slot_ == nullptr) {
    auto error = lynx::base::LynxError(error::E_EXCEPTION_PLATFORM,
                                       RequiredChildrenErrorMessage(), "",
                                       base::LynxErrorLevel::Error);
    lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
    return;
  }
  if (header_over_slot_) {
    NodeManager::Instance().RemoveNode(column_, header_->DrawNode());
    NodeManager::Instance().InsertNodeAfter(column_, header_->DrawNode(),
                                            slot_->DrawNode());
  } else {
    NodeManager::Instance().RemoveNode(column_, slot_->DrawNode());
    NodeManager::Instance().InsertNodeAfter(column_, slot_->DrawNode(),
                                            header_->DrawNode());
  }

  NodeManager::Instance().SetAttributeWithNumberValue(foldview_, NODE_WIDTH,
                                                      width_);
  NodeManager::Instance().SetAttributeWithNumberValue(foldview_, NODE_HEIGHT,
                                                      height_);
  NodeManager::Instance().SetAttributeWithNumberValue(column_, NODE_WIDTH,
                                                      width_);
  if (toolbar_ != nullptr) {
    NodeManager::Instance().SetAttributeWithNumberValue(toolbar_->DrawNode(),
                                                        NODE_POSITION, 0, 0);
  }

  UpdateFoldViewLayout();

  if (IsEnableNewGesture()) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        foldview_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, false);
    auto map = UIBase::GetGestureDetectorMap();
    if (!map.empty()) {
      for (const auto& entry : map) {
        if (entry.second->gesture_type() == GestureType::NATIVE) {
          NodeManager::Instance().SetAttributeWithNumberValue(
              foldview_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, true);
        }
      }
    }
  }
}

EventTarget* UIScrollCoordinator::HitTest(float point[2]) {
  EventTarget* target = nullptr;
  float child_point[] = {point[0], point[1]};
  float target_point[2] = {0}, scroll[2] = {0}, target_origin_rect[4] = {0};

  std::vector<UIBase*> hit_test_children;
  if (toolbar_ != nullptr) {
    hit_test_children.push_back(toolbar_);
  }
  if (header_over_slot_) {
    hit_test_children.push_back(header_);
    hit_test_children.push_back(slot_);
  } else {
    hit_test_children.push_back(slot_);
    hit_test_children.push_back(header_);
  }

  float current_offset = NodeManager::Instance().GetAttribute<float>(
      foldview_, NODE_SCROLL_OFFSET, 1);

  for (auto ui : hit_test_children) {
    if (!ui->ShouldHitTest()) {
      continue;
    }
    scroll[1] = ui == toolbar_ ? 0 : current_offset;

    ui->GetOriginRect(target_origin_rect);

    GetTargetPoint(target_point, point, scroll, target_origin_rect,
                   ui->GetTransform());
    if (ui->ContainsPoint(target_point)) {
      if (ui->IsOnResponseChain()) {
        target = ui;
        child_point[0] = target_point[0];
        child_point[1] = target_point[1];
        break;
      }
      if (target == nullptr) {
        target = ui;
        child_point[0] = target_point[0];
        child_point[1] = target_point[1];
      }
    }
  }

  if (target == nullptr) {
    return this;
  }
  const auto ret = target->HitTest(child_point);
  UIBase* target_ui = static_cast<UIBase*>(ret->FirstUITarget());
  UIBase* first_vertical_child = nullptr;

  while (target_ui != this) {
    if (target_ui->IsVerticalScrollView()) {
      first_vertical_child = target_ui;
    }
    target_ui = target_ui->Parent();
  }

  if (first_vertical_child != nullptr) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        first_vertical_child->Node(), NODE_SCROLL_NESTED_SCROLL,
        static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST),
        static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_SELF_FIRST));
  }

  return ret;
}

bool UIScrollCoordinator::CanConsumeGesture(float deltaX, float deltaY) {
  if ((IsAtBorder(true) && deltaY < 0) || (IsAtBorder(false) && deltaY > 0)) {
    return false;
  } else {
    return true;
  }
}

std::unordered_map<std::string, UIScrollCoordinator::UIMethod>
    UIScrollCoordinator::foldview_ui_method_map_ = {
        {"setFoldExpanded", &UIScrollCoordinator::SetFoldExpanded},
        {"scrollBy", &UIScrollCoordinator::ScrollByMethod}};

void UIScrollCoordinator::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = foldview_ui_method_map_.find(method);
      it != foldview_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIScrollCoordinator::ScrollByMethod(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!args.IsTable()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("params is not object!"));
    return;
  }
  float offset{0};
  auto table = args.Table();
  for (const auto& [k, v] : *table) {
    if (k.IsEqual("offset") && v.IsNumber()) {
      offset = v.Number();
    }
  }
  std::vector<float> result = ScrollBy(offset, offset);
  auto dictionary = lepus::Dictionary::Create();
  if (result.size() >= 4) {
    dictionary->SetValue("consumedX", result[0]);
    dictionary->SetValue("consumedY", result[1]);
    dictionary->SetValue("unconsumedX", result[2]);
    dictionary->SetValue("unconsumedY", result[3]);
  }
  callback(LynxGetUIResult::SUCCESS, lepus_value(std::move(dictionary)));
}

void UIScrollCoordinator::SetFoldExpanded(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (args.IsTable()) {
    const auto params = args.Table();
    const lepus::Value& offset_value = params->GetValue("offset");
    float screen_size[2] = {0};
    context_->ScreenSize(screen_size);
    const int32_t width_index = 0;
    if (offset_value.IsString()) {
      float offset = LynxUnitUtils::ToVPFromUnitValue(
          offset_value.StdString(), screen_size[width_index],
          context_->DevicePixelRatio());
      if (base::FloatsLarger(offset, fold_distance_)) {
        offset = fold_distance_;
      } else if (base::FloatsLarger(0, offset)) {
        offset = 0;
      }
      const lepus::Value& smooth_value = params->GetValue("smooth");
      bool smooth = smooth_value.IsBool() ? smooth_value.Bool() : true;

      ArkUI_AttributeItem item;
      ArkUI_NumberValue scroll_args_[] = {
          {.f32 = 0},
          {.f32 = offset},
          {.i32 = smooth ? 250 : 0},
          {.i32 = ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH},
          {.i32 = 0}};
      item = {scroll_args_, sizeof(scroll_args_) / sizeof(ArkUI_NumberValue)};
      NodeManager::Instance().SetAttribute(foldview_, NODE_SCROLL_OFFSET,
                                           &item);
      callback(LynxGetUIResult::SUCCESS, lepus::Value());
    } else {
      auto ret = lepus::Dictionary::Create();
      ret->SetValue("err", "offset must be a string!");
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
    }
  } else {
    auto ret = lepus::Dictionary::Create();
    ret->SetValue("err", "index is not assigned");
    callback(LynxGetUIResult::PARAM_INVALID, lepus::Value(ret));
  }
}

void UIScrollCoordinator::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIView::OnNodeEvent(event);
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  if (type == NODE_SCROLL_EVENT_ON_SCROLL) {
    GestureRecognized();
    context_->NotifyUIScroll();
    const float current_offset = NodeManager::Instance().GetAttribute<float>(
        foldview_, NODE_SCROLL_OFFSET, 1);

    if (current_offset > fold_distance_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          foldview_, NODE_SCROLL_OFFSET, fold_distance_);
    } else {
      if (abs(current_offset - pre_offset_) >= fold_distance_ * granularity_ ||
          (current_offset != pre_offset_ &&
           (current_offset == 0 || abs(current_offset - fold_distance_) < 1))) {
        auto param = lepus::Dictionary::Create();
        param->SetValue("offset", current_offset);
        param->SetValue("height", fold_distance_);
        CustomEvent custom_event{Sign(), "offset", "detail",
                                 lepus_value(param)};
        context_->SendEvent(custom_event);
        pre_offset_ = current_offset;
      }
    }
  } else if (type == NODE_SCROLL_EVENT_ON_WILL_SCROLL) {
    if (IsEnableNewGesture() && !consume_gesture_) {
      auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
      component_event->data[0].f32 = 0.f;
      component_event->data[1].f32 = 0.f;
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
