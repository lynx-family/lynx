// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

#include <arkui/native_node.h>
#include <multimedia/image_framework/image/image_packer_native.h>

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_function_loader.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/transform.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace tasm {
namespace harmony {

constexpr uint32_t kFlagFrameChanged = 1;
constexpr uint32_t kFlagFrameSizeChanged = 1 << 1;
constexpr uint32_t kFlagTransformChanged = 1 << 2;
constexpr uint32_t kFlagRadiusChanged = 1 << 3;
constexpr uint32_t kFlagOverflowChanged = 1 << 4;
constexpr uint32_t kFlagBackgroundChanged = 1 << 5;
constexpr uint32_t kFlagTransformOriginChanged = 1 << 6;
constexpr uint32_t kFlagClipPathChanged = 1 << 7;
constexpr uint32_t kFlagIdChanged = 1 << 8;
constexpr uint32_t kFlagAccessibilityLabel = 1 << 9;
constexpr uint32_t kFlagAccessibilityTraits = 1 << 10;
constexpr uint32_t kFlagAccessibilityMode = 1 << 11;
constexpr uint32_t kFlagAccessibilityExclusive = 1 << 12;
constexpr uint32_t kFlagBackgroundColor = 1 << 13;

// std::sqrt(5.0) is not constexpr in C++17, so we use an approximation
// to allow this value to be used in compile-time constants.
// This value (~2.236068) is accurate enough for current use cases.
static constexpr float kCameraDistanceNormalizationMultiplier = 2.236068;

std::unordered_map<std::string, UIBase::PropSetter> UIBase::prop_setters_ = {
    {"idSelector", &UIBase::SetIdSelector},
    {"react-ref", &UIBase::SetReactRef},
    {"background-color", &UIBase::SetBackgroundColor},
    {"transform", &UIBase::SetTransform},
    {"transform-origin", &UIBase::SetTransformOrigin},
    {"border-top-left-radius", &UIBase::SetBorderTopLeftRadius},
    {"border-top-right-radius", &UIBase::SetBorderTopRightRadius},
    {"border-bottom-right-radius", &UIBase::SetBorderBottomRightRadius},
    {"border-bottom-left-radius", &UIBase::SetBorderBottomLeftRadius},
    {"border-radius", &UIBase::SetBorderRadius},
    {"overflow-x", &UIBase::SetOverflowX},
    {"overflow-y", &UIBase::SetOverflowY},
    {"overflow", &UIBase::SetOverflow},
    {"opacity", &UIBase::SetOpacity},
    {"visibility", &UIBase::SetVisibility},
    {"image-rendering", &UIBase::SetImageRendering},
    {"border-left-width", &UIBase::SetBorderLeftWidth},
    {"border-right-width", &UIBase::SetBorderRightWidth},
    {"border-top-width", &UIBase::SetBorderTopWidth},
    {"border-bottom-width", &UIBase::SetBorderBottomWidth},
    {"border-left-style", &UIBase::SetBorderLeftStyle},
    {"border-top-style", &UIBase::SetBorderTopStyle},
    {"border-bottom-style", &UIBase::SetBorderBottomStyle},
    {"border-right-style", &UIBase::SetBorderRightStyle},
    {"border-top-color", &UIBase::SetBorderTopColor},
    {"border-right-color", &UIBase::SetBorderRightColor},
    {"border-bottom-color", &UIBase::SetBorderBottomColor},
    {"border-left-color", &UIBase::SetBorderLeftColor},
    {"background-clip", &UIBase::SetBackgroundClip},
    {"background-repeat", &UIBase::SetBackgroundRepeat},
    {"background-origin", &UIBase::SetBackgroundOrigin},
    {"background-size", &UIBase::SetBackgroundSize},
    {"background-image", &UIBase::SetBackgroundImage},
    {"background-position", &UIBase::SetBackgroundPosition},
    {"box-shadow", &UIBase::SetBoxShadow},
    {"user-interaction-enabled", &UIBase::SetUserInteractionEnabled},
    {"native-interaction-enabled", &UIBase::SetNativeInteractionEnabled},
    {"hit-slop", &UIBase::SetHitSlop},
    {"ignore-focus", &UIBase::SetIgnoreFocus},
    {"event-through", &UIBase::SetEventThrough},
    {"block-native-event", &UIBase::SetBlockNativeEvent},
    {"consume-slide-event", &UIBase::SetConsumeSlideEvent},
    {"enable-touch-pseudo-propagation",
     &UIBase::SetEnableTouchPseudoPropagation},
    {"filter", &UIBase::SetFilter},
    {"exposure-id", &UIBase::SetExposureID},
    {"exposure-scene", &UIBase::SetExposureScene},
    {"exposure-screen-margin-left", &UIBase::SetExposureScreenMarginLeft},
    {"exposure-screen-margin-right", &UIBase::SetExposureScreenMarginRight},
    {"exposure-screen-margin-top", &UIBase::SetExposureScreenMarginTop},
    {"exposure-screen-margin-bottom", &UIBase::SetExposureScreenMarginBottom},
    {"exposure-ui-margin-left", &UIBase::SetExposureUIMarginLeft},
    {"exposure-ui-margin-right", &UIBase::SetExposureUIMarginRight},
    {"exposure-ui-margin-top", &UIBase::SetExposureUIMarginTop},
    {"exposure-ui-margin-bottom", &UIBase::SetExposureUIMarginBottom},
    {"exposure-area", &UIBase::SetExposureArea},
    {"enable-exposure-ui-clip", &UIBase::SetEnableExposureUIClip},
    {"dataset", &UIBase::SetDataset},
    {"clip-path", &UIBase::SetClipPath},
    {"perspective", &UIBase::SetPerspective},
    {"accessibility-element", &UIBase::SetAccessibilityElement},
    {"accessibility-label", &UIBase::SetAccessibilityLabel},
    {"accessibility-traits", &UIBase::SetAccessibilityTraits},
    {"accessibility-elements-hidden", &UIBase::SetAccessibilityElementsHidden},
    {"accessibility-exclusive-focus", &UIBase::SetAccessibilityExclusiveFocus},
    {"a11y-id", &UIBase::SetAccessibilityId},
    {"enable-cross-language-option", &UIBase::SetCrossLanguageOption}};

std::unordered_map<std::string, UIBase::UIMethod> UIBase::ui_method_map_ = {
    {"boundingClientRect", &UIBase::GetBoundingClientRect},
    {"scrollIntoView", &UIBase::ScrollIntoView},
    {"takeScreenshot", &UIBase::TakeScreenshot},
};

UIBase::UIBase(LynxContext* context, ArkUI_NodeType type, int sign,
               const std::string& tag, bool has_customized_layout)
    : context_(context),
      node_type_(type),
      sign_(sign),
      tag_(tag),
      has_customized_layout_(has_customized_layout) {
  node_ = NodeManager::Instance().CreateNode(type);
  if (node_type_ == ARKUI_NODE_CUSTOM) {
    NodeManager::Instance().AddNodeCustomEventReceiver(
        Node(), UIBase::CustomEventReceiver);
    NodeManager::Instance().RegisterNodeCustomEvent(
        Node(), ARKUI_NODE_CUSTOM_EVENT_ON_DRAW,
        ARKUI_NODE_CUSTOM_EVENT_ON_DRAW, this);
  }
  NodeManager::Instance().AddNodeEventReceiver(Node(), UIBase::EventReceiver);
}

void UIBase::ConsumeGesture(int gesture_id, const lepus::Value& params) {
  if (!IsEnableNewGesture()) {
    return;
  }
  bool inner = params.GetProperty("inner").Bool();
  bool consume = params.GetProperty("consume").Bool();
  if (inner) {
    consume_gesture_ = consume;
  } else {
    gesture_status_ =
        consume ? LynxInterceptGestureStatus::LynxInterceptGestureStateTrue
                : LynxInterceptGestureStatus::LynxInterceptGestureStateFalse;
  }
}

UIBase::~UIBase() {
  if (node_type_ == ARKUI_NODE_CUSTOM) {
    NodeManager::Instance().UnregisterNodeCustomEvent(
        Node(), ARKUI_NODE_CUSTOM_EVENT_ON_DRAW);
    NodeManager::Instance().RemoveNodeCustomEventReceiver(
        Node(), UIBase::CustomEventReceiver);
  }
  NodeManager::Instance().RemoveNodeEventReceiver(Node(),
                                                  UIBase::EventReceiver);
  DetachFromNodeContent();
  NodeManager::Instance().DisposeNode(Node());
  DestroyDrawNode();
  auto manager = GetGestureArenaManager();
  // remove arena member if destroy
  if (manager != nullptr) {
    manager->RemoveMember(weak_from_this());
  }
  // clear gesture map if destroy
  gesture_handlers_.clear();
}

void UIBase::EventReceiver(ArkUI_NodeEvent* event) {
  auto* ui = reinterpret_cast<UIBase*>(OH_ArkUI_NodeEvent_GetUserData(event));
  if (!ui) {
    return;
  }
  ui->OnNodeEvent(event);
}

void UIBase::CustomEventReceiver(ArkUI_NodeCustomEvent* event) {
  UIBase* ui =
      reinterpret_cast<UIBase*>(OH_ArkUI_NodeCustomEvent_GetUserData(event));
  if (!ui) {
    return;
  }
  ArkUI_NodeCustomEventType type = OH_ArkUI_NodeCustomEvent_GetEventType(event);
  if (type == ARKUI_NODE_CUSTOM_EVENT_ON_DRAW) {
    auto event_node = OH_ArkUI_NodeCustomEvent_GetNodeHandle(event);
    ArkUI_DrawContext* context =
        OH_ArkUI_NodeCustomEvent_GetDrawContextInDraw(event);
    ui->OnDraw(reinterpret_cast<OH_Drawing_Canvas*>(
                   OH_ArkUI_DrawContext_GetCanvas(context)),
               event_node);
  } else if (type == ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE) {
    auto layout_constraint =
        OH_ArkUI_NodeCustomEvent_GetLayoutConstraintInMeasure(event);
    ui->OnMeasure(layout_constraint);
  } else if (type == ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT) {
    ui->OnLayout();
  }
}

void UIBase::UpdateLayout(float left, float top, float width, float height,
                          const float* paddings, const float* margins,
                          const float* sticky, float max_height,
                          uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_UPDATE_LAYOUT);
  left_ = left;
  top_ = top;
  padding_left_ = paddings[0];
  padding_top_ = paddings[1];
  padding_right_ = paddings[2];
  padding_bottom_ = paddings[3];
  margin_left_ = margins[0];
  margin_top_ = margins[1];
  margin_right_ = margins[2];
  margin_bottom_ = margins[3];
  if (width_ != width) {
    width_ = width;
    dirty_flags_ |= kFlagFrameSizeChanged;
  }
  if (height_ != height) {
    height_ = height;
    dirty_flags_ |= kFlagFrameSizeChanged;
  }
  dirty_flags_ |= kFlagFrameChanged;
  FrameDidChanged();
  RequestLayout();
  SendLayoutChangeEvent();
}

void UIBase::SendLayoutChangeEvent() {
  if (has_layout_change_event_) {
    float result[4] = {0, 0, width_, height_};
    auto ret = lepus::Dictionary::Create();
    GetBoundingClientRect(result);
    ret->SetValue("id", id_selector_);
    ret->SetValue("left", result[0]);
    ret->SetValue("top", result[1]);
    ret->SetValue("right", result[2]);
    ret->SetValue("bottom", result[3]);
    ret->SetValue("width", result[2] - result[0]);
    ret->SetValue("height", result[3] - result[1]);
    ret->SetValue("dataset", dataset_);
    CustomEvent layout_change_event = {sign_, "layoutchange", "detail",
                                       lepus::Value(std::move(ret))};
    context_->SendEvent(layout_change_event);
  }
}

void UIBase::SetParent(UIBase* parent) { parent_ = parent; }

void UIBase::AddChild(UIBase* child, int index) {
  if (index == -1) {
    children_.emplace_back(child);
  } else {
    children_.insert(children_.begin() + index, child);
  }
  child->SetParent(this);
  InsertNode(child, index);
}

void UIBase::RemoveChild(UIBase* child) {
  child->WillRemoveFromUIParent();
  child->SetParent(nullptr);
  RemoveNode(child);
  children_.erase(std::remove(children_.begin(), children_.end(), child),
                  children_.end());
}

void UIBase::InsertNode(UIBase* child, int index) {
  NodeManager::Instance().InsertNode(Node(), child->DrawNode(), index);
}

void UIBase::RemoveNode(UIBase* child) {
  NodeManager::Instance().RemoveNode(Node(), child->DrawNode());
}

void UIBase::FrameDidChanged() {
  NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_WIDTH,
                                                      width_);
  NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_HEIGHT,
                                                      height_);
  if (!parent_ || !parent_->HasCustomizedLayout()) {
    NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_POSITION,
                                                        left_, top_);
  }
  UpdateDrawNodeFrame();
}

