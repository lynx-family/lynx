// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/lynx_event_dispatcher.h"

#include <memory>
#include <utility>

#include "clay/lynx_adaptor/base_def.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/public/value.h"

#if OS_WIN
#include <Windows.h>
#elif OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

namespace clay {

namespace {
std::string AnimationPropertyTypeToString(ClayAnimationPropertyType type) {
  std::string ret;
  switch (type) {
    default:
    case ClayAnimationPropertyType::kNone:
      ret = "none";
      break;
    case ClayAnimationPropertyType::kOpacity:
      ret = "opacity";
      break;
    case ClayAnimationPropertyType::kScaleX:
      ret = "scalex";
      break;
    case ClayAnimationPropertyType::kScaleY:
      ret = "scaley";
      break;
    case ClayAnimationPropertyType::kScaleXY:
      ret = "scalexy";
      break;
    case ClayAnimationPropertyType::kWidth:
      ret = "width";
      break;
    case ClayAnimationPropertyType::kHeight:
      ret = "height";
      break;
    case ClayAnimationPropertyType::kBackgroundColor:
      ret = "background-color";
      break;
    case ClayAnimationPropertyType::kColor:
      ret = "color";
      break;
    case ClayAnimationPropertyType::kVisibility:
      ret = "visibility";
      break;
    case ClayAnimationPropertyType::kLeft:
      ret = "left";
      break;
    case ClayAnimationPropertyType::kTop:
      ret = "top";
      break;
    case ClayAnimationPropertyType::kRight:
      ret = "right";
      break;
    case ClayAnimationPropertyType::kBottom:
      ret = "bottom";
      break;
    case ClayAnimationPropertyType::kTransform:
      ret = "transform";
      break;
    case ClayAnimationPropertyType::kAll:
      ret = "all";
      break;
  }
  ret = "transition-" + ret;
  return ret;
}
void AddKeyModifierProperties(clay::Value::Map& dict) {
  bool altKey = false;
  bool ctrlKey = false;
  bool shiftKey = false;
  bool metaKey = false;
#if OS_WIN
  if ((GetKeyState(VK_LMENU) | GetKeyState(VK_RMENU)) & 0x8000) {
    altKey = true;
  }
  if ((GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & 0x8000) {
    ctrlKey = true;
  }
  if ((GetKeyState(VK_LSHIFT) | GetKeyState(VK_RSHIFT)) & 0x8000) {
    shiftKey = true;
  }
  if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
    metaKey = true;
  }
#elif OS_MAC
  if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Option) ||
      CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                            kVK_RightOption)) {
    altKey = true;
  }
  if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Control) ||
      CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                            kVK_RightControl)) {
    ctrlKey = true;
  }
  if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Shift) ||
      CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                            kVK_RightShift)) {
    shiftKey = true;
  }
  if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, kVK_Command) ||
      CGEventSourceKeyState(kCGEventSourceStateHIDSystemState,
                            kVK_RightCommand)) {
    metaKey = true;
  }
#endif
  dict["altKey"] = clay::Value(altKey);
  dict["shiftKey"] = clay::Value(shiftKey);
  dict["ctrlKey"] = clay::Value(ctrlKey);
  dict["metaKey"] = clay::Value(metaKey);
}
}  // namespace

// clay::EventDelegate override
void LynxEventDispatcher::OnTouchEvent(const std::string& event_name, int tag,
                                       float x, float y, float page_x,
                                       float page_y) {
  if (engine_proxy_) {
    engine_proxy_->SendTouchEvent(event_name, tag, x, y, x, y, page_x, page_y);
  }
}

