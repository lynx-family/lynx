// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_dispatcher.h"

#include <arkui/native_key_event.h>
#include <deviceinfo.h>
#include <dlfcn.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/fml/task_runner.h"
#include "core/base/harmony/harmony_function_loader.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/lynx_console_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/bubble_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_emitter.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

constexpr const char* kHitTargetStyle =
    "background-color:#9CC4E6;border-width:2px;border-color:red;";
constexpr int64_t kCurrentLynxPageOnlyEventID =
    std::numeric_limits<int64_t>::min();
constexpr int kKeyEventSupportVersion = 14;
constexpr int kAxisActionSupportVersion = 15;
constexpr int kModifierKeySupportVersion = 17;

int64_t CurrentTimestampMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

template <typename Function>
Function LoadArkUIInputFunction(const char* name, int support_version) {
  if (OH_GetSdkApiVersion() < support_version) {
    return nullptr;
  }
  void* handle =
      base::harmony::GetSharedObjectHandler(base::harmony::kAceNdkSoName);
  if (!handle) {
    return nullptr;
  }
  return reinterpret_cast<Function>(dlsym(handle, name));
}

using GetKeyEventType = ArkUI_KeyEventType (*)(const ArkUI_UIInputEvent*);
using GetKeyCode = int32_t (*)(const ArkUI_UIInputEvent*);
using GetKeyText = const char* (*)(const ArkUI_UIInputEvent*);
using GetAxisAction = int32_t (*)(const ArkUI_UIInputEvent*);
using GetModifierKeyStates = int32_t (*)(const ArkUI_UIInputEvent*, uint64_t*);

GetKeyEventType KeyEventTypeFunction() {
  static auto function = LoadArkUIInputFunction<GetKeyEventType>(
      "OH_ArkUI_KeyEvent_GetType", kKeyEventSupportVersion);
  return function;
}

GetKeyCode KeyCodeFunction() {
  static auto function = LoadArkUIInputFunction<GetKeyCode>(
      "OH_ArkUI_KeyEvent_GetKeyCode", kKeyEventSupportVersion);
  return function;
}

GetKeyText KeyTextFunction() {
  static auto function = LoadArkUIInputFunction<GetKeyText>(
      "OH_ArkUI_KeyEvent_GetKeyText", kKeyEventSupportVersion);
  return function;
}

GetAxisAction AxisActionFunction() {
  static auto function = LoadArkUIInputFunction<GetAxisAction>(
      "OH_ArkUI_AxisEvent_GetAxisAction", kAxisActionSupportVersion);
  return function;
}

GetModifierKeyStates ModifierKeyStatesFunction() {
  static auto function = LoadArkUIInputFunction<GetModifierKeyStates>(
      "OH_ArkUI_UIInputEvent_GetModifierKeyStates", kModifierKeySupportVersion);
  return function;
}

void AddModifierProperties(lepus::Dictionary* params,
                           const ArkUI_UIInputEvent* event) {
  uint64_t modifiers = 0;
  auto get_modifier_key_states = ModifierKeyStatesFunction();
  if (get_modifier_key_states) {
    get_modifier_key_states(event, &modifiers);
  }
  params->SetValue("altKey", (modifiers & ARKUI_MODIFIER_KEY_ALT) != 0);
  params->SetValue("ctrlKey", (modifiers & ARKUI_MODIFIER_KEY_CTRL) != 0);
  params->SetValue("shiftKey", (modifiers & ARKUI_MODIFIER_KEY_SHIFT) != 0);
  params->SetValue("metaKey", false);
}

int ToDOMMouseButton(int button) {
  switch (button) {
    case UI_MOUSE_EVENT_BUTTON_LEFT:
      return 0;
    case UI_MOUSE_EVENT_BUTTON_MIDDLE:
      return 1;
    case UI_MOUSE_EVENT_BUTTON_RIGHT:
      return 2;
    case UI_MOUSE_EVENT_BUTTON_BACK:
      return 3;
    case UI_MOUSE_EVENT_BUTTON_FORWARD:
      return 4;
    default:
      return -1;
  }
}

int ToDOMMouseButtonMask(int button) {
  switch (button) {
    case UI_MOUSE_EVENT_BUTTON_LEFT:
      return 1;
    case UI_MOUSE_EVENT_BUTTON_RIGHT:
      return 2;
    case UI_MOUSE_EVENT_BUTTON_MIDDLE:
      return 4;
    case UI_MOUSE_EVENT_BUTTON_BACK:
      return 8;
    case UI_MOUSE_EVENT_BUTTON_FORWARD:
      return 16;
    default:
      return 0;
  }
}

std::string PointerTypeForTool(int tool_type) {
  switch (tool_type) {
    case UI_INPUT_EVENT_TOOL_TYPE_PEN:
      return "pen";
    case UI_INPUT_EVENT_TOOL_TYPE_MOUSE:
    case UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD:
      return "mouse";
    case UI_INPUT_EVENT_TOOL_TYPE_FINGER:
    default:
      return "touch";
  }
}

std::string KeyTextForCode(int32_t key_code, const char* key_text) {
  if (key_code == ARKUI_KEYCODE_ENTER) {
    return "Enter";
  }
  if (key_code == ARKUI_KEYCODE_SPACE) {
    return " ";
  }
  if (key_text && key_text[0] != '\0') {
    return key_text;
  }
  switch (key_code) {
    case ARKUI_KEYCODE_TAB:
      return "Tab";
    case ARKUI_KEYCODE_ESCAPE:
      return "Escape";
    case ARKUI_KEYCODE_DEL:
      return "Backspace";
    case ARKUI_KEYCODE_FORWARD_DEL:
      return "Delete";
    case ARKUI_KEYCODE_DPAD_LEFT:
      return "ArrowLeft";
    case ARKUI_KEYCODE_DPAD_RIGHT:
      return "ArrowRight";
    case ARKUI_KEYCODE_DPAD_UP:
      return "ArrowUp";
    case ARKUI_KEYCODE_DPAD_DOWN:
      return "ArrowDown";
    case ARKUI_KEYCODE_MOVE_HOME:
      return "Home";
    case ARKUI_KEYCODE_MOVE_END:
      return "End";
    case ARKUI_KEYCODE_PAGE_UP:
      return "PageUp";
    case ARKUI_KEYCODE_PAGE_DOWN:
      return "PageDown";
    default:
      return "Unidentified";
  }
}

uint64_t NextRequestId(std::atomic<uint64_t>& request_id) {
  return request_id.fetch_add(1, std::memory_order_relaxed) + 1;
}

std::string AppendHitTargetStyle(std::string css_text) {
  if (!css_text.empty() && css_text.back() != ';') {
    css_text.push_back(';');
  }
  css_text += kHitTargetStyle;
  return css_text;
}

std::string BuildSetAttributesAsTextMessage(int node_id,
                                            const std::string& css_text,
                                            uint64_t message_id) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("id");
  writer.Uint64(message_id);
  writer.Key("method");
  writer.String("DOM.setAttributesAsText");
  writer.Key("params");
  writer.StartObject();
  writer.Key("nodeId");
  writer.Int(node_id);
  writer.Key("text");
  std::string style_text = "style=\"" + css_text + "\"";
  writer.String(style_text.c_str(),
                static_cast<rapidjson::SizeType>(style_text.size()));
  writer.Key("name");
  writer.String("style");
  writer.EndObject();
  writer.EndObject();
  return buffer.GetString();
}

std::string BuildGetInlineStylesForNodeMessage(int node_id,
                                               uint64_t message_id) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("id");
  writer.Uint64(message_id);
  writer.Key("method");
  writer.String("CSS.getInlineStylesForNode");
  writer.Key("params");
  writer.StartObject();
  writer.Key("nodeId");
  writer.Int(node_id);
  writer.EndObject();
  writer.EndObject();
  return buffer.GetString();
}

bool FindCssText(const rapidjson::Value& value, std::string& css_text) {
  if (value.IsObject()) {
    auto iter = value.FindMember("cssText");
    if (iter != value.MemberEnd() && iter->value.IsString()) {
      css_text.assign(iter->value.GetString(), iter->value.GetStringLength());
      return true;
    }
    for (auto member = value.MemberBegin(); member != value.MemberEnd();
         ++member) {
      if (FindCssText(member->value, css_text)) {
        return true;
      }
    }
  } else if (value.IsArray()) {
    for (auto iter = value.Begin(); iter != value.End(); ++iter) {
      if (FindCssText(*iter, css_text)) {
        return true;
      }
    }
  }
  return false;
}

std::optional<std::string> ExtractInlineCSSText(const std::string& response) {
  rapidjson::Document document;
  document.Parse(response.c_str());
  if (document.HasParseError()) {
    LOGE("ExtractInlineCSSText parse error: "
         << rapidjson::GetParseError_En(document.GetParseError())
         << ", offset: " << document.GetErrorOffset()
         << ", response: " << response.substr(0, 512));
    return std::nullopt;
  }
  std::string css_text;
  if (!FindCssText(document, css_text)) {
    LOGW("ExtractInlineCSSText cssText not found, response: "
         << response.substr(0, 512));
    return std::nullopt;
  }
  return css_text;
}

}  // namespace

struct EventDispatcher::WeakFlag {
  explicit WeakFlag(EventDispatcher* dispatcher) : dispatcher(dispatcher) {}
  std::atomic<EventDispatcher*> dispatcher;
};

static void* GetGestureInterrupterGetUserDataFunc() {
  if (OH_GetSdkApiVersion() < kGestureInterrupterUserDataSupportVersion) {
    return nullptr;
  }
  void* handle =
      base::harmony::GetSharedObjectHandler(base::harmony::kAceNdkSoName);
  if (handle == nullptr) {
    return nullptr;
  }
  return dlsym(handle, "OH_ArkUI_GestureInterrupter_GetUserData");
}

static void* GestureInterrupterGetUserDataFuncHandle() {
  static void* handle = GetGestureInterrupterGetUserDataFunc();
  return handle;
}

GestureReceiver EventDispatcher::long_press_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnLongPressEvent(
          OH_ArkUI_GestureEvent_GetRawInputEvent(event));
    };

GestureReceiver EventDispatcher::tap_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnTapEvent(
          OH_ArkUI_GestureEvent_GetRawInputEvent(event));
    };
// for gesture handler, record the current touch speed.
GestureReceiver EventDispatcher::velocity_tracker_pan_receiver_callback_ =
    [](ArkUI_GestureEvent* event, void* user_data) {
      if (!user_data) {
        return;
      }
      auto event_dispatcher = reinterpret_cast<EventDispatcher*>(user_data);
      event_dispatcher->OnGetVelocity(event);
    };