void UIBase::SetGestureDetectors(const GestureMap& gesture_detectors) {
  gesture_detectors_.clear();
  for (const auto& gesture : gesture_detectors) {
    gesture_detectors_.emplace(gesture.first, gesture.second);
  }
  auto manager = GetGestureArenaManager();
  if (manager == nullptr) {
    return;
  }
  if (manager->IsMemberExist(GestureArenaMemberId())) {
    // when update gesture handlers, need to reset it
    gesture_handlers_.clear();
  }
  if (gesture_handlers_.empty() && sign_ > 0) {
    gesture_handlers_ = BaseGestureHandler::ConvertToGestureHandler(
        sign_, context_, weak_from_this(), GetGestureDetectorMap());
  }
  if (manager != nullptr && !manager->IsMemberExist(gesture_arena_member_id_)) {
    gesture_arena_member_id_ = manager->AddMember(weak_from_this());
  }
}

std::shared_ptr<GestureArenaManager> UIBase::GetGestureArenaManager() {
  if (context_ == nullptr || context_->GetUIOwner() == nullptr) {
    return nullptr;
  }
  return context_->GetUIOwner()->GetGestureArenaManager();
}

void UIBase::SetGestureDetectorState(int gesture_id, int state) {
  if (!IsEnableNewGesture()) {
    return;
  }
  std::shared_ptr<GestureArenaManager> gesture_manager =
      GetGestureArenaManager();

  if (gesture_manager == nullptr) {
    return;
  }
  gesture_manager->SetGestureDetectorState(GestureArenaMemberId(), gesture_id,
                                           state);
}

const GestureHandlerMap& UIBase::GetGestureHandlers() {
  // Check if the new gesture feature is enabled
  if (!IsEnableNewGesture()) {
    static const GestureHandlerMap empty_map;
    return empty_map;
  }
  // Lazy initialization of gesture handlers
  if (gesture_handlers_.empty()) {
    gesture_handlers_ = BaseGestureHandler::ConvertToGestureHandler(
        sign_, context_, weak_from_this(), GetGestureDetectorMap());
  }
  // Return the initialized gesture handlers
  return gesture_handlers_;
}

void UIBase::UpdateProps(PropBundleHarmony* props) {
  for (const auto& [id, value] : props->GetProps()) {
    OnPropUpdate(id, value);
  }
}

void UIBase::UpdateSticky(const float* sticky) {
  if (!sticky) {
    return;
  }
  if (parent_) {
    parent_->enable_sticky_ = true;
  }
  sticky_value_ = {sticky[0], sticky[1], sticky[2], sticky[3], 0, 0};
}

bool UIBase::CheckStickyOnParentScroll(float scroll_left, float scroll_top) {
  if (sticky_value_.size() != 6) {
    return false;
  }
  float sticky_left = sticky_value_[0];
  float sticky_top = sticky_value_[1];
  float sticky_right = sticky_value_[2];
  float sticky_bottom = sticky_value_[3];

  float current_left = left_ - scroll_left;
  float current_top = top_ - scroll_top;
  if (base::FloatsLarger(sticky_left, current_left)) {
    // modify x
    sticky_value_[4] = sticky_left - current_left;
  } else {
    if (parent_ == nullptr) {
      return false;
    }

    float parent_width = parent_->width_;
    float cur_r = current_left + width_;
    if (base::FloatsLarger(cur_r + sticky_right, parent_width)) {
      sticky_value_[4] = std::max(parent_width - cur_r - sticky_right,
                                  sticky_left - current_left);
    } else {
      sticky_value_[4] = 0;
    }
  }

  if (base::FloatsLarger(sticky_top, current_top)) {
    // modify y
    sticky_value_[5] = sticky_top - current_top;
  } else {
    if (parent_ == nullptr) {
      return false;
    }
    float parent_height = parent_->height_;
    float current_bottom = current_top + height_;
    if (base::FloatsLarger(current_bottom + sticky_bottom, parent_height)) {
      sticky_value_[5] =
          std::max(parent_height - current_bottom - sticky_bottom,
                   sticky_top - current_top);
    } else {
      sticky_value_[5] = 0;
    }
  }
  return true;
}

void UIBase::OnNodeEvent(ArkUI_NodeEvent* event) {}

void UIBase::RemoveFromParent() {
  if (parent_) {
    parent_->RemoveChild(this);
  }
  parent_ = nullptr;
}

void UIBase::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    PropSetter setter = it->second;
    (this->*setter)(value);
  }
}

void UIBase::OnNodeReady() {
  if (((dirty_flags_ & (kFlagFrameChanged | kFlagBackgroundChanged)) != 0) &&
      background_drawable_) {
    background_drawable_->UpdateBounds(
        0, 0, width_, height_, padding_left_, padding_top_, padding_right_,
        padding_bottom_, context_->ScaledDensity());
    background_drawable_->AdjustBorder();
    Invalidate();
  }
  if ((dirty_flags_ & (kFlagFrameChanged | kFlagFrameSizeChanged |
                       kFlagRadiusChanged | kFlagOverflowChanged)) != 0) {
    ApplyOverflowClip();
    Invalidate();
  }

  if (NeedDrawNode()) {
    InitDrawNode();
  }

  if (basic_shape_ &&
      (dirty_flags_ & (kFlagFrameChanged | kFlagFrameSizeChanged |
                       kFlagClipPathChanged)) != 0) {
    basic_shape_->ParsePathWithParentSize(width_, height_);
    if (!basic_shape_->PathString().empty()) {
      ArkUI_NumberValue value[] = {
          {.i32 = static_cast<int32_t>(ARKUI_CLIP_TYPE_PATH)},
          {.f32 = width_},
          {.f32 = height_}};
      ArkUI_AttributeItem item = {
          .value = value,
          .size = sizeof(value) / sizeof(ArkUI_NumberValue),
          .string = basic_shape_->PathString().c_str(),
      };
      NodeManager::Instance().SetAttribute(DrawNode(), NODE_CLIP_SHAPE, &item);
      Invalidate();
    }
  }
  if ((dirty_flags_ & (kFlagFrameChanged | kFlagTransformOriginChanged)) != 0 &&
      transform_origin_.has_value()) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        DrawNode(), NODE_TRANSFORM_CENTER,
        transform_origin_->x.GetValue(width_),
        transform_origin_->y.GetValue(height_));
  }
  if ((dirty_flags_ & (kFlagFrameSizeChanged | kFlagTransformChanged)) != 0) {
    ApplyTransform();
    Invalidate();
  }

  if ((dirty_flags_ & kFlagIdChanged) != 0) {
    std::string id = context_->OwnerId() + id_selector_;
    ArkUI_AttributeItem item{.string = id.c_str()};
    NodeManager::Instance().SetAttribute(DrawNode(), NODE_ID, &item);
  }
  if ((dirty_flags_ & kFlagBackgroundColor) != 0) {
    if (background_drawable_) {
      if (has_background_color_) {
        NodeManager::Instance().ResetAttribute(DrawNode(),
                                               NODE_BACKGROUND_COLOR);
        has_background_color_ = false;
      }
      background_drawable_->SetBackgroundColor(background_color_);
    } else {
      has_background_color_ = true;
      NodeManager::Instance().SetAttributeWithNumberValue(
          DrawNode(), NODE_BACKGROUND_COLOR, background_color_);
    }
  }

  // Attribute for accessibility
  OnNodeReadyForAccessibility();

  dirty_flags_ = 0;

  if (!new_exposure_props_.empty()) {
    if (old_exposure_props_.empty()) {
      context_->AddUIToExposedMap(this);
    } else {
      if (new_exposure_props_.find("exposure-id") !=
              new_exposure_props_.end() ||
          new_exposure_props_.find("exposure-scene") !=
              new_exposure_props_.end()) {
        context_->AddUIToExposedMap(this);
        context_->RemoveUIFromExposedMap(this);
      }
      context_->TriggerExposureCheck();
    }
    old_exposure_props_ = std::move(new_exposure_props_);
    new_exposure_props_.clear();
  }
}