void LynxEventDispatcher::OnMouseEvent(const std::string& event_name,
                                       int view_id, int button, int buttons,
                                       float scale, float x, float y,
                                       float page_x, float page_y) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Map rk_dict;

  rk_dict["type"] = clay::Value(event_name.c_str());
  rk_dict["button"] = clay::Value(button);
  rk_dict["buttons"] = clay::Value(buttons);
  rk_dict["scale"] = clay::Value(scale);

  float density = engine_proxy_->GetDensity();
  rk_dict["x"] = clay::Value(x / density);
  rk_dict["y"] = clay::Value(y / density);
  rk_dict["pageX"] = clay::Value(page_x / density);
  rk_dict["pageY"] = clay::Value(page_y / density);
  rk_dict["clientX"] = clay::Value(page_x / density);
  rk_dict["clientY"] = clay::Value(page_y / density);

  rk_dict["identifier"] = clay::Value(reinterpret_cast<int64_t>(&rk_dict));

  AddKeyModifierProperties(rk_dict);

  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendBubbleEvent(event_name, view_id, params);
}

void LynxEventDispatcher::OnWheelEvent(const std::string& event_name,
                                       int view_id, float x, float y,
                                       float page_x, float page_y,
                                       float delta_x, float delta_y) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Map rk_dict;

  // apply the attributes of mouse event
  float density = engine_proxy_->GetDensity();
  rk_dict["x"] = clay::Value(x / density);
  rk_dict["y"] = clay::Value(y / density);
  rk_dict["pageX"] = clay::Value(page_x / density);
  rk_dict["pageY"] = clay::Value(page_y / density);
  rk_dict["clientX"] = clay::Value(page_x / density);
  rk_dict["clientY"] = clay::Value(page_y / density);

  // apply the attributes of wheel event
  rk_dict["deltaX"] = clay::Value(delta_x);
  rk_dict["deltaY"] = clay::Value(delta_y);

  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendBubbleEvent(event_name, view_id, params);
}

void LynxEventDispatcher::OnKeyEvent(const std::string& event_name, int view_id,
                                     const char* key, bool repeat) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Map rk_dict;
  rk_dict["type"] = clay::Value(event_name);
  rk_dict["key"] = clay::Value(key);
  rk_dict["repeat"] = clay::Value(repeat);

  AddKeyModifierProperties(rk_dict);

  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendBubbleEvent(event_name, view_id, params);
}

void LynxEventDispatcher::OnAnimationEvent(const std::string& event_name,
                                           const char* animation_name,
                                           int view_id) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Map rk_dict;
  rk_dict["animation_type"] = clay::Value("keyframe-animation");
  rk_dict["animation_name"] = clay::Value(animation_name);

  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendCustomEvent(event_name, view_id, params, "params");
}

void LynxEventDispatcher::OnTransitionEvent(const std::string& event_name,
                                            const char* animation_name,
                                            int view_id,
                                            ClayAnimationPropertyType type) {
  if (!engine_proxy_) {
    return;
  }
  auto apt = AnimationPropertyTypeToString(type);
  clay::Value::Map rk_dict;
  rk_dict["animation_type"] = clay::Value(apt.c_str());
  rk_dict["animation_name"] = clay::Value(animation_name);

  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendCustomEvent(event_name, view_id, params, "params");
}

void LynxEventDispatcher::OnFocusChanged(int view_id, bool focus) {
  if (!engine_proxy_) {
    return;
  }
  auto prev_state = !focus ? lynx::kPseudoStateFocus : lynx::kPseudoStateNone;
  auto cur_state = focus ? lynx::kPseudoStateFocus : lynx::kPseudoStateNone;
  engine_proxy_->OnPseudoStatusChanged(view_id, prev_state, cur_state);
}

void LynxEventDispatcher::OnHoverChanged(int view_id, bool hover) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Map rk_dict;
  rk_dict["hover"] = clay::Value(hover);
  auto params = lynx::ClayValue(clay::Value(std::move(rk_dict)));
  engine_proxy_->SendCustomEvent("hoverchange", view_id, params, "detail");

  auto prev_state = !hover ? lynx::kPseudoStateHover : lynx::kPseudoStateNone;
  auto cur_state = hover ? lynx::kPseudoStateHover : lynx::kPseudoStateNone;
  engine_proxy_->OnPseudoStatusChanged(view_id, prev_state, cur_state);
}