GestureInterrupter EventDispatcher::event_gesture_interrupter_callback_ =
    [](ArkUI_GestureInterruptInfo* info) -> ArkUI_GestureInterruptResult {
  EventDispatcher* event_dispatcher =
      NodeManager::Instance().GetEventDispatcher();
  if (LynxEnv::GetInstance().EnableHarmonyGestureInterrupterUserData() &&
      OH_GetSdkApiVersion() >= kGestureInterrupterUserDataSupportVersion) {
    void* func = GestureInterrupterGetUserDataFuncHandle();
    if (func != nullptr) {
      using OhGetUserData = void* (*)(ArkUI_GestureInterruptInfo*);
      auto* event_dispatcher_from_user_data = static_cast<EventDispatcher*>(
          reinterpret_cast<OhGetUserData>(func)(info));
      if (event_dispatcher_from_user_data != nullptr) {
        event_dispatcher = event_dispatcher_from_user_data;
      }
    }
  }
  if (!event_dispatcher) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  if (event_dispatcher->EventThrough()) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }

  auto gesture = OH_ArkUI_GestureInterruptInfo_GetRecognizer(info);
  if (gesture == event_dispatcher->long_press_gesture_ ||
      gesture == event_dispatcher->tap_gesture_) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->block_outer_pan_gesture_ &&
      event_dispatcher->ShouldBlockNativeEvent()) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->velocity_tracker_pan_gesture_ &&
      event_dispatcher->ui_owner_->GetGestureArenaManager() != nullptr &&
      event_dispatcher->ContainGestureNode()) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }

  if (gesture == event_dispatcher->native_gesture_pan_gesture_) {
    if (event_dispatcher->ShouldInterceptGesture()) {
      return GESTURE_INTERRUPT_RESULT_CONTINUE;
    } else {
      return GESTURE_INTERRUPT_RESULT_REJECT;
    }
  }

  switch (event_dispatcher->ShouldConsumeSlideEvent()) {
    case ConsumeSlideDirection::kHorizontal:
      return gesture == event_dispatcher->consume_horizontal_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kVertical:
      return gesture == event_dispatcher->consume_vertical_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kUp:
      return gesture == event_dispatcher->consume_up_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kRight:
      return gesture == event_dispatcher->consume_right_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kDown:
      return gesture == event_dispatcher->consume_down_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kLeft:
      return gesture == event_dispatcher->consume_left_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    case ConsumeSlideDirection::kAll:
      return gesture == event_dispatcher->consume_all_pan_gesture_
                 ? GESTURE_INTERRUPT_RESULT_CONTINUE
                 : GESTURE_INTERRUPT_RESULT_REJECT;
    default:
      return GESTURE_INTERRUPT_RESULT_REJECT;
  }
};

EventDispatcher::EventTargetDetail::EventTargetDetail(
    std::weak_ptr<EventTarget> active_target, float down_point[2]) {
  active_target_ = std::move(active_target);
  down_point_[0] = down_point[0];
  down_point_[1] = down_point[1];
  pre_point_[0] = down_point[0];
  pre_point_[1] = down_point[1];
}

void EventDispatcher::EventTargetDetail::GetDownPoint(float down_point[2]) {
  down_point[0] = down_point_[0];
  down_point[1] = down_point_[1];
}

void EventDispatcher::EventTargetDetail::GetPrePoint(float pre_point[2]) {
  pre_point[0] = pre_point_[0];
  pre_point[1] = pre_point_[1];
}

void EventDispatcher::EventTargetDetail::SetPrePoint(float pre_point[2]) {
  pre_point_[0] = pre_point[0];
  pre_point_[1] = pre_point[1];
}

EventDispatcher::EventDispatcher(UIOwner* ui_owner)
    : ui_owner_(ui_owner), weak_flag_(std::make_shared<WeakFlag>(this)) {
  NodeManager::Instance().SetEventDispatcher(this);
  velocity_tracker_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 0);
  NodeManager::Instance().SetGestureEventTarget(
      velocity_tracker_pan_gesture_, GESTURE_EVENT_ACTION_UPDATE, this,
      EventDispatcher::velocity_tracker_pan_receiver_callback_);
  block_outer_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
  consume_horizontal_pan_gesture_ = NodeManager::Instance().CreatePanGesture(
      1, GESTURE_DIRECTION_HORIZONTAL, 5);
  native_gesture_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
  consume_vertical_pan_gesture_ = NodeManager::Instance().CreatePanGesture(
      1, GESTURE_DIRECTION_VERTICAL, 5);
  consume_up_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_UP, 5);
  consume_right_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_RIGHT, 5);
  consume_down_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_DOWN, 5);
  consume_left_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_LEFT, 5);
  consume_all_pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
}

EventDispatcher::~EventDispatcher() {
  if (weak_flag_) {
    weak_flag_->dispatcher.store(nullptr, std::memory_order_release);
  }
  NodeManager::Instance().SetEventDispatcher(nullptr);
  if (long_press_gesture_) {
    NodeManager::Instance().DisposeGesture(long_press_gesture_);
  }
  if (tap_gesture_) {
    NodeManager::Instance().DisposeGesture(tap_gesture_);
  }
  NodeManager::Instance().DisposeGesture(block_outer_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_horizontal_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_vertical_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_up_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_right_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_down_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_left_pan_gesture_);
  NodeManager::Instance().DisposeGesture(consume_all_pan_gesture_);

  NodeManager::Instance().DisposeGesture(native_gesture_pan_gesture_);
  NodeManager::Instance().DisposeGesture(velocity_tracker_pan_gesture_);
}

void EventDispatcher::AttachGesturesToRoot(UIBase* root) {
  if (ui_owner_->Destroyed() || !root->RootNode()) {
    return;
  }
  root_target_ = root->weak_from_this();
  if (root->IsRoot()) {
    fallback_hit_test_root_ = root->weak_from_this();
    if (active_overlay_hit_test_roots_.empty()) {
      hit_test_root_ = root->weak_from_this();
    }
  }
  if (long_press_gesture_) {
    NodeManager::Instance().AddGestureToNode(
        root->RootNode(), long_press_gesture_, PARALLEL, NORMAL_GESTURE_MASK);
  }
  if (tap_gesture_) {
    NodeManager::Instance().AddGestureToNode(root->RootNode(), tap_gesture_,
                                             PARALLEL, NORMAL_GESTURE_MASK);
  }
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           block_outer_pan_gesture_, PARALLEL,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_horizontal_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_vertical_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(
      root->RootNode(), consume_up_pan_gesture_, PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_right_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_down_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_left_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           consume_all_pan_gesture_, PRIORITY,
                                           NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           native_gesture_pan_gesture_,
                                           PRIORITY, NORMAL_GESTURE_MASK);
  NodeManager::Instance().AddGestureToNode(root->RootNode(),
                                           velocity_tracker_pan_gesture_,
                                           PARALLEL, NORMAL_GESTURE_MASK);

  NodeManager::Instance().SetGestureInterrupterToNode(
      root->RootNode(), EventDispatcher::event_gesture_interrupter_callback_,
      this);
}

void EventDispatcher::AttachGesturesToOverlayRoot(UIBase* root, int32_t level) {
  if (ui_owner_->Destroyed() || !root->RootNode()) {
    return;
  }
  AttachGesturesToRoot(root);
  ActivateOverlayHitTestRoot(root, level);
}

void EventDispatcher::DetachGesturesFromRoot(UIBase* root) {
  if (!root || !root->RootNode()) {
    return;
  }
  auto* root_node = root->RootNode();
  NodeManager::Instance().SetGestureInterrupterToNode(root_node, nullptr);
  if (long_press_gesture_) {
    NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                  long_press_gesture_);
  }
  if (tap_gesture_) {
    NodeManager::Instance().RemoveGestureFromNode(root_node, tap_gesture_);
  }
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                block_outer_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(
      root_node, consume_horizontal_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_vertical_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_up_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_right_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_down_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_left_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                consume_all_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                native_gesture_pan_gesture_);
  NodeManager::Instance().RemoveGestureFromNode(root_node,
                                                velocity_tracker_pan_gesture_);
  if (root_target_.lock().get() == root) {
    root_target_.reset();
  }
  if (root->IsOverlayContent()) {
    if (fallback_hit_test_root_.lock().get() == root) {
      auto* page_root = ui_owner_->Root();
      fallback_hit_test_root_ =
          page_root ? page_root->weak_from_this() : std::weak_ptr<UIBase>();
    }
    DeactivateOverlayHitTestRoot(root);
  }
}

void EventDispatcher::InitTouchEnv(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    float page_point[2] = {0.f};
    GetEventPagePoint(page_point, event, i);
    EventTarget* best_hittest_target = FindTarget(page_point);
    if (best_hittest_target == nullptr) {
      continue;
    }
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    if (!primary_pointer_id_.has_value()) {
      primary_pointer_id_ = pointer_id;
    }
    int tool_type = OH_ArkUI_UIInputEvent_GetToolType(event);
    pointer_tool_types_.insert_or_assign(pointer_id, tool_type);
    primary_pointer_ids_by_tool_.try_emplace(tool_type, pointer_id);
    LOGI("EventDispatcher InitTouchEnv hit target: "
         << best_hittest_target->Sign())
    ShowMessageOnConsole("EventDispatcher: hit the target with sign = " +
                             std::to_string(best_hittest_target->Sign()),
                         runtime::CONSOLE_LOG_INFO);
    RetainTextEventTargetRoot(best_hittest_target);
    if (IsPrimaryInput(event, pointer_id)) {
      InspectHitTarget(best_hittest_target);
      first_finger_down_point_[0] = 0;
      first_finger_down_point_[1] = 0;
      first_active_target_ = best_hittest_target->WeakTarget();
      UIBase* root = root_target_.expired()
                         ? nullptr
                         : static_cast<UIBase*>(root_target_.lock().get());
      UIBase* target_ui =
          best_hittest_target->HasUI()
              ? static_cast<UIBase*>(best_hittest_target)
              : static_cast<UIBase*>(best_hittest_target->FirstUITarget());
      LynxUIHelper::ConvertPointFromAncestorToDescendant(
          first_finger_down_point_, root, target_ui, page_point);
    }
    active_target_finger_map_.insert_or_assign(
        pointer_id,
        EventTargetDetail(best_hittest_target->WeakTarget(), page_point));
  }
}

void EventDispatcher::RetainTextEventTargetRoot(EventTarget* target) {
  std::shared_ptr<EventTarget> retained_root;
  while (target != nullptr && !target->HasUI()) {
    auto retained_target = target->WeakTarget().lock();
    if (!retained_target) {
      return;
    }
    auto* parent = target->ParentTarget();
    retained_root = std::move(retained_target);
    if (parent == target) {
      break;
    }
    target = parent;
  }
  if (retained_root) {
    retained_text_event_targets_.insert_or_assign(retained_root.get(),
                                                  std::move(retained_root));
  }
}

void EventDispatcher::ResetTouchEnv(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    active_target_finger_map_.erase(pointer_id);
    auto tool_it = pointer_tool_types_.find(pointer_id);
    if (tool_it != pointer_tool_types_.end()) {
      int tool_type = tool_it->second;
      pointer_tool_types_.erase(tool_it);
      bool has_active_pointer_of_type = std::any_of(
          pointer_tool_types_.begin(), pointer_tool_types_.end(),
          [tool_type](const auto& entry) { return entry.second == tool_type; });
      if (!has_active_pointer_of_type) {
        primary_pointer_ids_by_tool_.erase(tool_type);
      }
    }
  }
  has_touch_moved_ = false;
  if (active_target_finger_map_.empty()) {
    primary_pointer_id_.reset();
  }
}

EventDispatcher::EmulatedTouchPoint EventDispatcher::CreateEmulatedTouchPoint(
    int x, int y) {
  EmulatedTouchPoint point;
  float scaled_density = ui_owner_->Context()->ScaledDensity();
  if (base::FloatsLargerOrEqual(0.f, scaled_density)) {
    scaled_density = 1.f;
  }
  point.page_point[0] = static_cast<float>(x) / scaled_density;
  point.page_point[1] = static_cast<float>(y) / scaled_density;
  point.client_point[0] = point.page_point[0];
  point.client_point[1] = point.page_point[1];
  return point;
}