void UIBase::SetIdSelector(const lepus::Value& value) {
  id_selector_ = value.StdString();
  dirty_flags_ |= kFlagIdChanged;
}

void UIBase::SetReactRef(const lepus::Value& value) {
  react_ref_id_ = value.StdString();
}

void UIBase::SetBackgroundColor(const lepus::Value& value) {
  background_color_ = static_cast<uint32_t>(value.Number());
  dirty_flags_ |= kFlagBackgroundColor;
}

void UIBase::SetOpacity(const lepus::Value& value) {
  double opacity = value.Number();
  if (value.IsNil()) {
    opacity = 1;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(DrawNode(), NODE_OPACITY,
                                                      opacity);
}

void UIBase::SetVisibility(const lepus::Value& value) {
  auto v = static_cast<starlight::VisibilityType>(value.Number());
  if (value.IsNil()) {
    v = starlight::VisibilityType::kVisible;
  }
  ArkUI_Visibility visibility = ARKUI_VISIBILITY_VISIBLE;
  if (v == starlight::VisibilityType::kHidden) {
    visibility = ARKUI_VISIBILITY_HIDDEN;
  } else if (v == starlight::VisibilityType::kNone) {
    visibility = ARKUI_VISIBILITY_NONE;
  }
  NodeManager::Instance().SetAttributeWithNumberValue(
      DrawNode(), NODE_VISIBILITY, static_cast<int32_t>(visibility));
}

void UIBase::SetTransform(const lepus::Value& value) {
  if (value.IsNil()) {
    transform_ = nullptr;
  } else {
    transform_ = std::make_unique<Transform>(value);
  }
  dirty_flags_ |= kFlagTransformChanged;
}

void UIBase::SetTransformOrigin(const lepus::Value& value) {
  if (value.IsNil()) {
    transform_origin_ = std::nullopt;
    NodeManager::Instance().ResetAttribute(DrawNode(), NODE_TRANSFORM_CENTER);
    return;
  }
  if (!value.IsArray() || value.Array()->size() != 4) {
    return;
  }
  const auto& val_arr = value.Array();
  const auto& x_val = val_arr->get(0);
  const auto& x_type =
      static_cast<PlatformLengthType>(val_arr->get(1).Number());
  const auto& y_val = val_arr->get(2);
  const auto& y_type =
      static_cast<PlatformLengthType>(val_arr->get(3).Number());
  transform_origin_ = (TransformOrigin){.x = (PlatformLength){x_val, x_type},
                                        .y = (PlatformLength){y_val, y_type}};

  dirty_flags_ |= kFlagTransformOriginChanged;
}

void UIBase::ApplyTransform() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_APPLY_TRANSFORM);
  if (!transform_) {
    if ((dirty_flags_ & kFlagTransformChanged) != 0) {
      NodeManager::Instance().ResetAttribute(DrawNode(), NODE_TRANSFORM);
      NodeManager::Instance().ResetAttribute(DrawNode(), NODE_Z_INDEX);
      translation_z_ = .0f;
    }
    return;
  }
  if (transform_origin_.has_value()) {
    transform_->SetTransformOrigin(transform_origin_.value());
  }
  auto matrix = transform_->GetTransformMatrix(width_, height_,
                                               context_->ScaledDensity());
  if (!perspective_.IsNil()) {
    float value = GetPerspectiveValue();
    transforms::Matrix44 per_matrix{};
    per_matrix.setRC(3, 2, value);
    matrix.preConcat(per_matrix);
  }
  const float* data = matrix.Data();
  ArkUI_NumberValue xform_value[16];
  for (int i = 0; i < 16; ++i) {
    xform_value[i].f32 = data[i];
  }
  ArkUI_AttributeItem xform =
      (ArkUI_AttributeItem){.value = xform_value, .size = 16};
  NodeManager::Instance().SetAttribute(DrawNode(), NODE_TRANSFORM, &xform);
  translation_z_ = data[Transform::kIndexTranslationZ];
  NodeManager::Instance().SetAttributeWithNumberValue(DrawNode(), NODE_Z_INDEX,
                                                      translation_z_);
}

void UIBase::SetBorderRadius(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderRadius(value);
  dirty_flags_ |= kFlagRadiusChanged;
}

void UIBase::SetBorderTopLeftRadius(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderTopLeftRadius(value);
  dirty_flags_ |= kFlagRadiusChanged;
}

void UIBase::SetBorderTopRightRadius(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderTopRightRadius(value);
  dirty_flags_ |= kFlagRadiusChanged;
}

void UIBase::SetBorderBottomRightRadius(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderBottomRightRadius(value);
  dirty_flags_ |= kFlagRadiusChanged;
}

void UIBase::SetBorderBottomLeftRadius(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderBottomLeftRadius(value);
  dirty_flags_ |= kFlagRadiusChanged;
}

bool UIBase::ComputeOverflowValue(const lepus::Value& value) {
  if (value.IsNil()) {
    return DefaultOverflowValue();
  }
  return static_cast<starlight::OverflowType>(value.Number()) ==
         starlight::OverflowType::kVisible;
}

void UIBase::SetOverflow(const lepus::Value& value) {
  overflow_.overflow_x = overflow_.overflow_y = ComputeOverflowValue(value);
  dirty_flags_ |= kFlagOverflowChanged;
}

void UIBase::SetOverflowX(const lepus::Value& value) {
  overflow_.overflow_x = ComputeOverflowValue(value);
  dirty_flags_ |= kFlagOverflowChanged;
}

void UIBase::SetOverflowY(const lepus::Value& value) {
  overflow_.overflow_y = ComputeOverflowValue(value);
  dirty_flags_ |= kFlagOverflowChanged;
}

void UIBase::ApplyOverflowClip() {
  if (!overflow_.overflow_x && !overflow_.overflow_y) {
    need_clip_ = true;
    if (background_drawable_ && background_drawable_->GetBorderRadius() &&
        !background_drawable_->GetBorderRadius()->IsZero()) {
      if (background_drawable_ &&
          !background_drawable_->GetClipPath().empty()) {
        ArkUI_NumberValue value[] = {
            {.i32 = static_cast<int32_t>(ARKUI_CLIP_TYPE_PATH)},
            {.f32 = width_},
            {.f32 = height_}};
        ArkUI_AttributeItem item = {
            .value = value,
            .size = sizeof(value) / sizeof(ArkUI_NumberValue),
            .string = background_drawable_->GetClipPath().c_str(),
        };
        NodeManager::Instance().SetAttribute(Node(), NODE_CLIP_SHAPE, &item);
      }
    } else {
      NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_CLIP, 1);
    }
  } else if (overflow_.overflow_x && overflow_.overflow_y &&
             (dirty_flags_ & kFlagOverflowChanged) != 0) {
    need_clip_ = false;
    // overflow changed to visible.
    NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_CLIP, 0);
  } else if ((overflow_.overflow_x || overflow_.overflow_y) &&
             (dirty_flags_ & kFlagOverflowChanged) != 0) {
    need_clip_ = true;
    float screen_size[2] = {0};
    context_->ScreenSize(screen_size);
    float clip_width = overflow_.overflow_x ? screen_size[0] : width_;
    float clip_height = overflow_.overflow_y ? screen_size[1] : height_;

    ArkUI_NumberValue value[] = {
        {.i32 = static_cast<int32_t>(ARKUI_CLIP_TYPE_RECTANGLE)},
        {.f32 = clip_width},
        {.f32 = clip_height},
        {.f32 = 0},
        {.f32 = 0}};
    ArkUI_AttributeItem item = {
        .value = value,
        .size = sizeof(value) / sizeof(ArkUI_NumberValue),
    };
    NodeManager::Instance().SetAttribute(Node(), NODE_CLIP_SHAPE, &item);
  }
}

void UIBase::CreateOrUpdateBackground() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_CREATE_OR_UPDATE_BACKGROUND);
  if (!background_drawable_) {
    background_drawable_ =
        std::make_unique<BackgroundDrawable>(std::weak_ptr(shared_from_this()));
  }
  dirty_flags_ |= kFlagBackgroundChanged;
}

void UIBase::RequestLayout() {
  NodeManager::Instance().RequestLayout(DrawNode());
}

void UIBase::Invalidate() { NodeManager::Instance().Invalidate(DrawNode()); }

void UIBase::OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_CREATE_ON_DRAW);
  if (background_drawable_) {
    // The draw function in the UIBase might be triggered by receivers from two
    // different nodes, so drawing is only required when the node returned in
    // the event matches.
    bool need_draw = draw_node_ ? node == draw_node_ : node == Node();
    if (need_draw) {
      background_drawable_->Render(canvas);
    }
  }
}

void UIBase::SetBorderLeftStyle(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderLeftStyle(value);
}

void UIBase::SetBorderRightStyle(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderRightStyle(value);
}

void UIBase::SetBorderTopStyle(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderTopStyle(value);
}

void UIBase::SetBorderBottomStyle(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderBottomStyle(value);
}

void UIBase::SetBorderLeftColor(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderLeftColor(value);
}

void UIBase::SetBorderRightColor(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderRightColor(value);
}

void UIBase::SetBorderTopColor(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderTopColor(value);
}

void UIBase::SetBorderBottomColor(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderBottomColor(value);
}

void UIBase::SetBorderLeftWidth(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderLeftWidth(value);
}

void UIBase::SetBorderRightWidth(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderRightWidth(value);
}

void UIBase::SetBorderTopWidth(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderTopWidth(value);
}

void UIBase::SetBorderBottomWidth(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBorderBottomWidth(value);
}

void UIBase::SetBackgroundClip(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundClip(value);
}

void UIBase::SetBackgroundOrigin(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundOrigin(value);
}

void UIBase::SetBackgroundImage(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundImage(value);
}

void UIBase::SetBackgroundSize(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundSize(value);
}

void UIBase::SetBackgroundPosition(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundPosition(value);
}

void UIBase::SetBackgroundRepeat(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBackgroundRepeat(value);
}

void UIBase::SetBoxShadow(const lepus::Value& value) {
  CreateOrUpdateBackground();
  background_drawable_->SetBoxShadow(value);
}