void LynxEventDispatcher::OnDragDropEvent(const std::string& event_name,
                                          int view_id, clay::Value::Map map) {
  if (!engine_proxy_) {
    return;
  }
  map.emplace("type", clay::Value(event_name.c_str()));
  auto params = lynx::ClayValue(clay::Value{std::move(map)});
  engine_proxy_->SendBubbleEvent(event_name, view_id, params);
}

void LynxEventDispatcher::OnSendCustomEvent(int view_id,
                                            const std::string& event_name,
                                            clay::Value::Map args) {
  if (!engine_proxy_) {
    return;
  }
  auto params = lynx::ClayValue(clay::Value{std::move(args)});
  engine_proxy_->SendCustomEvent(event_name, view_id, params, "detail");
}

void LynxEventDispatcher::OnSendGlobalEvent(const std::string& event_name,
                                            clay::Value args) {
  if (!runtime_proxy_) {
    return;
  }
  clay::Value::Array array_wrapper(2);
  array_wrapper[0] = clay::Value(event_name);
  clay::Value::Array array_args(1);
  array_args[0] = std::move(args);
  array_wrapper[1] = clay::Value(std::move(array_args));

  auto params = clay::Value(std::move(array_wrapper));
  runtime_proxy_->CallJSFunction(
      "GlobalEventEmitter", "emit",
      std::make_unique<lynx::ClayValue>(std::move(params)));
}

void LynxEventDispatcher::OnDrawEndEvent() {
  if (!perf_controller_) {
    return;
  }
  perf_controller_->OnPaintEnd();
}

void LynxEventDispatcher::OnFirstMeaningfulPaint() {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->OnFirstMeaningfulPaint();
}

void LynxEventDispatcher::OnOverlayEvent(int view_id, const char* overlay_id,
                                         int overlay_count,
                                         const char** overlay_ids,
                                         const char* event_name) {
  if (!engine_proxy_) {
    return;
  }
  clay::Value::Array rk_param(1);
  {
    clay::Value::Map rk_dict;
    clay::Value::Array overlays(overlay_count);
    for (int i = 0; i < overlay_count; ++i) {
      overlays[i] = clay::Value(overlay_ids[i]);
    }
    rk_dict["currentOverlayId"] = clay::Value(overlay_id);

    auto rk_overlays = clay::Value(std::move(overlays));
    rk_dict["overlays"] = std::move(rk_overlays);

    rk_param[0] = clay::Value(std::move(rk_dict));
  }
  auto params = lynx::ClayValue(clay::Value(std::move(rk_param)));
  engine_proxy_->SendCustomEvent(event_name, view_id, params, "detail");
}

void LynxEventDispatcher::OnLayoutChanged(int view_id, clay::Value::Map map) {
  if (!engine_proxy_) {
    return;
  }
  auto params = lynx::ClayValue(clay::Value(std::move(map)));
  engine_proxy_->SendCustomEvent("layoutchange", view_id, params, "detail");
}

void LynxEventDispatcher::OnIntersectionEvent(int view_id,
                                              clay::Value::Map map) {
  if (!engine_proxy_) {
    return;
  }
  auto params = lynx::ClayValue(clay::Value(std::move(map)));
  engine_proxy_->SendCustomEvent("intersection", view_id, params, "detail");
}

void LynxEventDispatcher::OnCallJSApiCallback(int callback_id,
                                              clay::Value value) {
  if (!runtime_proxy_) {
    return;
  }
  runtime_proxy_->CallJSApiCallbackWithValue(
      callback_id, std::make_unique<lynx::ClayValue>(std::move(value)));
}

void LynxEventDispatcher::CallJSIntersectionObserver(int observer_id,
                                                     int callback_id,
                                                     clay::Value params) {
  if (!runtime_proxy_) {
    return;
  }
  runtime_proxy_->CallJSIntersectionObserver(
      observer_id, callback_id,
      std::make_unique<lynx::ClayValue>(std::move(params)));
}

}  // namespace clay