void EventDispatcher::InitTouchEnv(const EmulatedTouchPoint& point) {
  if (root_target_.expired()) {
    if (auto* root = ui_owner_->Root()) {
      root_target_ = root->weak_from_this();
    }
  }
  float page_point[2] = {point.page_point[0], point.page_point[1]};
  EventTarget* best_hittest_target = FindTarget(page_point);
  if (best_hittest_target == nullptr) {
    return;
  }
  LOGI("EventDispatcher InitTouchEnv synthetic hit target: "
       << best_hittest_target->Sign())
  RetainTextEventTargetRoot(best_hittest_target);
  first_finger_down_point_[0] = 0;
  first_finger_down_point_[1] = 0;
  first_active_target_ = best_hittest_target->WeakTarget();
  UIBase* root = root_target_.expired()
                     ? nullptr
                     : static_cast<UIBase*>(root_target_.lock().get());
  UIBase* target_ui =
      best_hittest_target->HasUI()
          ? static_cast<UIBase*>(best_hittest_target)
          : static_cast<UIBase*>(best_hittest_target->FirstUITarget());
  LynxUIHelper::ConvertPointFromAncestorToDescendant(
      first_finger_down_point_, root, target_ui, page_point);
  active_target_finger_map_.insert_or_assign(
      point.pointer_id,
      EventTargetDetail(best_hittest_target->WeakTarget(), page_point));
}

void EventDispatcher::ResetTouchEnv(const EmulatedTouchPoint& point) {
  active_target_finger_map_.erase(point.pointer_id);
  has_touch_moved_ = false;
  if (active_target_finger_map_.empty()) {
    primary_pointer_id_.reset();
  }
}

void EventDispatcher::InitClickEnv() {
  click_target_chain_.clear();
  if (first_active_target_.expired()) {
    return;
  }
  auto active_target = first_active_target_.lock().get();
  while (active_target != nullptr &&
         active_target->ParentTarget() != active_target) {
    click_target_chain_.push_back(active_target->WeakTarget());
    active_target = active_target->ParentTarget();
  }

  while (!click_target_chain_.empty()) {
    auto& last_target = click_target_chain_.front();
    if (last_target.expired()) {
      click_target_chain_.pop_front();
      continue;
    }
    bool has_click_event = false;
    for (const auto& event : last_target.lock()->EventSet()) {
      if (event == "click") {
        has_click_event = true;
        break;
      }
    }
    if (has_click_event) {
      break;
    } else {
      click_target_chain_.pop_front();
    }
  }

  for (const auto& target : click_target_chain_) {
    if (target.expired()) {
      continue;
    }
    target.lock()->OnResponseChain();
  }
}

void EventDispatcher::ResetClickEnv() {
  for (const auto& target : click_target_chain_) {
    if (target.expired()) {
      continue;
    }
    target.lock()->OffResponseChain();
  }
}

void EventDispatcher::OnTouchDown(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (IsPrimaryInput(event, OH_ArkUI_PointerEvent_GetPointerId(event, i))) {
      first_touch_moved_ = false;
      first_touch_outside_ = false;
      gesture_recognized_target_set_.clear();
      event_target_chain_.clear();
      InitClickEnv();
      if (!enable_multi_touch_) {
        DispatchSingleTouchEvent(TouchEvent::START, event);
      }
      ActivePseudoStatus();
      break;
    }
  }
}

void EventDispatcher::OnTouchMove(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  bool first_touch_changed = false;
  float pre_page_point[2] = {0.f};
  for (size_t i = 0; i < num; ++i) {
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    float page_point[2] = {0.f};
    GetEventPagePoint(page_point, event, i);
    if (auto touch_target = active_target_finger_map_.find(pointer_id);
        touch_target != active_target_finger_map_.end()) {
      touch_target->second.GetPrePoint(pre_page_point);
      if (base::FloatsNotEqual(page_point[0], pre_page_point[0]) ||
          base::FloatsNotEqual(page_point[1], pre_page_point[1])) {
        has_touch_moved_ = true;
        touch_target->second.SetPrePoint(page_point);
        if (!first_touch_moved_ && IsPrimaryInput(event, pointer_id)) {
          first_touch_changed = true;
          float down_page_point[2] = {0.f};
          touch_target->second.GetDownPoint(down_page_point);
          if (base::FloatsLarger(
                  pow(abs(page_point[0] - down_page_point[0]), 2) +
                      pow(abs(page_point[1] - down_page_point[1]), 2),
                  pow(tap_slop_, 2))) {
            first_touch_moved_ = true;
          }
        }
      }
    }
  }

  if (first_touch_changed) {
    if (auto first_touch_target = active_target_finger_map_.find(
            primary_pointer_id_.value_or(std::numeric_limits<int>::min()));
        first_touch_target != active_target_finger_map_.end()) {
      first_touch_target->second.GetPrePoint(pre_page_point);
      if (!click_target_chain_.empty()) {
        auto active_target = FindTarget(pre_page_point);
        auto click_target = click_target_chain_.front();
        first_touch_outside_ =
            first_touch_outside_ || IsTouchMoveOutside(active_target) ||
            !CanRespondTap(!click_target.expired() ? click_target.lock().get()
                                                   : nullptr);
      }
      if (first_touch_moved_ ||
          !CanRespondTap(!first_active_target_.expired()
                             ? first_active_target_.lock().get()
                             : nullptr)) {
        DeactivatePseudoStatus(PseudoStatus::kActive);
      }
    }
  }
}

void EventDispatcher::OnTouchUp(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    if (IsPrimaryInput(event, OH_ArkUI_PointerEvent_GetPointerId(event, i))) {
      if (!enable_multi_touch_) {
        DispatchSingleTouchEvent(TouchEvent::UP, event);
      }
      UpdateFocusedTarget();
      DeactivatePseudoStatus(PseudoStatus::kAll);
      break;
    }
  }
}

void EventDispatcher::OnTouchCancel(const ArkUI_UIInputEvent* event) {
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (IsPrimaryInput(event, OH_ArkUI_PointerEvent_GetPointerId(event, i))) {
      ResetClickEnv();
      UpdateFocusedTarget();
      DeactivatePseudoStatus(PseudoStatus::kAll);
      break;
    }
  }
}

void EventDispatcher::OnTouchDown(const EmulatedTouchPoint& point) {
  first_touch_moved_ = false;
  first_touch_outside_ = false;
  gesture_recognized_target_set_.clear();
  event_target_chain_.clear();
  InitClickEnv();
  if (!enable_multi_touch_) {
    DispatchSingleTouchEvent(TouchEvent::START, point);
  }
  ActivePseudoStatus();
  ui_owner_->SetActiveUIToGestureArenaAtDownEvent(first_active_target_);
}

void EventDispatcher::OnTouchMove(const EmulatedTouchPoint& point) {
  bool first_touch_changed = false;
  float pre_page_point[2] = {0.f};
  float page_point[2] = {point.page_point[0], point.page_point[1]};
  if (auto touch_target = active_target_finger_map_.find(point.pointer_id);
      touch_target != active_target_finger_map_.end()) {
    touch_target->second.GetPrePoint(pre_page_point);
    if (base::FloatsNotEqual(page_point[0], pre_page_point[0]) ||
        base::FloatsNotEqual(page_point[1], pre_page_point[1])) {
      first_touch_changed = true;
      has_touch_moved_ = true;
      touch_target->second.SetPrePoint(page_point);
      if (!first_touch_moved_) {
        float down_page_point[2] = {0.f};
        touch_target->second.GetDownPoint(down_page_point);
        const float dx = page_point[0] - down_page_point[0];
        const float dy = page_point[1] - down_page_point[1];
        if (base::FloatsLarger(dx * dx + dy * dy,
                               static_cast<float>(tap_slop_ * tap_slop_))) {
          first_touch_moved_ = true;
        }
      }
    }
  }

  if (first_touch_changed) {
    if (!click_target_chain_.empty()) {
      auto active_target = FindTarget(page_point);
      auto click_target = click_target_chain_.front();
      first_touch_outside_ =
          first_touch_outside_ || IsTouchMoveOutside(active_target) ||
          !CanRespondTap(!click_target.expired() ? click_target.lock().get()
                                                 : nullptr);
    }
    if (first_touch_moved_ ||
        !CanRespondTap(!first_active_target_.expired()
                           ? first_active_target_.lock().get()
                           : nullptr)) {
      DeactivatePseudoStatus(PseudoStatus::kActive);
    }
  }
}

void EventDispatcher::OnTouchUp(const EmulatedTouchPoint& point) {
  if (!enable_multi_touch_) {
    DispatchSingleTouchEvent(TouchEvent::UP, point);
  }
  OnTapEvent(point);
  OnClickEvent(point);
  ResetClickEnv();
  UpdateFocusedTarget();
  DeactivatePseudoStatus(PseudoStatus::kAll);
}

void EventDispatcher::OnTapEvent(const EmulatedTouchPoint& point) {
  bool can_respond_tap = !first_active_target_.expired()
                             ? CanRespondTap(first_active_target_.lock().get())
                             : false;
  if (first_active_target_.expired() || first_touch_moved_ ||
      !can_respond_tap) {
    LOGI("EventDispatcher OnTapEvent synthetic tap failed: "
         << first_active_target_.expired() << ", " << first_touch_moved_ << ", "
         << can_respond_tap)
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::TAP, point);
}

void EventDispatcher::OnClickEvent(const EmulatedTouchPoint& point) {
  if (click_target_chain_.empty()) {
    return;
  }
  auto first_click_target = click_target_chain_.front();
  bool can_respond_tap = !first_click_target.expired()
                             ? CanRespondTap(first_click_target.lock().get())
                             : false;
  if (first_click_target.expired() || first_touch_outside_ ||
      !can_respond_tap) {
    LOGI("EventDispatcher OnClickEvent synthetic click failed: "
         << first_click_target.expired() << ", " << first_touch_outside_ << ", "
         << can_respond_tap);
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::CLICK, point);
}

EventTarget* EventDispatcher::FindTarget(float point[2]) {
  if (root_target_.expired()) {
    return nullptr;
  }
  auto root = root_target_.lock();
  if (!root->IsOverlayContent()) {
    std::vector<UIBase*> excluded_roots;
    for (auto it = active_overlay_hit_test_roots_.begin();
         it != active_overlay_hit_test_roots_.end();) {
      auto overlay = it->root.lock();
      if (!overlay) {
        it = active_overlay_hit_test_roots_.erase(it);
        continue;
      }
      if (it->pass_through && overlay->Parent()) {
        excluded_roots.push_back(overlay->Parent());
      }
      ++it;
    }
    if (!excluded_roots.empty()) {
      return root->HitTestExcluding(point, excluded_roots);
    }
  }
  return root->HitTest(point);
}

bool EventDispatcher::CanRespondTap(EventTarget* active_target) {
  if (active_target == nullptr) {
    return false;
  }
  if (gesture_recognized_target_set_.empty()) {
    return true;
  }

  while (active_target != nullptr &&
         active_target->ParentTarget() != active_target) {
    if (gesture_recognized_target_set_.find(active_target->Sign()) !=
        gesture_recognized_target_set_.end()) {
      return false;
    }
    active_target = active_target->ParentTarget();
  }
  return true;
}

void EventDispatcher::UpdateFocusedTarget() {
  auto active_target = first_active_target_.lock();
  auto focused_target = focused_target_.lock();
  if (active_target && !active_target->IgnoreFocus()) {
    if (focused_target) {
      if (focused_target != active_target) {
        focused_target->OnFocusChange(
            false, active_target != nullptr && active_target->Focusable());
      }
    }
    active_target->OnFocusChange(
        true, focused_target != nullptr && focused_target->Focusable());
    SetFocusedTarget(first_active_target_);
  }
}

void EventDispatcher::GetTargetPoint(EventTarget* active_target,
                                     float target_point[2],
                                     float page_point[2]) {
  active_target = active_target->FirstUITarget();
  if (!active_target) {
    return;
  }
  UIBase* active_ui = reinterpret_cast<UIBase*>(active_target);
  if (active_ui && !ui_owner_->Destroyed()) {
    LynxUIHelper::ConvertPointFromAncestorToDescendant(
        target_point, ui_owner_->Root(), active_ui, page_point);
  }
}