void UIBase::SetFilter(const lepus::Value& value) {
  if (value.IsNil()) {
    NodeManager::Instance().ResetAttribute(node_, NODE_GRAY_SCALE);
    NodeManager::Instance().ResetAttribute(node_, NODE_BLUR);
    return;
  }
  const auto& val_array = value.Array();
  const starlight::FilterType type =
      static_cast<starlight::FilterType>(value.Array()->get(0).Number());
  double amount = 0;
  switch (type) {
    case starlight::FilterType::kNone:
      NodeManager::Instance().ResetAttribute(node_, NODE_GRAY_SCALE);
      NodeManager::Instance().ResetAttribute(node_, NODE_BLUR);
      break;
    case starlight::FilterType::kGrayscale:
      amount = val_array->get(1).Number();
      NodeManager::Instance().ResetAttribute(node_, NODE_BLUR);
      NodeManager::Instance().SetAttributeWithNumberValue(
          node_, NODE_GRAY_SCALE, amount);
      break;
    case starlight::FilterType::kBlur:
      amount = val_array->get(1).Number();
      NodeManager::Instance().ResetAttribute(node_, NODE_GRAY_SCALE);
      NodeManager::Instance().SetAttributeWithNumberValue(
          node_, NODE_BLUR, amount * context_->ScaledDensity());
      break;
  }
}

void UIBase::SetUserInteractionEnabled(const lepus::Value& value) {
  if (value.IsBool()) {
    user_interaction_enabled_ = value.Bool();
  }
}

void UIBase::SetNativeInteractionEnabled(const lepus::Value& value) {
  if (value.IsBool()) {
    // TODO(hexionghui): Aligned to iOS.
    if (value.Bool()) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          node_, NODE_HIT_TEST_BEHAVIOR,
          static_cast<int32_t>(ARKUI_HIT_TEST_MODE_DEFAULT));
      return;
    }
    NodeManager::Instance().SetAttributeWithNumberValue(
        node_, NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(ARKUI_HIT_TEST_MODE_NONE));
  }
}

void UIBase::SetHitSlop(const lepus::Value& value) {
  float slop = 0;
  if (value.IsString()) {
    slop = ToVPFromUnitValue(value.StdString());
    if (base::FloatsNotEqual(slop, hit_slop_left_) ||
        base::FloatsNotEqual(slop, hit_slop_right_) ||
        base::FloatsNotEqual(slop, hit_slop_top_) ||
        base::FloatsNotEqual(slop, hit_slop_bottom_)) {
      hit_slop_left_ = hit_slop_right_ = hit_slop_top_ = hit_slop_bottom_ =
          slop;
    }
  }

  if (!value.IsTable()) {
    return;
  }
  for (auto& it : *value.Table()) {
    if (it.first.str() == "left") {
      slop = ToVPFromUnitValue(it.second.StdString());
      hit_slop_left_ =
          base::FloatsNotEqual(slop, hit_slop_left_) ? slop : hit_slop_left_;
      continue;
    }
    if (it.first.str() == "right") {
      slop = ToVPFromUnitValue(it.second.StdString());
      hit_slop_right_ =
          base::FloatsNotEqual(slop, hit_slop_right_) ? slop : hit_slop_right_;
      continue;
    }
    if (it.first.str() == "top") {
      slop = ToVPFromUnitValue(it.second.StdString());
      hit_slop_top_ =
          base::FloatsNotEqual(slop, hit_slop_top_) ? slop : hit_slop_top_;
      continue;
    }
    if (it.first.str() == "bottom") {
      slop = ToVPFromUnitValue(it.second.StdString());
      hit_slop_bottom_ = base::FloatsNotEqual(slop, hit_slop_bottom_)
                             ? slop
                             : hit_slop_bottom_;
      continue;
    }
  }
}

void UIBase::SetIgnoreFocus(const lepus::Value& value) {
  if (value.IsBool()) {
    ignore_focus_ = value.Bool() ? LynxEventPropStatus::kEnable
                                 : LynxEventPropStatus::kDisable;
  }
}

void UIBase::SetEventThrough(const lepus::Value& value) {
  if (value.IsBool()) {
    event_through_ = value.Bool() ? LynxEventPropStatus::kEnable
                                  : LynxEventPropStatus::kDisable;
  }
}

void UIBase::SetBlockNativeEvent(const lepus::Value& value) {
  if (value.IsBool()) {
    block_native_event_ = value.Bool();
  }
}

void UIBase::SetConsumeSlideEvent(const lepus::Value& value) {
  if (value.IsNumber()) {
    consume_slide_event_ = static_cast<ConsumeSlideDirection>(value.Number());
    return;
  }
  if (!value.IsArrayOrJSArray()) {
    consume_slide_event_ = ConsumeSlideDirection::kNone;
    return;
  }

  bool direction_up_left = false;
  bool direction_up_right = false;
  bool direction_right_top = false;
  bool direction_right_bottom = false;
  bool direction_down_left = false;
  bool direction_down_right = false;
  bool direction_left_top = false;
  bool direction_left_bottom = false;
  tasm::ForEachLepusValue(
      value, [&direction_up_left, &direction_up_right, &direction_right_top,
              &direction_right_bottom, &direction_down_left,
              &direction_down_right, &direction_left_top,
              &direction_left_bottom](const auto& index, const auto& angles) {
        if (angles.IsArrayOrJSArray() && angles.GetLength() == 2) {
          const auto& begin = angles.GetProperty(0);
          const auto& end = angles.GetProperty(1);
          if (begin.IsNumber() && end.IsNumber()) {
            float angle_begin = (begin.Number() + 180) / 45,
                  angle_end = (end.Number() + 180) / 45;
            if (base::FloatsLargerOrEqual(angle_end, angle_begin)) {
              // judge down
              if (base::FloatsLargerOrEqual(angle_begin, 1.f) &&
                  base::FloatsLarger(2.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_down_left = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 1.f) &&
                  base::FloatsLargerOrEqual(2.f, angle_end)) {
                direction_down_left = true;
              }
              if (base::FloatsLargerOrEqual(1.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 2.f)) {
                direction_down_left = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 2.f) &&
                  base::FloatsLarger(3.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_down_right = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 2.f) &&
                  base::FloatsLargerOrEqual(3.f, angle_end)) {
                direction_down_right = true;
              }
              if (base::FloatsLargerOrEqual(2.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 3.f)) {
                direction_down_right = true;
              }
              // judge right
              if (base::FloatsLargerOrEqual(angle_begin, 4.f) &&
                  base::FloatsLarger(5.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_right_top = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 4.f) &&
                  base::FloatsLargerOrEqual(5.f, angle_end)) {
                direction_right_top = true;
              }
              if (base::FloatsLargerOrEqual(4.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 5.f)) {
                direction_right_top = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 3.f) &&
                  base::FloatsLarger(4.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_right_bottom = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 3.f) &&
                  base::FloatsLargerOrEqual(4.f, angle_end)) {
                direction_right_bottom = true;
              }
              if (base::FloatsLargerOrEqual(3.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 4.f)) {
                direction_right_bottom = true;
              }
              // judge up
              if (base::FloatsLargerOrEqual(angle_begin, 6.f) &&
                  base::FloatsLarger(7.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_up_left = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 6.f) &&
                  base::FloatsLargerOrEqual(7.f, angle_end)) {
                direction_up_left = true;
              }
              if (base::FloatsLargerOrEqual(6.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 7.f)) {
                direction_up_left = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 5.f) &&
                  base::FloatsLarger(6.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_up_right = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 5.f) &&
                  base::FloatsLargerOrEqual(6.f, angle_end)) {
                direction_up_right = true;
              }
              if (base::FloatsLargerOrEqual(5.f, angle_begin) &&
                  base::FloatsLargerOrEqual(angle_end, 6.f)) {
                direction_up_right = true;
              }
              // judge left
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(angle_end, 7.f) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_left_top = true;
              }
              if (base::FloatsLargerOrEqual(angle_begin, 0.f) &&
                  base::FloatsLarger(1.f, angle_begin) &&
                  base::FloatsLargerOrEqual(8.f, angle_end)) {
                direction_left_bottom = true;
              }
            }
          }
        }
      });

  bool direction_up = direction_up_left || direction_up_right;
  bool direction_right = direction_right_top || direction_right_bottom;
  bool direction_down = direction_down_left || direction_down_right;
  bool direction_left = direction_left_top || direction_left_bottom;
  if (direction_up && direction_right && direction_down && direction_left) {
    consume_slide_event_ = ConsumeSlideDirection::kAll;
    return;
  }
  if (direction_left && direction_right) {
    consume_slide_event_ = ConsumeSlideDirection::kHorizontal;
    return;
  }
  if (direction_up && direction_down) {
    consume_slide_event_ = ConsumeSlideDirection::kVertical;
    return;
  }
  if (direction_up) {
    consume_slide_event_ = ConsumeSlideDirection::kUp;
    return;
  }
  if (direction_right) {
    consume_slide_event_ = ConsumeSlideDirection::kRight;
    return;
  }
  if (direction_down) {
    consume_slide_event_ = ConsumeSlideDirection::kDown;
    return;
  }
  if (direction_left) {
    consume_slide_event_ = ConsumeSlideDirection::kLeft;
    return;
  }
}

void UIBase::SetEnableTouchPseudoPropagation(const lepus::Value& value) {
  if (value.IsBool()) {
    enable_touch_pseudo_propagation_ = value.Bool();
  }
}

void UIBase::SetExposureID(const lepus::Value& value) {
  if (value.IsString()) {
    exposure_id_ = value.StdString();
    new_exposure_props_["exposure-id"] = value;
  }
}

void UIBase::SetExposureScene(const lepus::Value& value) {
  if (value.IsString()) {
    exposure_scene_ = value.StdString();
    new_exposure_props_["exposure-scene"] = value;
  }
}

float UIBase::ToVPFromUnitValue(const std::string& value) {
  float screen_size[2] = {0.f};
  context_->ScreenSize(screen_size);
  return LynxUnitUtils::ToVPFromUnitValue(value, screen_size[0],
                                          context_->DevicePixelRatio());
}

void UIBase::SetExposureScreenMarginLeft(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_screen_margin_left_ = num;
  new_exposure_props_["exposure-screen-margin-left"] = value;
}

void UIBase::SetExposureScreenMarginRight(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_screen_margin_right_ = num;
  new_exposure_props_["exposure-screen-margin-right"] = value;
}

void UIBase::SetExposureScreenMarginTop(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_screen_margin_top_ = num;
  new_exposure_props_["exposure-screen-margin-top"] = value;
}

void UIBase::SetExposureScreenMarginBottom(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_screen_margin_bottom_ = num;
  new_exposure_props_["exposure-screen-margin-bottom"] = value;
}

void UIBase::SetExposureUIMarginLeft(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_ui_margin_left_ = num;
  new_exposure_props_["exposure-ui-margin-left"] = value;
}

