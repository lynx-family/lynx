// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NODE_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NODE_MANAGER_H_

#include <arkui/native_dialog.h>
#include <arkui/native_gesture.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <node_api.h>
#include <window_manager/oh_display_info.h>
#include <window_manager/oh_display_manager.h>

#include <string>
#include <unordered_map>
#include <utility>

typedef ArkUI_GestureInterruptResult (*GestureInterrupter)(
    ArkUI_GestureInterruptInfo* info);
typedef void (*GestureReceiver)(ArkUI_GestureEvent* event, void* extraParams);

namespace lynx {
namespace tasm {
namespace harmony {
class EventDispatcher;
class NodeManager {
 public:
  static NodeManager& Instance() {
    static NodeManager instance;
    return instance;
  }

  static ArkUI_NativeDialogAPI_1* DialogInstance() {
    static ArkUI_NativeDialogAPI_1* dialog = GetDialog();
    return dialog;
  }

  static NativeDisplayManager_Orientation GetOrientation() {
    NativeDisplayManager_Orientation orientation = DISPLAY_MANAGER_UNKNOWN;
    OH_NativeDisplayManager_GetDefaultDisplayOrientation(&orientation);
    return orientation;
  }

  static int32_t GetScreenWidth() {
    int32_t width;
    OH_NativeDisplayManager_GetDefaultDisplayWidth(&width);
    return width;
  }

  static int32_t GetScreenHeight() {
    int32_t height;
    OH_NativeDisplayManager_GetDefaultDisplayHeight(&height);
    return height;
  }

  ArkUI_NodeHandle CreateNode(ArkUI_NodeType type);
  void DisposeNode(ArkUI_NodeHandle node);
  bool SetAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType type,
                    ArkUI_AttributeItem* item);
  const ArkUI_AttributeItem* GetAttribute(ArkUI_NodeHandle node,
                                          ArkUI_NodeAttributeType type);

  bool ResetAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType type);

  bool InsertNode(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int index);
  bool InsertNodeAfter(ArkUI_NodeHandle parent, ArkUI_NodeHandle child,
                       ArkUI_NodeHandle after);
  bool RemoveNode(ArkUI_NodeHandle parent, ArkUI_NodeHandle child);
  void RequestLayout(ArkUI_NodeHandle node);
  void Invalidate(ArkUI_NodeHandle node);
  ArkUI_NodeHandle GetParent(ArkUI_NodeHandle node);

  /**
   * Add event to the target node.
   * @param node Node instance the event binds to.
   * @param type Type of the event that you wants to receive.
   * @param id Customized id.
   * @param data Customized data. If you registered the node with {RegisterNode}
   * before, you must pass a pointer to {UIBase} instance to handle the event by
   * the default event receiver.
   * @return
   */
  bool RegisterNodeEvent(ArkUI_NodeHandle node, ArkUI_NodeEventType type,
                         uint32_t id, void* data);
  bool RegisterNodeCustomEvent(ArkUI_NodeHandle node,
                               ArkUI_NodeCustomEventType type, uint32_t id,
                               void* data);

  void UnregisterNodeEvent(ArkUI_NodeHandle node, ArkUI_NodeEventType type);
  void UnregisterNodeCustomEvent(ArkUI_NodeHandle node,
                                 ArkUI_NodeCustomEventType type);

  void AddNodeEventReceiver(ArkUI_NodeHandle node,
                            void (*eventReceiver)(ArkUI_NodeEvent* event));
  void AddNodeCustomEventReceiver(
      ArkUI_NodeHandle node,
      void (*eventReceiver)(ArkUI_NodeCustomEvent* event));
  void RemoveNodeEventReceiver(ArkUI_NodeHandle node,
                               void (*eventReceiver)(ArkUI_NodeEvent* event));
  void RemoveNodeCustomEventReceiver(
      ArkUI_NodeHandle node,
      void (*eventReceiver)(ArkUI_NodeCustomEvent* event));

  void SetMeasuredSize(ArkUI_NodeHandle node, int32_t width, int32_t height);
  void SetLayoutPosition(ArkUI_NodeHandle node, int32_t x, int32_t y);

  ArkUI_IntSize GetMeasuredSize(ArkUI_NodeHandle node);

  int32_t MeasureNode(ArkUI_NodeHandle node,
                      ArkUI_LayoutConstraint* Constraint);

  int32_t LayoutNode(ArkUI_NodeHandle node, int32_t x, int32_t y);

  ArkUI_GestureRecognizer* CreateLongPressGesture(int32_t fingers_num = 1,
                                                  bool repeat_result = false,
                                                  int32_t duration_num = 500);

  ArkUI_GestureRecognizer* createTapGestureWithDistanceThreshold(
      int32_t count_num = 1, int32_t fingers_num = 1,
      double distance_threshold = 5);