void EventDispatcher::GetPagePoint(float page_point[2], float node_point[2]) {
  if (from_overlay_ && !ui_owner_->Destroyed()) {
    auto root = ui_owner_->Root();

    ArkUI_IntOffset page_offset;
    OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->GetProxyNode(),
                                                        &page_offset);
    float node_point_x = node_point[0], node_point_y = node_point[1];
    float scaled_density = root->GetContext()->ScaledDensity();
    page_point[0] = node_point_x - page_offset.x / scaled_density;
    page_point[1] = node_point_y - page_offset.y / scaled_density;
  }
  if (has_event_point_offset_) {
    page_point[0] += event_point_offset_[0];
    page_point[1] += event_point_offset_[1];
  }
}

void EventDispatcher::GetEventPagePoint(float page_point[2],
                                        const ArkUI_UIInputEvent* event,
                                        size_t index, float point_scale) {
  // ArkUI events are local to the receiving root. GetPagePoint retains the
  // coordinate conversion for events forwarded by the legacy overlay path.
  page_point[0] = OH_ArkUI_PointerEvent_GetXByIndex(event, index) / point_scale;
  page_point[1] = OH_ArkUI_PointerEvent_GetYByIndex(event, index) / point_scale;
  GetPagePoint(page_point, page_point);
}

EventDispatcher::InputEventPoint EventDispatcher::GetInputEventPoint(
    EventTarget* target, const ArkUI_UIInputEvent* event, size_t index) {
  InputEventPoint point;
  GetEventPagePoint(point.page, event, index);
  point.target[0] = point.page[0];
  point.target[1] = point.page[1];
  GetTargetPoint(target, point.target, point.page);
  point.client[0] = OH_ArkUI_PointerEvent_GetWindowXByIndex(event, index);
  point.client[1] = OH_ArkUI_PointerEvent_GetWindowYByIndex(event, index);
  return point;
}

void EventDispatcher::DispatchPointerEvent(
    const std::string& name, EventTarget* target,
    const ArkUI_UIInputEvent* event, size_t index,
    const std::string& pointer_type, bool is_primary, int button, int buttons) {
  if (!target) {
    return;
  }
  InputEventPoint point = GetInputEventPoint(target, event, index);
  auto params = lepus::Dictionary::Create();
  params->SetValue("type", name);
  params->SetValue("pointerId",
                   OH_ArkUI_PointerEvent_GetPointerId(event, index));
  params->SetValue("pointerType", pointer_type);
  params->SetValue("isPrimary", is_primary);
  params->SetValue("button", button);
  params->SetValue("buttons", buttons);
  params->SetValue("x", point.target[0]);
  params->SetValue("y", point.target[1]);
  params->SetValue("pageX", point.page[0]);
  params->SetValue("pageY", point.page[1]);
  params->SetValue("clientX", point.client[0]);
  params->SetValue("clientY", point.client[1]);
  params->SetValue("timestamp", time_stamp_);
  AddModifierProperties(params.get(), event);
  BubbleEvent pointer_event(target->Sign(), name, LynxEventType::kPointer,
                            lepus::Value(std::move(params)));
  ui_owner_->SendEvent(pointer_event);
}

void EventDispatcher::DispatchTouchPointerEvents(
    const std::string& name, const ArkUI_UIInputEvent* event, int button,
    int buttons, const std::vector<int32_t>* pointer_ids) {
  size_t pointer_count = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t index = 0; index < pointer_count; ++index) {
    if (!pointer_ids && name != "pointercancel" &&
        !IsActiveFinger(event, index)) {
      continue;
    }
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, index);
    if (pointer_ids && std::find(pointer_ids->begin(), pointer_ids->end(),
                                 pointer_id) == pointer_ids->end()) {
      continue;
    }
    auto target_it = active_target_finger_map_.find(pointer_id);
    if (target_it == active_target_finger_map_.end()) {
      continue;
    }
    auto tool_it = pointer_tool_types_.find(pointer_id);
    int tool_type = tool_it == pointer_tool_types_.end()
                        ? OH_ArkUI_UIInputEvent_GetToolType(event)
                        : tool_it->second;
    auto target = target_it->second.ActiveTarget().lock();
    DispatchPointerEvent(name, target.get(), event, index,
                         PointerTypeForTool(tool_type),
                         IsPrimaryPointer(pointer_id), button, buttons);
  }
}

void EventDispatcher::DispatchMouseEvent(const std::string& name,
                                         EventTarget* target,
                                         const ArkUI_UIInputEvent* event,
                                         int button, int buttons) {
  if (!target) {
    return;
  }
  InputEventPoint point = GetInputEventPoint(target, event, 0);
  auto params = lepus::Dictionary::Create();
  params->SetValue("type", name);
  params->SetValue("button", button);
  params->SetValue("buttons", buttons);
  params->SetValue("scale", 1.f);
  params->SetValue("x", point.target[0]);
  params->SetValue("y", point.target[1]);
  params->SetValue("pageX", point.page[0]);
  params->SetValue("pageY", point.page[1]);
  params->SetValue("clientX", point.client[0]);
  params->SetValue("clientY", point.client[1]);
  params->SetValue("identifier", OH_ArkUI_PointerEvent_GetPointerId(event, 0));
  params->SetValue("timestamp", time_stamp_);
  AddModifierProperties(params.get(), event);
  BubbleEvent mouse_event(target->Sign(), name, LynxEventType::kMouse,
                          lepus::Value(std::move(params)));
  ui_owner_->SendEvent(mouse_event);
}

void EventDispatcher::DispatchWheelEvent(EventTarget* target,
                                         const ArkUI_UIInputEvent* event) {
  if (!target) {
    return;
  }
  InputEventPoint point = GetInputEventPoint(target, event, 0);
  auto params = lepus::Dictionary::Create();
  params->SetValue("type", "wheel");
  params->SetValue("x", point.target[0]);
  params->SetValue("y", point.target[1]);
  params->SetValue("pageX", point.page[0]);
  params->SetValue("pageY", point.page[1]);
  params->SetValue("clientX", point.client[0]);
  params->SetValue("clientY", point.client[1]);
  params->SetValue("deltaX", OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event));
  params->SetValue("deltaY", OH_ArkUI_AxisEvent_GetVerticalAxisValue(event));
  params->SetValue("timestamp", time_stamp_);
  AddModifierProperties(params.get(), event);
  BubbleEvent wheel_event(target->Sign(), "wheel", LynxEventType::kWheel,
                          lepus::Value(std::move(params)));
  ui_owner_->SendEvent(wheel_event);
}

bool EventDispatcher::HasEventInChain(EventTarget* target,
                                      const std::string& name) const {
  while (target) {
    const auto events = target->EventSet();
    if (std::find(events.begin(), events.end(), name) != events.end()) {
      return true;
    }
    EventTarget* parent = target->ParentTarget();
    if (parent == target) {
      break;
    }
    target = parent;
  }
  return false;
}

bool EventDispatcher::IsTextInputTarget(EventTarget* target) const {
  if (!target) {
    return false;
  }
  auto* ui_target = static_cast<UIBase*>(target->FirstUITarget());
  if (!ui_target) {
    return false;
  }
  const std::string& tag = ui_target->Tag();
  return tag == "input" || tag == "textarea" || tag == "x-input" ||
         tag == "x-textarea" || tag == "x-input-ng" || tag == "x-textarea-ng";
}

void EventDispatcher::DispatchActivationClick(EventTarget* target,
                                              const InputEventPoint* point) {
  if (!target || !HasEventInChain(target, TouchEvent::CLICK)) {
    return;
  }
  TouchEvent click_event(target->Sign(), TouchEvent::CLICK);
  if (point) {
    float target_point[2] = {point->target[0], point->target[1]};
    float page_point[2] = {point->page[0], point->page[1]};
    float client_point[2] = {point->client[0], point->client[1]};
    click_event.SetTargetPoint(target_point);
    click_event.SetPagePoint(page_point);
    click_event.SetClientPoint(client_point);
  }
  click_event.SetTimeStamp(time_stamp_);
  click_event.SetTarget(target->WeakTarget());
  ui_owner_->SendEvent(click_event);
}

void EventDispatcher::GetEventPointOffset(float point_offset[2]) const {
  point_offset[0] = has_event_point_offset_ ? event_point_offset_[0] : 0.f;
  point_offset[1] = has_event_point_offset_ ? event_point_offset_[1] : 0.f;
}

void EventDispatcher::AddTargetTouchMap(lepus::Value& target_touch_map,
                                        const ArkUI_UIInputEvent* event) {
  auto dict = target_touch_map.Table();
  size_t num = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t i = 0; i < num; ++i) {
    if (!IsActiveFinger(event, i)) {
      continue;
    }
    int pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, i);
    if (auto touch_target = active_target_finger_map_.find(pointer_id);
        touch_target != active_target_finger_map_.end()) {
      auto active_target = touch_target->second.ActiveTarget().lock().get();
      if (!active_target) {
        continue;
      }

      std::string target_sign = std::to_string(active_target->Sign());
      float page_point[2] = {0.f};
      GetEventPagePoint(page_point, event, i);
      float target_point[2] = {page_point[0], page_point[1]};
      GetTargetPoint(active_target, target_point, page_point);
      float client_point[2] = {
          OH_ArkUI_PointerEvent_GetWindowXByIndex(event, i),
          OH_ArkUI_PointerEvent_GetWindowYByIndex(event, i)};

      auto touch = lepus::CArray::Create();
      touch->emplace_back(pointer_id);
      touch->emplace_back(client_point[0]);
      touch->emplace_back(client_point[1]);
      touch->emplace_back(page_point[0]);
      touch->emplace_back(page_point[1]);
      touch->emplace_back(target_point[0]);
      touch->emplace_back(target_point[1]);

      if (auto it = dict->find(target_sign); it != dict->end()) {
        it->second.Array()->emplace_back(std::move(touch));
      } else {
        auto array = lepus::CArray::Create();
        array->emplace_back(std::move(touch));
        dict->SetValue(target_sign, std::move(array));
      }
    }
  }
}

void EventDispatcher::MarkDispatchInCurrentLynxPageOnly(
    TouchEvent& touch_event) const {
  if (dispatch_touch_event_in_current_lynx_page_only_) {
    touch_event.SetEventID(kCurrentLynxPageOnlyEventID);
  }
}

void EventDispatcher::AddTargetTouchMap(lepus::Value& target_touch_map,
                                        const EmulatedTouchPoint& point) {
  auto dict = target_touch_map.Table();
  if (auto touch_target = active_target_finger_map_.find(point.pointer_id);
      touch_target != active_target_finger_map_.end()) {
    auto active_target = touch_target->second.ActiveTarget().lock().get();
    if (!active_target) {
      return;
    }

    std::string target_sign = std::to_string(active_target->Sign());
    float page_point[2] = {point.page_point[0], point.page_point[1]};
    float target_point[2] = {page_point[0], page_point[1]};
    GetTargetPoint(active_target, target_point, page_point);
    float client_point[2] = {point.client_point[0], point.client_point[1]};

    auto touch = lepus::CArray::Create();
    touch->emplace_back(point.pointer_id);
    touch->emplace_back(client_point[0]);
    touch->emplace_back(client_point[1]);
    touch->emplace_back(page_point[0]);
    touch->emplace_back(page_point[1]);
    touch->emplace_back(target_point[0]);
    touch->emplace_back(target_point[1]);

    if (auto it = dict->find(target_sign); it != dict->end()) {
      it->second.Array()->emplace_back(std::move(touch));
    } else {
      auto array = lepus::CArray::Create();
      array->emplace_back(std::move(touch));
      dict->SetValue(target_sign, std::move(array));
    }
  }
}