void UIBase::SetExposureUIMarginRight(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_ui_margin_right_ = num;
  new_exposure_props_["exposure-ui-margin-right"] = value;
}

void UIBase::SetExposureUIMarginTop(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_ui_margin_top_ = num;
  new_exposure_props_["exposure-ui-margin-top"] = value;
}

void UIBase::SetExposureUIMarginBottom(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    num = ToVPFromUnitValue(value.StdString());
  }
  exposure_ui_margin_bottom_ = num;
  new_exposure_props_["exposure-ui-margin-bottom"] = value;
}

void UIBase::SetExposureArea(const lepus::Value& value) {
  float num = 0.f;
  if (value.IsNumber()) {
    num = value.Number();
  } else if (value.IsString()) {
    auto str = value.StdString();
    if (str.find("%") == str.size() - 1) {
      num = std::stof(str.substr(0, str.size() - 1)) / 100.f;
    }
  }
  exposure_area_ = num;
  new_exposure_props_["exposure-area"] = value;
}

void UIBase::SetEnableExposureUIClip(const lepus::Value& value) {
  if (value.IsBool()) {
    enable_exposure_ui_clip_ = value.Bool() ? LynxEventPropStatus::kEnable
                                            : LynxEventPropStatus::kDisable;
  }
  new_exposure_props_["enable-exposure-ui-clip"] = value;
}

void UIBase::SetDataset(const lepus::Value& value) {
  dataset_ = std::move(value);
}

void UIBase::SetImageRendering(const lepus::Value& value) {
  rendering_type_ = static_cast<starlight::ImageRenderingType>(value.Number());
}

void UIBase::SetEvents(const std::vector<lepus::Value>& events) {
  events_.clear();
  for (const auto& e : events) {
    if (!e.IsArray()) {
      continue;
    }
    const auto& name = e.Array()->get(0).StdString();
    events_.emplace_back(name);
    if (!has_appear_event_) {
      has_appear_event_ = name == "uiappear";
    }
    if (!has_disappear_event_) {
      has_disappear_event_ = name == "uidisappear";
    }
    if (has_appear_event_ || has_disappear_event_) {
      context_->AddUIToExposedMap(this, std::to_string(sign_), lepus::Value(),
                                  true);
    }
    if (!has_layout_change_event_ && name == "layoutchange") {
      has_layout_change_event_ = true;
    }
  }
}

UIBase* UIBase::FindViewById(const std::string& id, bool by_ref_id,
                             bool traverse_children) {
  if (id.empty()) {
    return nullptr;
  }
  const std::string& node_id = by_ref_id ? react_ref_id_ : id_selector_;
  if (node_id == id) {
    return this;
  }
  // Only top level component can traverse children
  if (!traverse_children && is_component_) {
    return nullptr;
  }
  for (const auto& child : children_) {
    auto ret = child->FindViewById(id, by_ref_id, false);
    if (ret != nullptr) {
      return ret;
    }
  }
  return nullptr;
}

void UIBase::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = ui_method_map_.find(method); it != ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    callback(LynxGetUIResult::METHOD_NOT_FOUND,
             lepus::Value("method not found: " + method));
  }
}

void UIBase::GetBoundingClientRect(float* res, bool to_screen) {
  float rect[4] = {0, 0, width_, height_};
  if (to_screen) {
    LynxUIHelper::ConvertRectFromUIToScreen(res, this, rect);
  } else {
    LynxUIHelper::ConvertRectFromUIToRootUI(res, this, rect);
  }
}

UIBase* UIBase::GetRelativeUI(const std::string& relativeId) {
  if (relativeId.empty()) {
    return nullptr;
  }
  UIBase* current = parent_;
  while (current != nullptr && current->Parent() != current) {
    if (relativeId == current->IdSelector()) {
      return current;
    }
    current = current->Parent();
  }
  return nullptr;
}

void UIBase::GetBoundingClientRect(const lepus::Value& args, float result[4]) {
  std::string relativeId;
  bool enable_android_transform = false;
  bool enable_harmony_transform = false;
  bool has_harmony = false;
  if (args.IsTable() || args.IsJSTable()) {
    tasm::ForEachLepusValue(args, [&relativeId, &enable_android_transform,
                                   &enable_harmony_transform, &has_harmony](
                                      const auto& key, const auto& val) {
      if (key.StdString() == "androidEnableTransformProps") {
        enable_android_transform = val.Bool();
      }
      if (key.StdString() == "harmonyEnableTransformProps") {
        has_harmony = true;
        enable_harmony_transform = val.Bool();
      }
      if (key.StdString() == "relativeTo") {
        relativeId = val.StdString();
      }
    });
  }

  bool enable_transform =
      has_harmony ? enable_harmony_transform : enable_android_transform;
  if (relativeId == "screen") {
    LynxUIHelper::ConvertRectFromUIToScreen(result, this, result,
                                            enable_transform);
  } else {
    UIBase* relativeUI = GetRelativeUI(relativeId);
    relativeUI = relativeUI == nullptr
                     ? context_->FindUIByIdSelector(relativeId)
                     : relativeUI;
    relativeUI = relativeUI == nullptr
                     ? reinterpret_cast<UIBase*>(context_->Root())
                     : relativeUI;
    LynxUIHelper::ConvertRectFromUIToAnotherUI(result, this, relativeUI, result,
                                               enable_transform);
  }
}

void UIBase::GetBoundingClientRect(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  float result[4] = {0, 0, width_, height_};
  GetBoundingClientRect(args, result);

  auto ret = lepus::Dictionary::Create();
  ret->SetValue("id", id_selector_);
  ret->SetValue("left", result[0]);
  ret->SetValue("top", result[1]);
  ret->SetValue("right", result[2]);
  ret->SetValue("bottom", result[3]);
  ret->SetValue("width", result[2] - result[0]);
  ret->SetValue("height", result[3] - result[1]);
  callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
}

void UIBase::ScrollIntoView(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!args.IsObject()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("missing the param of `scrollIntoViewOptions`"));
    return;
  }
  auto option = args.GetProperty("scrollIntoViewOptions");

  if (!option.IsTable()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("missing the param of `scrollIntoViewOptions`"));
    return;
  }

  auto behavior = "auto";
  auto block = "start";
  auto inline_value = "nearest";

  auto table = option.Table();
  for (const auto& [k, v] : *table) {
    if (v.IsString()) {
      const char* v_str = v.CString();
      if (k.IsEqual("behavior")) {
        behavior = v_str;
      } else if (k.IsEqual("block")) {
        block = v_str;
      } else if (k.IsEqual("inline")) {
        inline_value = v_str;
      }
    }
  }
  bool find_scroll_view = false;
  UIBase* base = parent_;
  while (base) {
    if (base->Tag() == "scroll-view") {
      find_scroll_view = true;
      base->ScrollIntoView(base::StringEqual(behavior, "smooth"), this, block,
                           inline_value);
      callback(LynxGetUIResult::SUCCESS, lepus_value(""));
      break;
    }
    base = base->parent_;
  }

  if (!find_scroll_view) {
    LOGE("UIBase scrollIntoView failed for nodeId:" << Sign());
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus_value(base::FormatString(
                 "UIBase scrollIntoView failed for nodeId: %d", sign_)));
  } else {
    callback(LynxGetUIResult::SUCCESS, lepus_value(""));
  }
}

bool UIBase::NeedDrawNode() {
  if (!background_drawable_) {
    return false;
  }

  if (node_type_ == ARKUI_NODE_CUSTOM) {
    if (background_drawable_->HasShadow()) {
      return true;
    }
    if (!need_clip_) {
      return false;
    }

    return background_drawable_->HasBorder() ||
           background_drawable_->HasImage();
  }
  return background_drawable_->HasBorder() ||
         background_drawable_->HasImage() || background_drawable_->HasShadow();
}

void UIBase::AttachToNodeContent(NativeNodeContent* content) {
  node_content_ = std::unique_ptr<NativeNodeContent>(content);
  // FIXME(chengjunnan): adpat to draw node.
  OH_ArkUI_NodeContent_AddNode(content->NodeContentHandle(), Node());
  node_content_->SetUI(this);
}

void UIBase::DetachFromNodeContent() {
  if (node_content_) {
    OH_ArkUI_NodeContent_RemoveNode(node_content_->NodeContentHandle(), Node());
    node_content_ = nullptr;
  }
}

void UIBase::InitDrawNode() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UIBASE_INIT_DRAW_NODE);
  if (draw_node_) {
    return;
  }
  auto parent = NodeManager::Instance().GetParent(Node());
  if (parent) {
    draw_node_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);
    // register draw event
    NodeManager::Instance().AddNodeCustomEventReceiver(
        draw_node_, UIBase::CustomEventReceiver);
    NodeManager::Instance().RegisterNodeCustomEvent(
        draw_node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW,
        ARKUI_NODE_CUSTOM_EVENT_ON_DRAW, this);
    // update view tree
    NodeManager::Instance().InsertNodeAfter(parent, draw_node_, Node());
    NodeManager::Instance().RemoveNode(parent, Node());
    NodeManager::Instance().InsertNode(draw_node_, Node(), 0);
    // update opacity
    float opacity{1.f};
    NodeManager::Instance().GetAttributeValues(Node(), NODE_OPACITY, &opacity);
    NodeManager::Instance().ResetAttribute(Node(), NODE_OPACITY);
    NodeManager::Instance().SetAttributeWithNumberValue(draw_node_,
                                                        NODE_OPACITY, opacity);
    // update visibility
    int32_t visibility{ARKUI_VISIBILITY_VISIBLE};
    NodeManager::Instance().GetAttributeValues(Node(), NODE_VISIBILITY,
                                               &visibility);
    NodeManager::Instance().ResetAttribute(Node(), NODE_VISIBILITY);
    NodeManager::Instance().SetAttributeWithNumberValue(
        draw_node_, NODE_VISIBILITY, visibility);

    NodeManager::Instance().ResetAttribute(Node(), NODE_Z_INDEX);
    NodeManager::Instance().SetAttributeWithNumberValue(
        draw_node_, NODE_Z_INDEX, translation_z_);
  }
  UpdateDrawNodeFrame();
}

void UIBase::DestroyDrawNode() {
  if (draw_node_) {
    NodeManager::Instance().UnregisterNodeCustomEvent(
        draw_node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW);
    NodeManager::Instance().RemoveNodeCustomEventReceiver(
        draw_node_, UIBase::CustomEventReceiver);
    NodeManager::Instance().DisposeNode(draw_node_);
    draw_node_ = nullptr;
  }
}