  ArkUI_GestureRecognizer* CreatePanGesture(
      int32_t fingers_num = 1,
      ArkUI_GestureDirectionMask directions = GESTURE_DIRECTION_ALL,
      double distance_num = 5);

  void SetGestureEventTarget(ArkUI_GestureRecognizer* recognizer,
                             ArkUI_GestureEventActionTypeMask action_type_mask,
                             void* extra_params,
                             void (*target_receiver)(ArkUI_GestureEvent* event,
                                                     void* extra_params));

  void SetGestureInterrupterToNode(ArkUI_NodeHandle node,
                                   ArkUI_GestureInterruptResult (*interrupter)(
                                       ArkUI_GestureInterruptInfo* info));

  void AddGestureToNode(ArkUI_NodeHandle node,
                        ArkUI_GestureRecognizer* recognizer,
                        ArkUI_GesturePriority mode, ArkUI_GestureMask mask);

  void DisposeGesture(ArkUI_GestureRecognizer* recognizer);

  void SetEventDispatcher(EventDispatcher* dispatcher) {
    event_dispatcher_ = dispatcher;
  }

  EventDispatcher* GetEventDispatcher() { return event_dispatcher_; }

  template <typename... Args>
  void SetAttributeWithNumberValue(ArkUI_NodeHandle node,
                                   ArkUI_NodeAttributeType type,
                                   Args&&... args) {
    static_assert(
        ((std::is_same_v<ArkUI_NumberValue, std::remove_reference_t<Args>> ||
          std::is_arithmetic_v<std::remove_reference_t<Args>>)&&...),
        "Arguments must be of type ArkUI_NumberValue or arithmetic types");
    ArkUI_AttributeItem item{};
    ArkUI_NumberValue value[] = {NumberValue(std::forward<Args>(args))...};
    item.value = value;
    item.size = sizeof...(args);
    Instance().SetAttribute(node, type, &item);
  }

  template <typename... Args>
  void GetAttributeValues(ArkUI_NodeHandle node, ArkUI_NodeAttributeType type,
                          Args*... args) {
    const auto* item = native_node_api_->getAttribute(node, type);
    auto* data = item->value;
    if (item->size < sizeof...(args)) {
      return;
    }
    size_t i = 0;
    ((*args = GetValue<Args>(&data[i++])), ...);
  }

  template <typename T>
  T GetAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType type,
                 size_t index = 0) {
    const auto* item = native_node_api_->getAttribute(node, type);
    if (!item) {
      return T{};
    }
    // Size of value is always 1 no matter what type it is. We can not use
    // item->size to prevent index out of bounds error here.
    return GetValue<T>(&(item->value[index]));
  }

  template <>
  std::string GetAttribute<std::string>(ArkUI_NodeHandle node,
                                        ArkUI_NodeAttributeType type,
                                        size_t index) {
    const auto* item = native_node_api_->getAttribute(node, type);
    if (!item || !item->string) {
      return std::string{};
    }
    return item->string;
  }

  template <>
  void GetAttributeValues<std::string>(ArkUI_NodeHandle node,
                                       ArkUI_NodeAttributeType type,
                                       std::string* dst) {
    *dst = native_node_api_->getAttribute(node, type)->string;
  }

 private:
  NodeManager();
  template <typename T>
  ArkUI_NumberValue NumberValue(T val) {
    if constexpr (std::is_arithmetic_v<T>) {
      return {.f32 = static_cast<float>(val)};
    } else {
      static_assert(
          std::is_same_v<T, ArkUI_NumberValue>,
          "Arguments must be of type ArkUI_NumberValue or arithmetic types");
      return val;
    }
  }

  template <>
  ArkUI_NumberValue NumberValue(int32_t val) {
    return {.i32 = val};
  }

  template <>
  ArkUI_NumberValue NumberValue(uint32_t val) {
    return {.u32 = val};
  }

  template <typename T>
  T GetValue(const ArkUI_NumberValue* number_value) {
    static_assert(std::is_same_v<T, int> || std::is_same_v<T, float> ||
                      std::is_same_v<T, double> ||
                      std::is_same_v<T, uint32_t> ||
                      std::is_same_v<T, std::string>,
                  "Only accept float, double, int and unsigned int here.");

    if constexpr (std::is_same_v<T, int>) {
      return number_value->i32;
    } else if constexpr (std::is_same_v<T, float> ||
                         std::is_same_v<T, double>) {
      return number_value->f32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      return number_value->u32;
    }
    return 0;
  }

  static ArkUI_NativeDialogAPI_1* GetDialog() {
    ArkUI_NativeDialogAPI_1* dialog = nullptr;
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_DIALOG, ArkUI_NativeDialogAPI_1,
                                dialog);
    return dialog;
  }

  ArkUI_NativeNodeAPI_1* native_node_api_{nullptr};
  ArkUI_NativeGestureAPI_1* native_gesture_api_{nullptr};
  EventDispatcher* event_dispatcher_{nullptr};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_NODE_MANAGER_H_