void EventDispatcher::SetEnableMultiTouch(bool enable_multi_touch) {
  enable_multi_touch_ = enable_multi_touch;
}

void EventDispatcher::SetTapSlop(const std::string& tap_slop) {
  if (tap_gesture_) {
    return;
  }
  float screen_size[2] = {0};
  ui_owner_->Context()->ScreenSize(screen_size);
  tap_slop_ = LynxUnitUtils::ToVPFromUnitValue(
      tap_slop, screen_size[0], ui_owner_->Context()->DevicePixelRatio(), 5.f);
  tap_slop_ = tap_slop_ > 0 ? tap_slop_ : 5.f;
  tap_gesture_ = NodeManager::Instance().createTapGestureWithDistanceThreshold(
      1, 1, tap_slop_);
  if (tap_gesture_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetGestureEventTarget(
      tap_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this,
      EventDispatcher::tap_receiver_callback_);
  if (!root_target_.expired()) {
    NodeManager::Instance().AddGestureToNode(root_target_.lock()->RootNode(),
                                             tap_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }
}

void EventDispatcher::SetLongPressDuration(int32_t long_press_duration) {
  if (long_press_gesture_) {
    return;
  }
  long_press_duration_ = long_press_duration > 0 ? long_press_duration : 500;
  long_press_gesture_ = NodeManager::Instance().CreateLongPressGesture(
      1, false, long_press_duration_);
  if (long_press_gesture_ == nullptr) {
    return;
  }
  NodeManager::Instance().SetGestureEventTarget(
      long_press_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this,
      EventDispatcher::long_press_receiver_callback_);
  if (!root_target_.expired()) {
    NodeManager::Instance().AddGestureToNode(root_target_.lock()->RootNode(),
                                             long_press_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }
}

void EventDispatcher::OnGestureRecognized(UIBase* ui) {
  gesture_recognized_target_set_.insert(ui->Sign());
}

void EventDispatcher::OnGestureRecognizedWithSign(int sign) {
  gesture_recognized_target_set_.insert(sign);
}

void EventDispatcher::HandleTouchDown(const ArkUI_UIInputEvent* event) {
  if (active_target_finger_map_.empty()) {
    retained_text_event_targets_.clear();
  }
  InitTouchEnv(event);
  if (EventThrough()) {
    ResetTouchEnv(event);
    return;
  }
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  if (enable_multi_touch_) {
    DispatchMultiTouchEvent(TouchEvent::START, target_touch_map, event);
  }
  OnTouchDown(event);
  DispatchTouchPointerEvents("pointerdown", event, 0, 1);
  DispatchTouchEventToChildLynxPage(event);
}

void EventDispatcher::HandleTouchMove(const ArkUI_UIInputEvent* event) {
  std::vector<int32_t> changed_pointer_ids;
  size_t pointer_count = OH_ArkUI_PointerEvent_GetPointerCount(event);
  for (size_t index = 0; index < pointer_count; ++index) {
    int32_t pointer_id = OH_ArkUI_PointerEvent_GetPointerId(event, index);
    auto target_it = active_target_finger_map_.find(pointer_id);
    if (target_it == active_target_finger_map_.end()) {
      continue;
    }
    float page_point[2] = {0.f, 0.f};
    float previous_point[2] = {0.f, 0.f};
    GetEventPagePoint(page_point, event, index);
    target_it->second.GetPrePoint(previous_point);
    if (base::FloatsNotEqual(page_point[0], previous_point[0]) ||
        base::FloatsNotEqual(page_point[1], previous_point[1])) {
      changed_pointer_ids.push_back(pointer_id);
    }
  }
  OnTouchMove(event);
  if (has_touch_moved_) {
    auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
    AddTargetTouchMap(target_touch_map, event);
    if (enable_multi_touch_) {
      DispatchMultiTouchEvent(TouchEvent::MOVE, target_touch_map, event);
    } else {
      DispatchSingleTouchEvent(TouchEvent::MOVE, event);
    }
  }
  DispatchTouchPointerEvents("pointermove", event, -1, 1, &changed_pointer_ids);
  DispatchTouchEventToChildLynxPage(event);
}

void EventDispatcher::HandleTouchUp(const ArkUI_UIInputEvent* event) {
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  if (enable_multi_touch_) {
    DispatchMultiTouchEvent(TouchEvent::UP, target_touch_map, event);
  }
  OnTouchUp(event);
  DispatchTouchPointerEvents("pointerup", event, 0, 0);
  ResetTouchEnv(event);
  DispatchTouchEventToChildLynxPage(event);
}

void EventDispatcher::HandleTouchCancel(const ArkUI_UIInputEvent* event) {
  auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
  AddTargetTouchMap(target_touch_map, event);
  if (enable_multi_touch_) {
    DispatchMultiTouchEvent(TouchEvent::CANCEL, target_touch_map, event);
  } else {
    DispatchSingleTouchEvent(TouchEvent::CANCEL, event);
  }
  DispatchTouchPointerEvents("pointercancel", event, -1, 0);
  OnTouchCancel(event);
  ResetTouchEnv(event);
  DispatchTouchEventToChildLynxPage(event);
}

void EventDispatcher::ActivePseudoStatus() {
  if (first_active_target_.expired()) {
    return;
  }
  EventTarget* current = first_active_target_.lock().get();
  while (current != nullptr && current->ParentTarget() != current) {
    event_target_chain_.push_back(current->WeakTarget());
    current->OnPseudoStatusChanged(PseudoStatus::kNone, PseudoStatus::kActive);
    if (has_touch_pseudo_) {
      ui_owner_->SendPseudoStatusEvent(current->Sign(), PseudoStatus::kNone,
                                       PseudoStatus::kActive);
    }
    if (!current->TouchPseudoPropagation()) {
      break;
    }
    current = current->ParentTarget();
  }
}

void EventDispatcher::DeactivatePseudoStatus(PseudoStatus status) {
  int32_t int_status = static_cast<int32_t>(status);
  for (auto target : event_target_chain_) {
    if (target.expired()) {
      continue;
    }
    auto current = target.lock();
    int32_t current_status = static_cast<int32_t>(current->GetPseudoStatus());
    current->OnPseudoStatusChanged(
        static_cast<PseudoStatus>(current_status),
        static_cast<PseudoStatus>(current_status & ~int_status));
    if (has_touch_pseudo_) {
      ui_owner_->SendPseudoStatusEvent(
          current->Sign(), static_cast<PseudoStatus>(current_status),
          static_cast<PseudoStatus>(current_status & ~int_status));
    }
  }
}

void EventDispatcher::OnGetVelocity(const ArkUI_GestureEvent* event) {
  float velocity_x = OH_ArkUI_PanGesture_GetVelocityX(event);
  float velocity_y = OH_ArkUI_PanGesture_GetVelocityY(event);
  if (ui_owner_) {
    ui_owner_->SetVelocityToGestureArena(velocity_x, velocity_y);
  }
}

bool EventDispatcher::IsTouchMoveOutside(EventTarget* target) {
  if (target == nullptr) {
    return true;
  }

  std::vector<EventTarget*> target_chain;
  while (target != nullptr && target->ParentTarget() != target) {
    target_chain.push_back(target);
    target = target->ParentTarget();
  }
  if (target_chain.size() < click_target_chain_.size()) {
    return true;
  }

  for (size_t i = 0; i < click_target_chain_.size(); ++i) {
    if (click_target_chain_[i].expired() ||
        click_target_chain_[i].lock().get() != target_chain[i]) {
      return true;
    }
  }
  return false;
}

bool EventDispatcher::IsActiveFinger(const ArkUI_UIInputEvent* event,
                                     size_t index) {
  if (auto action = OH_ArkUI_UIInputEvent_GetAction(event);
      action == UI_TOUCH_EVENT_ACTION_MOVE ||
      action == UI_TOUCH_EVENT_ACTION_CANCEL) {
    return true;
  }
  float active_x = OH_ArkUI_PointerEvent_GetX(event);
  float active_y = OH_ArkUI_PointerEvent_GetY(event);
  float finger_x = OH_ArkUI_PointerEvent_GetXByIndex(event, index);
  float finger_y = OH_ArkUI_PointerEvent_GetYByIndex(event, index);
  return base::FloatsEqual(active_x, finger_x) &&
         base::FloatsEqual(active_y, finger_y);
}

bool EventDispatcher::IsPrimaryInput(const ArkUI_UIInputEvent*,
                                     int pointer_id) {
  return primary_pointer_id_.has_value() &&
         primary_pointer_id_.value() == pointer_id;
}

bool EventDispatcher::IsPrimaryPointer(int pointer_id) const {
  auto tool_it = pointer_tool_types_.find(pointer_id);
  if (tool_it == pointer_tool_types_.end()) {
    return false;
  }
  auto primary_it = primary_pointer_ids_by_tool_.find(tool_it->second);
  return primary_it != primary_pointer_ids_by_tool_.end() &&
         primary_it->second == pointer_id;
}

bool EventDispatcher::ShouldDispatchInCurrentLynxPageOnly(UIBase* root) const {
  return root != ui_owner_->Root();
}

UIBase* EventDispatcher::GetChildLynxPageUI(EventTarget* active_target) {
  auto* children_lynx_page_ui = active_target->ChildrenLynxPageUI();
  if (!children_lynx_page_ui) {
    return nullptr;
  }
  auto child_it = children_lynx_page_ui->find(active_target);
  if (child_it == children_lynx_page_ui->end()) {
    return nullptr;
  }
  return static_cast<UIBase*>(child_it->second.lock().get());
}

void EventDispatcher::PrepareChildEventPointOffset(
    const ArkUI_UIInputEvent* event, EventTarget* active_target,
    float point_offset[2], float scale) {
  float raw_page_point[2] = {
      OH_ArkUI_PointerEvent_GetXByIndex(event, 0) / scale,
      OH_ArkUI_PointerEvent_GetYByIndex(event, 0) / scale};
  float page_point[2] = {0.f};
  GetEventPagePoint(page_point, event, 0, scale);
  float target_point[2] = {page_point[0], page_point[1]};
  GetTargetPoint(active_target, target_point, page_point);
  point_offset[0] = target_point[0] - raw_page_point[0];
  point_offset[1] = target_point[1] - raw_page_point[1];
}

void EventDispatcher::DispatchEventToChildLynxPage(
    const ArkUI_UIInputEvent* event, ChildLynxPageEventType event_type,
    float scale) {
  auto active_target = first_active_target_.lock();
  if (!active_target) {
    return;
  }
  UIBase* child_lynx_page_ui = GetChildLynxPageUI(active_target.get());
  if (!child_lynx_page_ui) {
    return;
  }
  auto* child_event_dispatcher =
      child_lynx_page_ui->GetContext()->GetUIOwner()->GetEventDispatcher();

  float point_offset[2] = {0.f, 0.f};
  PrepareChildEventPointOffset(event, active_target.get(), point_offset, scale);

  child_event_dispatcher->has_event_point_offset_ = true;
  child_event_dispatcher->event_point_offset_[0] = point_offset[0];
  child_event_dispatcher->event_point_offset_[1] = point_offset[1];

  switch (event_type) {
    case ChildLynxPageEventType::kTouch: {
      child_event_dispatcher->time_stamp_ = time_stamp_;
      NodeManager::Instance().SetEventDispatcher(child_event_dispatcher);
      auto action = OH_ArkUI_UIInputEvent_GetAction(event);
      if (action == UI_TOUCH_EVENT_ACTION_DOWN) {
        child_event_dispatcher->from_overlay_ = false;
        child_event_dispatcher->root_target_ =
            child_lynx_page_ui->weak_from_this();
        child_event_dispatcher
            ->dispatch_touch_event_in_current_lynx_page_only_ =
            child_event_dispatcher->ShouldDispatchInCurrentLynxPageOnly(
                child_lynx_page_ui);
        child_event_dispatcher->HandleTouchDown(event);
      } else if (!child_event_dispatcher->first_active_target_.expired() &&
                 !child_event_dispatcher->active_target_finger_map_.empty()) {
        if (child_event_dispatcher->EventThrough()) {
          break;
        }
        switch (action) {
          case UI_TOUCH_EVENT_ACTION_MOVE:
            child_event_dispatcher->HandleTouchMove(event);
            break;
          case UI_TOUCH_EVENT_ACTION_UP:
            child_event_dispatcher->HandleTouchUp(event);
            break;
          case UI_TOUCH_EVENT_ACTION_CANCEL:
            child_event_dispatcher->HandleTouchCancel(event);
            break;
          default:
            break;
        }
      }
      break;
    }
    case ChildLynxPageEventType::kClick:
      child_event_dispatcher->OnClickEvent(event);
      child_event_dispatcher->ResetClickEnv();
      break;
    case ChildLynxPageEventType::kTap:
      child_event_dispatcher->OnTapEvent(event);
      break;
    case ChildLynxPageEventType::kLongPress:
      child_event_dispatcher->OnLongPressEvent(event);
      break;
  }

  NodeManager::Instance().SetEventDispatcher(this);
}

void EventDispatcher::DispatchTouchEventToChildLynxPage(
    const ArkUI_UIInputEvent* event) {
  DispatchEventToChildLynxPage(event, ChildLynxPageEventType::kTouch);
}

void EventDispatcher::DispatchClickEventToChildLynxPage(
    const ArkUI_UIInputEvent* event) {
  DispatchEventToChildLynxPage(event, ChildLynxPageEventType::kClick);
}

void EventDispatcher::DispatchTapEventToChildLynxPage(
    const ArkUI_UIInputEvent* event) {
  float scale = ui_owner_->Context()->ScaledDensity();
  DispatchEventToChildLynxPage(event, ChildLynxPageEventType::kTap, scale);
}

void EventDispatcher::DispatchLongPressEventToChildLynxPage(
    const ArkUI_UIInputEvent* event) {
  float scale = ui_owner_->Context()->ScaledDensity();
  DispatchEventToChildLynxPage(event, ChildLynxPageEventType::kLongPress,
                               scale);
}

void EventDispatcher::DispatchActiveTargetTouchEvent(
    const ArkUI_UIInputEvent* event) {
  auto active_target = first_active_target_.lock();
  if (!active_target) {
    return;
  }
  active_target->DispatchTouch(event);
  DispatchActiveTargetTouchEventToChildLynxPage(event);
}

void EventDispatcher::DispatchActiveTargetTouchEventToChildLynxPage(
    const ArkUI_UIInputEvent* event) {
  auto active_target = first_active_target_.lock();
  if (!active_target) {
    return;
  }
  UIBase* child_lynx_page_ui = GetChildLynxPageUI(active_target.get());
  if (!child_lynx_page_ui) {
    return;
  }
  auto* child_event_dispatcher =
      child_lynx_page_ui->GetContext()->GetUIOwner()->GetEventDispatcher();
  child_event_dispatcher->DispatchActiveTargetTouchEvent(event);
  NodeManager::Instance().SetEventDispatcher(this);
}

void EventDispatcher::DispatchTouchEventToChildGestureArena(
    const std::string& event_name, const ArkUI_UIInputEvent* event) {
  auto active_target = first_active_target_.lock();
  if (!active_target) {
    return;
  }
  UIBase* child_lynx_page_ui = GetChildLynxPageUI(active_target.get());
  if (!child_lynx_page_ui) {
    return;
  }
  auto* child_event_dispatcher =
      child_lynx_page_ui->GetContext()->GetUIOwner()->GetEventDispatcher();
  child_event_dispatcher->DispatchTouchEventToGestureArena(event_name, event);
  NodeManager::Instance().SetEventDispatcher(this);
}

void EventDispatcher::DispatchTouchEventToGestureArena(
    const std::string& event_name, const ArkUI_UIInputEvent* event) {
  if (!first_active_target_.expired() && ui_owner_->GetGestureArenaManager()) {
    if (event_name == TouchEvent::START) {
      ui_owner_->SetActiveUIToGestureArenaAtDownEvent(first_active_target_);
    }
    if (last_touch_event_ != nullptr &&
        (event_name == TouchEvent::START || event_name == TouchEvent::MOVE ||
         event_name == TouchEvent::UP || event_name == TouchEvent::CANCEL)) {
      ui_owner_->DispatchTouchEventToGestureArena(event_name, last_touch_event_,
                                                  event);
    }
  }
  DispatchTouchEventToChildGestureArena(event_name, event);
}

void EventDispatcher::OnTouchEvent(const ArkUI_UIInputEvent* event,
                                   UIBase* root, bool from_overlay) {
  if (ui_owner_->Destroyed()) {
    return;
  }
  time_stamp_ = CurrentTimestampMilliseconds();
  NodeManager::Instance().SetEventDispatcher(this);
  auto action = OH_ArkUI_UIInputEvent_GetAction(event);
  std::string event_name;
  if (action == UI_TOUCH_EVENT_ACTION_DOWN) {
    event_name = TouchEvent::START;
    float active_x = OH_ArkUI_PointerEvent_GetX(event);
    float active_y = OH_ArkUI_PointerEvent_GetY(event);
    LOGI("EventDispatcher OnTouchEvent down x:" << active_x
                                                << ", y:" << active_y)
    ShowMessageOnConsole(
        "EventDispatcher: receive touch for lynx " + ui_owner_->Id() +
            ", touch: " + std::to_string(UI_TOUCH_EVENT_ACTION_DOWN) + " x: " +
            std::to_string(active_x) + " y: " + std::to_string(active_y),
        runtime::CONSOLE_LOG_INFO);
    from_overlay_ = from_overlay;
    root_target_ = root->weak_from_this();
    dispatch_touch_event_in_current_lynx_page_only_ =
        ShouldDispatchInCurrentLynxPageOnly(root);
    HandleTouchDown(event);
  } else if (!first_active_target_.expired() &&
             !active_target_finger_map_.empty()) {
    if (EventThrough()) {
      return;
    }
    switch (action) {
      case UI_TOUCH_EVENT_ACTION_MOVE: {
        event_name = TouchEvent::MOVE;
        HandleTouchMove(event);
        break;
      }
      case UI_TOUCH_EVENT_ACTION_UP: {
        event_name = TouchEvent::UP;
        float active_x = OH_ArkUI_PointerEvent_GetX(event);
        float active_y = OH_ArkUI_PointerEvent_GetY(event);
        ShowMessageOnConsole(
            "EventDispatcher: receive touch for lynx " + ui_owner_->Id() +
                ", touch: " + std::to_string(UI_TOUCH_EVENT_ACTION_UP) +
                " x: " + std::to_string(active_x) +
                " y: " + std::to_string(active_y),
            runtime::CONSOLE_LOG_INFO);
        HandleTouchUp(event);
        OnClickEvent(event);
        ResetClickEnv();
        break;
      }
      case UI_TOUCH_EVENT_ACTION_CANCEL: {
        event_name = TouchEvent::CANCEL;
        float active_x = OH_ArkUI_PointerEvent_GetX(event);
        float active_y = OH_ArkUI_PointerEvent_GetY(event);
        ShowMessageOnConsole(
            "EventDispatcher: receive touch for lynx " + ui_owner_->Id() +
                ", touch: " + std::to_string(UI_TOUCH_EVENT_ACTION_CANCEL) +
                " x: " + std::to_string(active_x) +
                " y: " + std::to_string(active_y),
            runtime::CONSOLE_LOG_INFO);
        HandleTouchCancel(event);
        break;
      }
    }
  }

  if (EventThrough() ||
      (action == UI_TOUCH_EVENT_ACTION_MOVE && !has_touch_moved_)) {
    return;
  }

  DispatchActiveTargetTouchEvent(event);
  DispatchTouchEventToGestureArena(event_name, event);
}

void EventDispatcher::OnMouseEvent(const ArkUI_UIInputEvent* event,
                                   UIBase* root, bool from_overlay) {
  if (ui_owner_->Destroyed() || !event || !root) {
    return;
  }
  time_stamp_ = CurrentTimestampMilliseconds();
  NodeManager::Instance().SetEventDispatcher(this);
  from_overlay_ = from_overlay;
  root_target_ = root->weak_from_this();

  float page_point[2] = {0.f, 0.f};
  GetEventPagePoint(page_point, event, 0);
  auto target = FindTarget(page_point);
  auto down_target = mouse_down_target_.lock();
  EventTarget* event_target = target ? target : down_target.get();
  int action = OH_ArkUI_MouseEvent_GetMouseAction(event);
  int native_button = OH_ArkUI_MouseEvent_GetMouseButton(event);
  int button = ToDOMMouseButton(native_button);
  int button_mask = ToDOMMouseButtonMask(native_button);

  if (event_target) {
    InputEventPoint point = GetInputEventPoint(event_target, event, 0);
    if (event_target->EventThrough(point.target)) {
      if (action == UI_MOUSE_EVENT_ACTION_RELEASE) {
        mouse_buttons_ &= ~button_mask;
      } else if (action == UI_MOUSE_EVENT_ACTION_CANCEL) {
        mouse_buttons_ = 0;
      }
      if (mouse_buttons_ == 0) {
        mouse_down_target_.reset();
        mouse_activation_button_ = -1;
      }
      return;
    }
  }

  switch (action) {
    case UI_MOUSE_EVENT_ACTION_PRESS: {
      if (!target) {
        return;
      }
      bool starts_activation = mouse_buttons_ == 0;
      mouse_buttons_ |= button_mask;
      if (starts_activation) {
        mouse_down_target_ = target->WeakTarget();
        mouse_activation_button_ = button;
      }
      DispatchPointerEvent("pointerdown", target, event, 0, "mouse", true,
                           button, mouse_buttons_);
      DispatchMouseEvent("mousedown", target, event, button, mouse_buttons_);
      break;
    }
    case UI_MOUSE_EVENT_ACTION_MOVE: {
      if (!target) {
        return;
      }
      DispatchPointerEvent("pointermove", target, event, 0, "mouse", true, -1,
                           mouse_buttons_);
      DispatchMouseEvent("mousemove", target, event, -1, mouse_buttons_);
      break;
    }
    case UI_MOUSE_EVENT_ACTION_RELEASE: {
      mouse_buttons_ &= ~button_mask;
      if (event_target) {
        DispatchPointerEvent("pointerup", event_target, event, 0, "mouse", true,
                             button, mouse_buttons_);
        DispatchMouseEvent("mouseup", event_target, event, button,
                           mouse_buttons_);
      }
      if (target && down_target && target == down_target.get() &&
          mouse_activation_button_ == button && button == 0) {
        InputEventPoint point = GetInputEventPoint(target, event, 0);
        DispatchMouseEvent("mouseclick", target, event, button, mouse_buttons_);
        DispatchActivationClick(target, &point);
      }
      if (mouse_buttons_ == 0) {
        mouse_down_target_.reset();
        mouse_activation_button_ = -1;
      }
      break;
    }
    case UI_MOUSE_EVENT_ACTION_CANCEL: {
      if (event_target) {
        DispatchPointerEvent("pointercancel", event_target, event, 0, "mouse",
                             true, -1, 0);
      }
      mouse_buttons_ = 0;
      mouse_down_target_.reset();
      mouse_activation_button_ = -1;
      break;
    }
    default:
      break;
  }
}

void EventDispatcher::OnAxisEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                                  bool from_overlay) {
  if (ui_owner_->Destroyed() || !event || !root) {
    return;
  }
  time_stamp_ = CurrentTimestampMilliseconds();
  NodeManager::Instance().SetEventDispatcher(this);
  from_overlay_ = from_overlay;
  root_target_ = root->weak_from_this();

  int action = UI_AXIS_EVENT_ACTION_NONE;
  if (auto get_axis_action = AxisActionFunction()) {
    action = get_axis_action(event);
  }
  if (action == UI_AXIS_EVENT_ACTION_END ||
      action == UI_AXIS_EVENT_ACTION_CANCEL) {
    axis_target_.reset();
    return;
  }

  float page_point[2] = {0.f, 0.f};
  GetEventPagePoint(page_point, event, 0);
  if (action == UI_AXIS_EVENT_ACTION_BEGIN || axis_target_.expired()) {
    if (auto* target = FindTarget(page_point)) {
      axis_target_ = target->WeakTarget();
    }
  }
  if (action == UI_AXIS_EVENT_ACTION_BEGIN) {
    return;
  }

  auto target = axis_target_.lock();
  if (!target) {
    return;
  }
  InputEventPoint point = GetInputEventPoint(target.get(), event, 0);
  if (!target->EventThrough(point.target)) {
    DispatchWheelEvent(target.get(), event);
  }
  if (action == UI_AXIS_EVENT_ACTION_NONE) {
    axis_target_.reset();
  }
}

void EventDispatcher::OnKeyEvent(const ArkUI_UIInputEvent* event) {
  if (ui_owner_->Destroyed() || !event) {
    return;
  }
  time_stamp_ = CurrentTimestampMilliseconds();
  auto get_key_type = KeyEventTypeFunction();
  auto get_key_code = KeyCodeFunction();
  auto get_key_text = KeyTextFunction();
  if (!get_key_type || !get_key_code || !get_key_text) {
    return;
  }

  ArkUI_KeyEventType key_type = get_key_type(event);
  int32_t key_code = get_key_code(event);
  bool was_pressed =
      pressed_key_codes_.find(key_code) != pressed_key_codes_.end();
  bool repeat = false;
  std::string event_name;
  if (key_type == ARKUI_KEY_EVENT_DOWN) {
    repeat = !pressed_key_codes_.insert(key_code).second;
    event_name = "keydown";
  } else if (key_type == ARKUI_KEY_EVENT_LONG_PRESS) {
    pressed_key_codes_.insert(key_code);
    repeat = true;
    event_name = "keydown";
  } else if (key_type == ARKUI_KEY_EVENT_UP) {
    event_name = "keyup";
    pressed_key_codes_.erase(key_code);
  } else {
    return;
  }

  auto target = focused_target_.lock();
  if (!target) {
    return;
  }
  auto params = lepus::Dictionary::Create();
  params->SetValue("type", event_name);
  params->SetValue("key", KeyTextForCode(key_code, get_key_text(event)));
  params->SetValue("repeat", repeat);
  params->SetValue("timestamp", time_stamp_);
  AddModifierProperties(params.get(), event);
  BubbleEvent key_event(target->Sign(), event_name, LynxEventType::kKeyboard,
                        lepus::Value(std::move(params)));
  ui_owner_->SendEvent(key_event);

  if (IsTextInputTarget(target.get())) {
    return;
  }
  bool activate =
      (key_code == ARKUI_KEYCODE_ENTER || key_code == ARKUI_KEYCODE_SPACE) &&
      key_type == ARKUI_KEY_EVENT_UP && was_pressed;
  if (activate) {
    DispatchActivationClick(target.get());
  }
}

void EventDispatcher::EmulateTouch(const std::string& event_type, int x, int y,
                                   const std::string& button, float delta_x,
                                   float delta_y, int modifiers,
                                   int click_count) {
  (void)button;
  (void)delta_x;
  (void)delta_y;
  (void)modifiers;
  (void)click_count;
  if (ui_owner_->Destroyed()) {
    return;
  }
  time_stamp_ = CurrentTimestampMilliseconds();
  NodeManager::Instance().SetEventDispatcher(this);
  from_overlay_ = false;
  if (auto* root = ui_owner_->Root()) {
    root_target_ = root->weak_from_this();
  }

  EmulatedTouchPoint point = CreateEmulatedTouchPoint(x, y);
  if (event_type == "mousePressed") {
    ResetClickEnv();
    DeactivatePseudoStatus(PseudoStatus::kAll);
    active_target_finger_map_.clear();
    primary_pointer_id_.reset();
    pointer_tool_types_.clear();
    primary_pointer_ids_by_tool_.clear();
    first_active_target_.reset();
    retained_text_event_targets_.clear();
    has_touch_moved_ = false;
    InitTouchEnv(point);
    if (EventThrough()) {
      ResetTouchEnv(point);
      first_active_target_.reset();
      return;
    }
    if (!first_active_target_.expired()) {
      if (enable_multi_touch_) {
        auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
        AddTargetTouchMap(target_touch_map, point);
        DispatchMultiTouchEvent(TouchEvent::START, target_touch_map);
      }
      OnTouchDown(point);
    }
  } else if (event_type == "mouseMoved") {
    if (first_active_target_.expired() || active_target_finger_map_.empty() ||
        EventThrough()) {
      return;
    }
    OnTouchMove(point);
    if (has_touch_moved_) {
      if (enable_multi_touch_) {
        auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
        AddTargetTouchMap(target_touch_map, point);
        DispatchMultiTouchEvent(TouchEvent::MOVE, target_touch_map);
      } else {
        DispatchSingleTouchEvent(TouchEvent::MOVE, point);
      }
    }
  } else if (event_type == "mouseReleased") {
    if (first_active_target_.expired() || active_target_finger_map_.empty()) {
      return;
    }
    if (EventThrough()) {
      ResetClickEnv();
      DeactivatePseudoStatus(PseudoStatus::kAll);
      ResetTouchEnv(point);
      first_active_target_.reset();
      return;
    }
    if (enable_multi_touch_) {
      auto target_touch_map = lepus::Value(lepus::Dictionary::Create());
      AddTargetTouchMap(target_touch_map, point);
      DispatchMultiTouchEvent(TouchEvent::UP, target_touch_map);
    }
    OnTouchUp(point);
    ResetTouchEnv(point);
  }
}

void EventDispatcher::DispatchSingleTouchEvent(
    const std::string& name, const ArkUI_UIInputEvent* event) {
  if (first_active_target_.expired()) {
    return;
  }

  auto active_target = first_active_target_.lock().get();
  TouchEvent touch_event(active_target->Sign(), name);
  auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  touch_event.SetTimeStamp(time_stamp);
  float scaled_density =
      (name == TouchEvent::TAP || name == TouchEvent::LONGPRESS)
          ? ui_owner_->Context()->ScaledDensity()
          : 1;
  float page_point[2] = {0.f};
  GetEventPagePoint(page_point, event, 0, scaled_density);
  float target_point[2] = {page_point[0], page_point[1]};
  GetTargetPoint(active_target, target_point, page_point);
  float client_point[2] = {
      OH_ArkUI_PointerEvent_GetWindowXByIndex(event, 0) / scaled_density,
      OH_ArkUI_PointerEvent_GetWindowYByIndex(event, 0) / scaled_density};
  touch_event.SetTargetPoint(target_point);
  touch_event.SetPagePoint(page_point);
  touch_event.SetClientPoint(client_point);
  touch_event.SetTimeStamp(OH_ArkUI_UIInputEvent_GetEventTime(event));
  touch_event.SetTarget(first_active_target_);
  MarkDispatchInCurrentLynxPageOnly(touch_event);
  ui_owner_->SendEvent(touch_event);
  last_touch_event_ = std::make_shared<TouchEvent>(touch_event);
}

void EventDispatcher::DispatchSingleTouchEvent(
    const std::string& name, const EmulatedTouchPoint& point) {
  if (first_active_target_.expired()) {
    return;
  }

  auto active_target = first_active_target_.lock().get();
  TouchEvent touch_event(active_target->Sign(), name);
  touch_event.SetTimeStamp(time_stamp_);
  float page_point[2] = {point.page_point[0], point.page_point[1]};
  float target_point[2] = {page_point[0], page_point[1]};
  GetTargetPoint(active_target, target_point, page_point);
  float client_point[2] = {point.client_point[0], point.client_point[1]};
  touch_event.SetTargetPoint(target_point);
  touch_event.SetPagePoint(page_point);
  touch_event.SetClientPoint(client_point);
  MarkDispatchInCurrentLynxPageOnly(touch_event);
  ui_owner_->SendEvent(touch_event);
  last_touch_event_ = std::make_shared<TouchEvent>(touch_event);
}

void EventDispatcher::DispatchMultiTouchEvent(
    const std::string& name, const lepus::Value& target_touch_map,
    const ArkUI_UIInputEvent* event) {
  (void)event;
  DispatchMultiTouchEvent(name, target_touch_map);
}

void EventDispatcher::DispatchMultiTouchEvent(
    const std::string& name, const lepus::Value& target_touch_map) {
  TouchEvent touch_event(name, target_touch_map);
  touch_event.SetTimeStamp(time_stamp_);
  touch_event.SetTarget(first_active_target_);
  MarkDispatchInCurrentLynxPageOnly(touch_event);
  ui_owner_->SendEvent(touch_event);
  last_touch_event_ = std::make_shared<TouchEvent>(touch_event);
}

void EventDispatcher::OnLongPressEvent(const ArkUI_UIInputEvent* event) {
  if (first_active_target_.expired()) {
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::LONGPRESS, event);
  DispatchLongPressEventToChildLynxPage(event);
}

void EventDispatcher::OnTapEvent(const ArkUI_UIInputEvent* event) {
  bool can_respond_tap = !first_active_target_.expired()
                             ? CanRespondTap(first_active_target_.lock().get())
                             : false;
  if (first_active_target_.expired() || first_touch_moved_ ||
      !can_respond_tap) {
    ShowMessageOnConsole("EventDispatcher: tap failed due to [target] " +
                             std::to_string(first_active_target_.expired()) +
                             ", [move] " + std::to_string(first_touch_moved_) +
                             ", [gesture] " + std::to_string(!can_respond_tap),
                         runtime::CONSOLE_LOG_WARNING);
    LOGI("EventDispatcher OnTapEvent tap failed: "
         << first_active_target_.expired() << ", " << first_touch_moved_ << ", "
         << can_respond_tap)
    return;
  }
  ShowMessageOnConsole("EventDispatcher: fire tap for target " +
                           std::to_string(first_active_target_.lock()->Sign()),
                       runtime::CONSOLE_LOG_INFO);
  DispatchSingleTouchEvent(TouchEvent::TAP, event);
  DispatchTapEventToChildLynxPage(event);
}

void EventDispatcher::OnClickEvent(const ArkUI_UIInputEvent* event) {
  if (click_target_chain_.empty()) {
    return;
  }
  auto first_click_target = click_target_chain_.front();
  bool can_respond_tap = !first_click_target.expired()
                             ? CanRespondTap(first_click_target.lock().get())
                             : false;
  if (first_click_target.expired() || first_touch_outside_ ||
      !can_respond_tap) {
    LOGI("EventDispatcher OnClickEvent click failed: "
         << first_click_target.expired() << ", " << first_touch_outside_ << ", "
         << can_respond_tap);
    return;
  }
  DispatchSingleTouchEvent(TouchEvent::CLICK, event);
  DispatchClickEventToChildLynxPage(event);
}

bool EventDispatcher::EventThrough() {
  auto target = first_active_target_.lock();
  if (!target) {
    return false;
  }
  return target->EventThrough(first_finger_down_point_);
}

bool EventDispatcher::ShouldInterceptGesture() {
  if (first_active_target_.expired()) {
    return false;
  }
  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->IsInterceptGesture()) {
      return true;
    }
    target = target->ParentTarget();
  }
  return false;
}