void UIBase::UpdateDrawNodeFrame() {
  if (!draw_node_) {
    return;
  }
  auto position = NodeManager::Instance().GetAttribute(Node(), NODE_POSITION);
  float x = position->value[0].f32;
  float y = position->value[1].f32;
  auto size = NodeManager::Instance().GetAttribute(Node(), NODE_SIZE);
  float width = size->value[0].f32;
  float height = size->value[1].f32;
  NodeManager::Instance().SetAttributeWithNumberValue(Node(), NODE_POSITION, 0,
                                                      0);
  NodeManager::Instance().SetAttributeWithNumberValue(draw_node_, NODE_SIZE,
                                                      width, height);
  NodeManager::Instance().SetAttributeWithNumberValue(draw_node_, NODE_POSITION,
                                                      x, y);
}

bool UIBase::NeedDraw(ArkUI_NodeHandle node) { return node_ == node; }

float UIBase::ViewLeft() const {
  return NodeManager::Instance().GetAttribute<float>(DrawNode(), NODE_POSITION,
                                                     0);
}

float UIBase::ViewTop() const {
  return NodeManager::Instance().GetAttribute<float>(DrawNode(), NODE_POSITION,
                                                     1);
}

// TODO(hexionghui): Remove this later, and replace it with GetPointInTarget.
void UIBase::GetTargetPoint(float target_point[2], float point[2],
                            float scroll[2], float target_origin_rect[4],
                            Transform* target_transform) {
  target_point[0] = point[0] - target_origin_rect[0] + scroll[0];
  target_point[1] = point[1] - target_origin_rect[1] + scroll[1];
  if (target_transform) {
    transforms::Matrix44 invert_matrix;
    if (target_transform
            ->GetTransformMatrix(target_origin_rect[2], target_origin_rect[3],
                                 1.f, true)
            .invert(&invert_matrix)) {
      invert_matrix.mapPoint(target_point, target_point);
    }
  }
}

// TODO(hexionghui): Remove this later, and replace it with GetPointInTarget.
void UIBase::GetOriginRect(float origin_rect[4]) {
  origin_rect[0] = ViewLeft();
  origin_rect[1] = ViewTop();
  origin_rect[2] = width_;
  origin_rect[3] = height_;
}

bool UIBase::ContainsPoint(float point[2]) {
  float x = point[0], y = point[1];
  bool contain = base::FloatsLargerOrEqual(x, hit_slop_left_) &&
                 base::FloatsLargerOrEqual(width_ + hit_slop_right_, x) &&
                 base::FloatsLargerOrEqual(y, hit_slop_top_) &&
                 base::FloatsLargerOrEqual(height_ + hit_slop_bottom_, y);
  if (!contain) {
    if (overflow_.overflow_x && overflow_.overflow_y) {
      // TODO(hexionghui): need to optimize to avoid redundant hit-test when
      // HitTest(point) != this
      return HitTest(point) != this;
    }
    if (overflow_.overflow_x &&
        (base::FloatsLarger(hit_slop_top_, y) ||
         base::FloatsLarger(y, height_ + hit_slop_bottom_))) {
      return contain;
    }
    if (overflow_.overflow_y &&
        (base::FloatsLarger(hit_slop_left_, x) ||
         base::FloatsLarger(x, width_ + hit_slop_right_))) {
      return contain;
    }
  }
  return contain;
}

bool UIBase::ShouldHitTest() {
  if (!node_ || NodeManager::Instance().GetParent(DrawNode()) == nullptr) {
    return false;
  }
  if (NodeManager::Instance().GetAttribute<int>(
          DrawNode(), NODE_VISIBILITY, 0) != ARKUI_VISIBILITY_VISIBLE) {
    return false;
  }
  return user_interaction_enabled_;
}

bool UIBase::IsOnResponseChain() { return false; }

EventTarget* UIBase::HitTest(float point[2]) {
  EventTarget* target = nullptr;
  float target_point[] = {point[0], point[1]};
  float child_point[2] = {0};
  for (int i = children_.size() - 1; i >= 0; --i) {
    UIBase* ui = children_[i];
    if (!ui->ShouldHitTest()) {
      continue;
    }

    ui->GetPointInTarget(child_point, this, point);
    if (ui->ContainsPoint(child_point)) {
      if (ui->IsOnResponseChain()) {
        target = ui;
        target_point[0] = child_point[0];
        target_point[1] = child_point[1];
        break;
      }
      if (target == nullptr ||
          base::FloatsLarger(ui->TranslateZ(),
                             reinterpret_cast<UIBase*>(target)->TranslateZ())) {
        target = ui;
        target_point[0] = child_point[0];
        target_point[1] = child_point[1];
      }
    }
  }

  if (target == nullptr) {
    return this;
  }
  return target->HitTest(target_point);
}

bool UIBase::IsVisible() {
  if (!node_ || NodeManager::Instance().GetParent(DrawNode()) == nullptr) {
    return false;
  }

  if (base::FloatsEqual(NodeManager::Instance().GetAttribute<float>(
                            DrawNode(), NODE_OPACITY, 0),
                        0.0f)) {
    return false;
  }

  if (NodeManager::Instance().GetAttribute<int>(
          DrawNode(), NODE_VISIBILITY, 0) != ARKUI_VISIBILITY_VISIBLE) {
    return false;
  }

  if ((base::FloatsEqual(width_, 0.0f) && !overflow_.overflow_x) ||
      (base::FloatsEqual(height_, 0.0f) && !overflow_.overflow_y)) {
    return false;
  }

  return true;
}

bool UIBase::IsScrollable() { return false; }

void UIBase::GestureRecognized() {
  // When there is a UI sliding on the touch response chain, the corresponding
  // UI needs to call this method to prevent the tap event from being triggered.
  context_->OnGestureRecognized(this);
}

std::string UIBase::ExposureUIKey(const std::string& unique_id,
                                  bool is_add) const {
  const auto& exposure_props =
      is_add ? new_exposure_props_ : old_exposure_props_;
  std::string exposure_scene, exposure_id;
  if (auto it = exposure_props.find("exposure-scene");
      it != exposure_props.end()) {
    exposure_scene = it->second.StdString();
  }
  if (auto it = exposure_props.find("exposure-id");
      it != exposure_props.end()) {
    exposure_id = it->second.StdString();
  }
  if (!unique_id.empty()) {
    return unique_id + "_" + exposure_scene + "_" + exposure_id;
  } else {
    return exposure_scene + "_" + exposure_id + "_" + std::to_string(sign_);
  }
}

void UIBase::GetExposureUIRect(float rect[4]) {
  rect[0] += exposure_ui_margin_left_;
  rect[1] += exposure_ui_margin_top_;
  rect[2] += exposure_ui_margin_right_;
  rect[3] += exposure_ui_margin_bottom_;
}

void UIBase::GetExposureWindowRect(float rect[4]) {
  rect[0] += exposure_screen_margin_left_;
  rect[1] += exposure_screen_margin_top_;
  rect[2] += exposure_screen_margin_right_;
  rect[3] += exposure_screen_margin_bottom_;
}

bool UIBase::IsVisibleForExposure(
    std::unordered_map<int, UIExposure::CommonAncestorUIRect>&
        common_ancestor_ui_rect_map,
    float offset_screen[2]) {
  float parent_rect[4] = {0};
  std::vector<UIBase*> parent_array;
  UIBase* current = parent_;
  while (current != nullptr && current->Parent() != current) {
    if (!current->IsVisible()) {
      return false;
    }
    if (current->IsOverlay()) {
      break;
    }
    if (current->EnableExposureUIClip() == LynxEventPropStatus::kEnable ||
        (current->EnableExposureUIClip() == LynxEventPropStatus::kUndefined &&
         current->IsScrollable()) ||
        current->IsRoot()) {
      parent_array.push_back(current);
      int sign = current->Sign();
      parent_rect[0] = 0;
      parent_rect[1] = 0;
      parent_rect[2] = current->width_;
      parent_rect[3] = current->height_;
      auto it = common_ancestor_ui_rect_map.find(sign);
      if (it != common_ancestor_ui_rect_map.end()) {
        if (it->second.ui_rect_updated) {
          auto& rect = it->second.ui_rect;
          parent_rect[0] = rect[0];
          parent_rect[1] = rect[1];
          parent_rect[2] = rect[2];
          parent_rect[3] = rect[3];
        } else {
          current->GetBoundingClientRect(parent_rect);
          LynxUIHelper::OffsetRect(parent_rect, offset_screen);
          auto& rect = it->second.ui_rect;
          rect[0] = parent_rect[0];
          rect[1] = parent_rect[1];
          rect[2] = parent_rect[2];
          rect[3] = parent_rect[3];
          it->second.ui_rect_updated = true;
        }
      } else {
        current->GetBoundingClientRect(parent_rect);
        LynxUIHelper::OffsetRect(parent_rect, offset_screen);
        UIExposure::CommonAncestorUIRect rect = {.ui_count = 1,
                                                 .ui_rect_updated = true,
                                                 .ui_rect[0] = parent_rect[0],
                                                 .ui_rect[1] = parent_rect[1],
                                                 .ui_rect[2] = parent_rect[2],
                                                 .ui_rect[3] = parent_rect[3]};
        common_ancestor_ui_rect_map.emplace(sign, std::move(rect));
      }
    }
    current = current->Parent();
  }

  float ui_rect[4] = {0, 0, width_, height_};
  bool ui_rect_calculated = false;
  for (auto parent : parent_array) {
    int sign = parent->Sign();
    auto it = common_ancestor_ui_rect_map.find(sign);
    if (it != common_ancestor_ui_rect_map.end()) {
      auto& rect = it->second.ui_rect;
      parent_rect[0] = rect[0];
      parent_rect[1] = rect[1];
      parent_rect[2] = rect[2];
      parent_rect[3] = rect[3];
    } else {
      parent->GetBoundingClientRect(parent_rect);
      LynxUIHelper::OffsetRect(parent_rect, offset_screen);
      UIExposure::CommonAncestorUIRect rect = {.ui_count = 1,
                                               .ui_rect_updated = true,
                                               .ui_rect[0] = parent_rect[0],
                                               .ui_rect[1] = parent_rect[1],
                                               .ui_rect[2] = parent_rect[2],
                                               .ui_rect[3] = parent_rect[3]};
      common_ancestor_ui_rect_map.emplace(sign, std::move(rect));
    }
    if (!ui_rect_calculated) {
      LynxUIHelper::ConvertRectFromDescendantToAncestor(ui_rect, this, parent,
                                                        ui_rect);
      LynxUIHelper::OffsetRect(ui_rect, parent_rect);
      GetExposureUIRect(ui_rect);
      ui_rect_calculated = true;
    }
    if (!LynxUIHelper::CheckViewportIntersectWithRatio(ui_rect, parent_rect,
                                                       exposure_area_)) {
      return false;
    }
  }

  float window_rect[4] = {0};
  auto it = common_ancestor_ui_rect_map.find(-10);
  if (it != common_ancestor_ui_rect_map.end()) {
    if (it->second.ui_rect_updated) {
      auto& rect = it->second.ui_rect;
      window_rect[0] = rect[0];
      window_rect[1] = rect[1];
      window_rect[2] = rect[2];
      window_rect[3] = rect[3];
    } else {
      float size[2] = {0};
      context_->ScreenSize(size);
      window_rect[2] = size[0];
      window_rect[3] = size[1];
      GetExposureWindowRect(window_rect);
      auto& rect = it->second.ui_rect;
      rect[0] = window_rect[0];
      rect[1] = window_rect[1];
      rect[2] = window_rect[2];
      rect[3] = window_rect[3];
      it->second.ui_rect_updated = true;
    }
  } else {
    float size[2] = {0};
    context_->ScreenSize(size);
    window_rect[2] = size[0];
    window_rect[3] = size[1];
    GetExposureWindowRect(window_rect);
    UIExposure::CommonAncestorUIRect rect = {.ui_count = 1,
                                             .ui_rect_updated = true,
                                             .ui_rect[0] = window_rect[0],
                                             .ui_rect[1] = window_rect[1],
                                             .ui_rect[2] = window_rect[2],
                                             .ui_rect[3] = window_rect[3]};
    common_ancestor_ui_rect_map.emplace(-10, std::move(rect));
  }

  return LynxUIHelper::CheckViewportIntersectWithRatio(ui_rect, window_rect,
                                                       exposure_area_);
}

void UIBase::WillRemoveFromUIParent() {
  if (draw_node_) {
    NodeManager::Instance().RemoveNode(parent_->Node(), draw_node_);
  }
}

void UIBase::GetTransformValue(float left, float right, float top, float bottom,
                               std::vector<float>& point) {
  std::pair<float, float> left_top{left, top};
  GetLocationOnScreen(left_top);
  point[0] = left_top.first;
  point[1] = left_top.second;
  std::pair<float, float> right_top{width_ + right, top};
  GetLocationOnScreen(right_top);
  point[2] = right_top.first;
  point[3] = right_top.second;
  std::pair<float, float> right_bottom{width_ + right, height_ + bottom};
  GetLocationOnScreen(right_bottom);
  point[4] = right_bottom.first;
  point[5] = right_bottom.second;
  std::pair<float, float> left_bottom{left, height_ + bottom};
  GetLocationOnScreen(left_bottom);
  point[6] = left_bottom.first;
  point[7] = left_bottom.second;
}

void UIBase::MapPointWithTransform(std::pair<float, float>& point) {
  if (GetTransform()) {
    float dst_point[2] = {0.f, 0.f};
    float src_point[2] = {point.first, point.second};
    GetTransform()
        ->GetTransformMatrix(width_, height_, 1.f, true)
        .mapPoint(dst_point, src_point);
    point.first = dst_point[0];
    point.second = dst_point[1];
  }
}

void UIBase::TransformFromViewToRootView(UIBase* ui,
                                         std::pair<float, float>& point) {
  auto scale = context_->ScaledDensity();
  point.first *= scale;
  point.second *= scale;
  ui->MapPointWithTransform(point);
  auto current_ui = ui;
  while (!current_ui->IsRoot()) {
    auto parent_ui = current_ui->Parent();
    if (!parent_ui) {
      break;
    }
    point.first += current_ui->left_ * scale;
    point.second += current_ui->top_ * scale;
    point.first -= parent_ui->ScrollX() * scale;
    point.second -= parent_ui->ScrollY() * scale;
    parent_ui->MapPointWithTransform(point);
    current_ui = parent_ui;
  }
}

void UIBase::GetLocationOnScreen(std::pair<float, float>& point) {
  auto base_point = std::pair<float, float>();
  // TODO(chengjunnan)  use windowLocation here later.
  base_point.first = 0;
  base_point.second = 0;
  TransformFromViewToRootView(this, point);
  point.first += base_point.first;
  point.second += base_point.second;
}

void UIBase::SetVisibility(bool visible) {
  ArkUI_Visibility visibility =
      visible ? ARKUI_VISIBILITY_VISIBLE : ARKUI_VISIBILITY_HIDDEN;
  NodeManager::Instance().SetAttributeWithNumberValue(
      DrawNode(), NODE_VISIBILITY, static_cast<int32_t>(visibility));
}

EventTarget* UIBase::ParentTarget() {
  return reinterpret_cast<EventTarget*>(parent_);
}

void UIBase::GetPointInTarget(float res[2], EventTarget* parent_target,
                              float point[2]) {
  return LynxUIHelper::ConvertPointFromAncestorToDescendant(
      res, static_cast<UIBase*>(parent_target), this, point);
}

bool UIBase::BlockNativeEvent() { return block_native_event_; }

bool UIBase::EventThrough() {
  if (event_through_ == LynxEventPropStatus::kEnable) {
    return true;
  } else if (event_through_ == LynxEventPropStatus::kDisable) {
    return false;
  }

  EventTarget* parent = this->ParentTarget();
  while (parent) {
    if (!parent->ParentTarget()) {
      return false;
    }
    return parent->EventThrough();
  }
  return false;
}

bool UIBase::IgnoreFocus() {
  if (ignore_focus_ == LynxEventPropStatus::kEnable) {
    return true;
  } else if (ignore_focus_ == LynxEventPropStatus::kDisable) {
    return false;
  }

  EventTarget* parent = this->ParentTarget();
  while (parent) {
    if (!parent->ParentTarget()) {
      return false;
    }
    return parent->IgnoreFocus();
  }
  return false;
}

ConsumeSlideDirection UIBase::ConsumeSlideEvent() {
  return consume_slide_event_;
}

bool UIBase::IsInterceptGesture() {
  return gesture_status_ ==
         LynxInterceptGestureStatus::LynxInterceptGestureStateTrue;
}

bool UIBase::TouchPseudoPropagation() {
  return enable_touch_pseudo_propagation_;
}

void UIBase::OnPseudoStatusChanged(PseudoStatus pre_status,
                                   PseudoStatus current_status) {
  pseudo_status_ = current_status;
}

PseudoStatus UIBase::GetPseudoStatus() { return pseudo_status_; }

std::vector<float> UIBase::ScrollBy(float delta_x, float delta_y) {
  return std::vector<float>{0, 0, delta_x, delta_y};
}

starlight::ImageRenderingType UIBase::RenderingType() {
  return rendering_type_;
}

void UIBase::SetClipPath(const lepus::Value& value) {
  basic_shape_ = std::make_unique<BasicShape>(value, context_->ScaledDensity());
  dirty_flags_ |= kFlagClipPathChanged;
}

void UIBase::SetPerspective(const lepus::Value& value) {
  perspective_ = std::move(value);
  dirty_flags_ |= kFlagTransformChanged;
}

float UIBase::GetPerspectiveValue() {
  float result = 0;
  if (perspective_.IsArray()) {
    const auto& perspective_data = perspective_.Array();
    if (perspective_data->size() > 1) {
      float perspective = perspective_data->get(0).Number();
      auto unit = static_cast<starlight::PerspectiveLengthUnit>(
          perspective_data->get(1).Number());
      if (unit == starlight::PerspectiveLengthUnit::VW) {
        result = perspective / 100.0f * context_->Root()->width_;
      } else if (unit == starlight::PerspectiveLengthUnit::VH) {
        result = perspective / 100.0f * context_->Root()->height_;
      } else {
        result = perspective;
      }
    }
  }
  return result != 0 ? 1.0 / (result * kCameraDistanceNormalizationMultiplier)
                     : 0;
}

void UIBase::SetAccessibilityElement(const lepus::Value& value) {
  if (value.IsBool()) {
    accessibility_mode_ = value.Bool() ? LynxAccessibilityMode::kEnable
                                       : LynxAccessibilityMode::kDisable;
    dirty_flags_ |= kFlagAccessibilityMode;
  }
}

void UIBase::SetAccessibilityElementsHidden(const lepus::Value& value) {
  if (value.IsBool()) {
    accessibility_mode_ = value.Bool()
                              ? LynxAccessibilityMode::kDisableForDescendants
                              : LynxAccessibilityMode::kEnable;
    dirty_flags_ |= kFlagAccessibilityMode;
  }
}

void UIBase::SetAccessibilityLabel(const lepus::Value& value) {
  if (value.IsString()) {
    accessibility_label_ = value.StdString();
    dirty_flags_ |= kFlagAccessibilityLabel;
  }
}

void UIBase::SetAccessibilityId(const lepus::Value& value) {
  if (value.IsString()) {
    accessibility_id_ = value.StdString();
  }
}

void UIBase::SetCrossLanguageOption(const lepus::Value& value) {
  if (value.IsBool()) {
    base::harmony::HarmonyCompatFunctionsHandler* handle =
        base::harmony::GetHarmonyCompatFunctionsHandler();
    if (handle == nullptr) {
      LOGE("GetHarmonyCompatFunctionsHandler failed");
      return;
    }

    ArkUI_CrossLanguageOption* option =
        handle->oh_create_cross_language_option_func();
    if (option == nullptr) {
      LOGE("oh_create_cross_language_option_func failed");
      return;
    }
    handle->oh_set_cross_language_option_set_attribute_setting_status_func(
        option, value.Bool());
    handle->oh_set_node_utils_set_cross_language_option_func(DrawNode(),
                                                             option);
    handle->oh_destroy_cross_language_option_func(option);
  }
}

ArkUI_NodeType UIBase::GetAccessibilityType(const std::string& traits) {
  if (traits == "text") {
    return ARKUI_NODE_TEXT;
  }
  if (traits == "image") {
    return ARKUI_NODE_IMAGE;
  }
  if (traits == "button") {
    return ARKUI_NODE_BUTTON;
  }
  return ARKUI_NODE_CUSTOM;
}

void UIBase::SetAccessibilityTraits(const lepus::Value& value) {
  if (value.IsString()) {
    accessibility_traits_ = value.StdString();
    dirty_flags_ |= kFlagAccessibilityTraits;
  }
}