bool EventDispatcher::ContainGestureNode() {
  if (first_active_target_.expired()) {
    return false;
  }
  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    // When greater than 0, the corresponding node is bound to a gesture handler
    if (target->GestureArenaMemberId() > 0) {
      return true;
    }
    target = target->ParentTarget();
  }
  return false;
}

bool EventDispatcher::ShouldBlockNativeEvent() {
  if (first_active_target_.expired()) {
    return false;
  }

  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->BlockNativeEvent(first_finger_down_point_)) {
      return true;
    }
    target = target->ParentTarget();
  }
  return false;
}

ConsumeSlideDirection EventDispatcher::ShouldConsumeSlideEvent() {
  if (first_active_target_.expired()) {
    return ConsumeSlideDirection::kNone;
  }

  auto target = first_active_target_.lock().get();
  while (target != nullptr && target->ParentTarget() != target) {
    if (target->ConsumeSlideEvent() != ConsumeSlideDirection::kNone) {
      // TODO(hexionghui): Should collect all consume-slide-event.
      return target->ConsumeSlideEvent();
    }
    target = target->ParentTarget();
  }
  return ConsumeSlideDirection::kNone;
}

void EventDispatcher::UpdateRootTarget(UIBase* root) {
  if (root) {
    root_target_ = root->weak_from_this();
    fallback_hit_test_root_ = root->weak_from_this();
    if (root->IsOverlayContent() || active_overlay_hit_test_roots_.empty()) {
      hit_test_root_ = root->weak_from_this();
    } else {
      RestoreHitTestRoot();
    }
  }
}