void UIBase::SetAccessibilityExclusiveFocus(const lepus::Value& value) {
  if (value.IsBool()) {
    context_->GetUIOwner()->AddOrRemoveUIFromExclusiveSet(Sign(), value.Bool());
    dirty_flags_ |= kFlagAccessibilityExclusive;
  }
}

bool UIBase::IsImportantForAccessibility() {
  if (accessibility_mode_ != LynxAccessibilityMode::kEnable) {
    return false;
  }
  if (context_->GetUIOwner()->HasAccessibilityExclusiveUI()) {
    UIBase* parent = parent_;
    while (parent) {
      if (context_->GetUIOwner()->ContainAccessibilityExclusiveUI(
              parent->Sign())) {
        return true;
      }
      parent = parent->parent_;
    }
    return false;
  }
  return true;
}

void UIBase::ResetAccessibilityAttrs() {
  if (IsImportantForAccessibility()) {
    if (accessibility_status_ == LynxAccessibilityStatus::kFocus) {
      return;
    }
    accessibility_status_ = LynxAccessibilityStatus::kFocus;
    NodeManager::Instance().SetAttributeWithNumberValue(
        DrawNode(), NODE_ACCESSIBILITY_MODE,
        static_cast<int>(accessibility_mode_));
    std::string label = GetAccessibilityLabel();
    if (!label.empty()) {
      ArkUI_AttributeItem item = {.string = label.c_str()};
      NodeManager::Instance().SetAttribute(DrawNode(), NODE_ACCESSIBILITY_TEXT,
                                           &item);
    }
    ArkUI_NodeType type = GetAccessibilityType(accessibility_traits_);
    if (type != ARKUI_NODE_CUSTOM) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          DrawNode(), NODE_ACCESSIBILITY_ROLE, static_cast<int>(type));
    }
  } else {
    if (accessibility_status_ == LynxAccessibilityStatus::kDisable) {
      return;
    }
    accessibility_status_ = LynxAccessibilityStatus::kDisable;
    NodeManager::Instance().SetAttributeWithNumberValue(
        DrawNode(), NODE_ACCESSIBILITY_MODE,
        static_cast<int>(LynxAccessibilityMode::kDisable));
    NodeManager::Instance().ResetAttribute(DrawNode(), NODE_ACCESSIBILITY_TEXT);
  }
  dirty_flags_ &= ~kFlagAccessibilityMode;
  dirty_flags_ &= ~kFlagAccessibilityLabel;
  dirty_flags_ &= ~kFlagAccessibilityTraits;
}

void UIBase::OnNodeReadyForAccessibility() {
  if ((dirty_flags_ & kFlagAccessibilityExclusive) != 0) {
    dirty_flags_ &= ~kFlagAccessibilityExclusive;
    context_->GetUIOwner()->ResetAccessibilityAttrs();
    return;
  }

  if ((dirty_flags_ & kFlagAccessibilityMode) != 0) {
    if (accessibility_mode_ != LynxAccessibilityMode::kEnable) {
      accessibility_status_ = LynxAccessibilityStatus::kDisable;
      NodeManager::Instance().SetAttributeWithNumberValue(
          DrawNode(), NODE_ACCESSIBILITY_MODE,
          static_cast<int>(accessibility_mode_));
      return;
    }
    if (!IsImportantForAccessibility()) {
      return;
    }
    accessibility_status_ = LynxAccessibilityStatus::kFocus;
    NodeManager::Instance().SetAttributeWithNumberValue(
        DrawNode(), NODE_ACCESSIBILITY_MODE,
        static_cast<int>(accessibility_mode_));
  }
  // For exclusive UI, do nothing
  if (accessibility_status_ != LynxAccessibilityStatus::kFocus) {
    return;
  }
  if ((accessibility_mode_ == LynxAccessibilityMode::kEnable) &&
      (dirty_flags_ & kFlagAccessibilityLabel) != 0) {
    std::string label = GetAccessibilityLabel();
    if (!label.empty()) {
      ArkUI_AttributeItem item = {.string = label.c_str()};
      NodeManager::Instance().SetAttribute(DrawNode(), NODE_ACCESSIBILITY_TEXT,
                                           &item);
    }
  }
  if ((accessibility_mode_ == LynxAccessibilityMode::kEnable) &&
      (dirty_flags_ & kFlagAccessibilityTraits) != 0) {
    ArkUI_NodeType type = GetAccessibilityType(accessibility_traits_);
    if (type != ARKUI_NODE_CUSTOM) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          DrawNode(), NODE_ACCESSIBILITY_ROLE, static_cast<int>(type));
    }
  }
}

void UIBase::SetAccessibilityLabelDirtyFlag() {
  if (accessibility_label_.empty()) {
    dirty_flags_ |= kFlagAccessibilityLabel;
  }
}

void UIBase::InitAccessibilityAttrs(LynxAccessibilityMode mode,
                                    const std::string& traits) {
  accessibility_mode_ = mode;
  accessibility_traits_ = traits;
  dirty_flags_ |= kFlagAccessibilityMode;
  dirty_flags_ |= kFlagAccessibilityTraits;
}

void UIBase::Base64EncodeTask(
    OH_PixelmapNative* pixel_map, const std::string& format,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  napi_env env = context_->GetNapiEnv();
  auto async_context =
      new EncodeAsyncContexts(pixel_map, format, std::move(callback));
  if (async_context == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("Create Base64 encode task failed"));
    return;
  }
  napi_value work_name;
  napi_create_string_utf8(env, "TakeScreenshotBase64EncodeTask",
                          NAPI_AUTO_LENGTH, &work_name);
  napi_create_async_work(
      env, nullptr, work_name,
      [](napi_env env, void* data) {
        EncodeAsyncContexts* encode_context =
            reinterpret_cast<EncodeAsyncContexts*>(data);
        if (encode_context == nullptr) {
          LOGE("Failed to get encode task context");
          return;
        }
        OH_Pixelmap_ImageInfo* image_info = nullptr;
        OH_PixelmapImageInfo_Create(&image_info);
        OH_PixelmapNative_GetImageInfo(encode_context->pixel_map, image_info);
        OH_PixelmapImageInfo_GetWidth(image_info, &encode_context->width);
        OH_PixelmapImageInfo_GetHeight(image_info, &encode_context->height);
        OH_PixelmapImageInfo_Release(image_info);

        OH_ImagePackerNative* image_packer = nullptr;
        OH_PackingOptions* pack_option = nullptr;
        OH_ImagePackerNative_Create(&image_packer);
        OH_PackingOptions_Create(&pack_option);
        std::string image_format = "image/" + encode_context->format;
        Image_MimeType mime_type = {image_format.data(), image_format.length()};
        OH_PackingOptions_SetMimeType(pack_option, &mime_type);
        OH_PackingOptions_SetQuality(pack_option, 100);

        size_t buffer_size = encode_context->width * encode_context->height;
        std::unique_ptr<uint8_t[]> buffer =
            std::make_unique<uint8_t[]>(buffer_size);
        int32_t res = OH_ImagePackerNative_PackToDataFromPixelmap(
            image_packer, pack_option, encode_context->pixel_map, buffer.get(),
            &buffer_size);
        OH_PixelmapNative_Release(encode_context->pixel_map);
        OH_PackingOptions_Release(pack_option);
        OH_ImagePackerNative_Release(image_packer);
        if (res != 0) {
          encode_context->data = "pack image failed";
          encode_context->res = false;
          return;
        }
        std::unique_ptr<char[]> base64_buf =
            std::make_unique<char[]>(modp_b64_encode_len(buffer_size));
        size_t base64_len = modp_b64_encode(
            base64_buf.get(), reinterpret_cast<const char*>(buffer.get()),
            buffer_size);
        if (base64_len == static_cast<size_t>(-1)) {
          encode_context->data = "image base64 encode failed";
          encode_context->res = false;
          return;
        }
        std::string base64_str(reinterpret_cast<const char*>(base64_buf.get()),
                               base64_len);
        encode_context->data = base64_str;
        encode_context->res = true;
      },
      [](napi_env env, napi_status status, void* data) {
        EncodeAsyncContexts* encode_context =
            reinterpret_cast<EncodeAsyncContexts*>(data);
        if (encode_context == nullptr) {
          LOGE("Failed to get encode task context");
          return;
        }
        if (encode_context->res) {
          auto ret = lepus::Dictionary::Create();
          ret->SetValue("width", encode_context->width);
          ret->SetValue("height", encode_context->height);
          std::string header = "data:image/jpeg;base64,";
          if (encode_context->format == "png") {
            header = "data:image/png;base64,";
          }
          ret->SetValue("data", header + encode_context->data);
          encode_context->callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
        } else {
          encode_context->callback(LynxGetUIResult::OPERATION_ERROR,
                                   lepus::Value(encode_context->data));
        }
        napi_delete_async_work(env, encode_context->async_work);
        delete encode_context;
      },
      reinterpret_cast<void*>(async_context), &async_context->async_work);
  napi_queue_async_work(env, async_context->async_work);
}

void UIBase::TakeScreenshot(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  base::harmony::HarmonyCompatFunctionsHandler* handle =
      base::harmony::GetHarmonyCompatFunctionsHandler();
  if (handle == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("load symbol failed"));
    return;
  }
  std::string format = "jpeg";
  float scale = 1.0f;
  if (args.IsTable() || args.IsJSTable()) {
    tasm::ForEachLepusValue(
        args, [&format, &scale](const auto& key, const auto& val) {
          if (key.StdString() == "format") {
            format = val.StdString();
          }
          if (key.StdString() == "scale") {
            scale = val.Number();
          }
        });
  }
  scale = std::clamp(scale, 0.0f, 1.0f);
  ArkUI_SnapshotOptions* snapshot_option =
      handle->oh_create_snapshot_option_func();
  if (snapshot_option == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("The current version does not support screenshot"));
    return;
  }
  int32_t res =
      handle->oh_snapshot_option_set_scale_func(snapshot_option, scale);
  if (res != 0) {
    handle->oh_destroy_snapshot_option_func(snapshot_option);
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("screenshot args init failed"));
    return;
  }
  OH_PixelmapNative* pixel_map = nullptr;
  handle->oh_get_node_snapshot_func(DrawNode(), snapshot_option, &pixel_map);
  handle->oh_destroy_snapshot_option_func(snapshot_option);
  if (pixel_map == nullptr) {
    callback(LynxGetUIResult::OPERATION_ERROR,
             lepus::Value("task snapshot failed"));
    return;
  }
  Base64EncodeTask(pixel_map, format, std::move(callback));
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