bool EventDispatcher::CanConsumeTouchEvent(float point[2]) {
  auto root = hit_test_root_.lock();
  if (!root) {
    RestoreHitTestRoot();
    root = hit_test_root_.lock();
  }
  return CanConsumeTouchEventAtRoot(point, root.get());
}

bool EventDispatcher::CanConsumeTouchEventAtRoot(float point[2], UIBase* root) {
  if (ui_owner_->Destroyed()) {
    return false;
  }

  auto retained_root =
      root ? root->weak_from_this().lock() : root_target_.lock();
  root = retained_root.get();
  if (!root || !root->RootNode()) {
    return false;
  }
  ArkUI_IntOffset page_offset;
  OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->RootNode(),
                                                      &page_offset);
  float node_point_x = point[0], node_point_y = point[1];
  float scaled_density = ui_owner_->Context()->ScaledDensity();
  float page_x = page_offset.x / scaled_density;
  float page_y = page_offset.y / scaled_density;

  if (base::FloatsLarger(page_x, node_point_x) ||
      base::FloatsLarger(page_y, node_point_y) ||
      base::FloatsLarger(node_point_x, page_x + root->width_) ||
      base::FloatsLarger(node_point_y, page_y + root->height_)) {
    UpdateOverlayPassThroughState(root, false);
    return false;
  }

  point[0] = node_point_x - page_x;
  point[1] = node_point_y - page_y;

  EventTarget* active_target = root->HitTest(point);
  if (active_target == nullptr) {
    UpdateOverlayPassThroughState(root, false);
    return false;
  }
  float target_point[2] = {point[0], point[1]};
  UIBase* target_ui =
      active_target->HasUI()
          ? static_cast<UIBase*>(active_target)
          : static_cast<UIBase*>(active_target->FirstUITarget());
  LynxUIHelper::ConvertPointFromAncestorToDescendant(target_point, root,
                                                     target_ui, point);
  bool can_consume = !active_target->EventThrough(target_point);
  UpdateOverlayPassThroughState(root, can_consume);
  return can_consume;
}

void EventDispatcher::ActivateOverlayHitTestRoot(UIBase* root, int32_t level) {
  active_overlay_hit_test_roots_.erase(
      std::remove_if(active_overlay_hit_test_roots_.begin(),
                     active_overlay_hit_test_roots_.end(),
                     [root](const ActiveOverlayHitTestRoot& candidate) {
                       auto retained = candidate.root.lock();
                       return !retained || retained.get() == root;
                     }),
      active_overlay_hit_test_roots_.end());
  active_overlay_hit_test_roots_.push_back(
      {root->weak_from_this(), level, ++overlay_activation_order_, false});
  RestoreHitTestRoot();
}

void EventDispatcher::DeactivateOverlayHitTestRoot(UIBase* root) {
  active_overlay_hit_test_roots_.erase(
      std::remove_if(active_overlay_hit_test_roots_.begin(),
                     active_overlay_hit_test_roots_.end(),
                     [root](const ActiveOverlayHitTestRoot& candidate) {
                       auto retained = candidate.root.lock();
                       return !retained || retained.get() == root;
                     }),
      active_overlay_hit_test_roots_.end());
  if (hit_test_root_.lock().get() == root) {
    RestoreHitTestRoot();
  }
}

void EventDispatcher::UpdateOverlayPassThroughState(UIBase* root,
                                                    bool can_consume) {
  if (!root || !root->IsOverlayContent()) {
    return;
  }
  for (auto& candidate : active_overlay_hit_test_roots_) {
    auto retained = candidate.root.lock();
    if (retained.get() != root || candidate.pass_through == !can_consume) {
      continue;
    }
    candidate.pass_through = !can_consume;
    LOGI("EventDispatcher overlay pass-through changed: sign="
         << (root->Parent() ? root->Parent()->Sign() : -1)
         << ", pass_through=" << candidate.pass_through)
    return;
  }
}

void EventDispatcher::RestoreHitTestRoot() {
  active_overlay_hit_test_roots_.erase(
      std::remove_if(active_overlay_hit_test_roots_.begin(),
                     active_overlay_hit_test_roots_.end(),
                     [](const ActiveOverlayHitTestRoot& candidate) {
                       return candidate.root.expired();
                     }),
      active_overlay_hit_test_roots_.end());
  auto top =
      std::max_element(active_overlay_hit_test_roots_.begin(),
                       active_overlay_hit_test_roots_.end(),
                       [](const ActiveOverlayHitTestRoot& lhs,
                          const ActiveOverlayHitTestRoot& rhs) {
                         return lhs.level < rhs.level ||
                                (lhs.level == rhs.level &&
                                 lhs.activation_order < rhs.activation_order);
                       });
  if (top != active_overlay_hit_test_roots_.end()) {
    hit_test_root_ = top->root;
    return;
  }
  if (auto fallback = fallback_hit_test_root_.lock()) {
    hit_test_root_ = fallback;
    return;
  }
  auto* root = ui_owner_->Root();
  hit_test_root_ = root ? root->weak_from_this() : std::weak_ptr<UIBase>();
}

void EventDispatcher::UpdateNativeInteractionEnabledForTree(UIBase* root) {
  if (!root) {
    return;
  }

  TraverseAndUpdateHitTestBehavior(root, false);
}

void EventDispatcher::TraverseAndUpdateHitTestBehavior(
    UIBase* node, bool has_disabled_ancestor) {
  if (!node || !node->Node()) {
    return;
  }

  bool current_native_interaction_enabled = node->NativeInteractionEnabled();
  bool should_disable =
      has_disabled_ancestor || !current_native_interaction_enabled;

  if (should_disable) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        node->DrawNode(), NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(ARKUI_HIT_TEST_MODE_NONE));
  } else {
    NodeManager::Instance().SetAttributeWithNumberValue(
        node->DrawNode(), NODE_HIT_TEST_BEHAVIOR,
        static_cast<int32_t>(ARKUI_HIT_TEST_MODE_DEFAULT));
  }

  for (UIBase* child : node->Children()) {
    TraverseAndUpdateHitTestBehavior(child, should_disable);
  }
}

bool EventDispatcher::IsHighlightTouchEnabled() const {
  bool lynx_debug_enabled =
      tasm::DevToolLifecycle::GetInstance().IsEnabled() ||
      LynxEnv::GetInstance().GetBoolEnv(LynxEnv::kLynxDebugEnabled, false);
  return lynx_debug_enabled && LynxEnv::GetInstance().GetBoolEnv(
                                   LynxEnv::kLynxEnableHighlightTouch, false);
}

void EventDispatcher::InspectHitTarget(EventTarget* active_target) {
  if (!IsHighlightTouchEnabled() || !active_target || !ui_owner_ ||
      !ui_owner_->Context()) {
    return;
  }
  auto* context = ui_owner_->Context();
  if (auto pre_target = pre_target_.lock();
      pre_target && pre_target_inline_css_text_) {
    context->InvokeCDPFromSDK(
        BuildSetAttributesAsTextMessage(pre_target->Sign(),
                                        *pre_target_inline_css_text_,
                                        NextRequestId(cdp_request_id_)),
        [](const std::string&) {});
    pre_target_.reset();
    pre_target_inline_css_text_.reset();
  }

  auto active_target_weak = active_target->WeakTarget();
  int active_sign = active_target->Sign();
  uint64_t sequence =
      inspect_hit_target_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
  std::weak_ptr<WeakFlag> weak_flag = weak_flag_;
  auto ui_task_runner = context->GetUITaskRunner();
  context->InvokeCDPFromSDK(
      BuildGetInlineStylesForNodeMessage(active_sign,
                                         NextRequestId(cdp_request_id_)),
      [weak_flag, ui_task_runner, active_target_weak, active_sign,
       sequence](const std::string& result) {
        if (!ui_task_runner) {
          return;
        }
        ui_task_runner->PostTask([weak_flag, active_target_weak, active_sign,
                                  result, sequence]() {
          auto flag = weak_flag.lock();
          auto* dispatcher =
              flag ? flag->dispatcher.load(std::memory_order_acquire) : nullptr;
          if (!dispatcher) {
            return;
          }
          dispatcher->ApplyHitTargetStyle(active_target_weak, active_sign,
                                          result, sequence);
        });
      });
}

void EventDispatcher::ApplyHitTargetStyle(
    std::weak_ptr<EventTarget> active_target, int active_sign,
    const std::string& inline_style_response, uint64_t sequence) {
  if (sequence !=
          inspect_hit_target_sequence_.load(std::memory_order_relaxed) ||
      !ui_owner_ || !ui_owner_->Context()) {
    return;
  }
  auto css_text = ExtractInlineCSSText(inline_style_response);
  if (!css_text) {
    return;
  }
  pre_target_inline_css_text_ = *css_text;
  pre_target_ = std::move(active_target);
  std::string highlight_css_text =
      AppendHitTargetStyle(*pre_target_inline_css_text_);
  ui_owner_->Context()->InvokeCDPFromSDK(
      BuildSetAttributesAsTextMessage(active_sign, highlight_css_text,
                                      NextRequestId(cdp_request_id_)),
      [](const std::string&) {});
}

void EventDispatcher::ShowMessageOnConsole(const std::string& message,
                                           int32_t level) const {
  if (!IsHighlightTouchEnabled() || !ui_owner_ || !ui_owner_->Context()) {
    return;
  }
  ui_owner_->Context()->ShowMessageOnConsole(message, level);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
