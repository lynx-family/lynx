// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

#include <ace/xcomponent/native_interface_xcomponent.h>

#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
namespace lynx {
namespace tasm {
namespace harmony {

NodeManager::NodeManager() {
  OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_NODE, ArkUI_NativeNodeAPI_1,
                              native_node_api_);
  OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_GESTURE, ArkUI_NativeGestureAPI_1,
                              native_gesture_api_);
}

ArkUI_NodeHandle NodeManager::CreateNode(ArkUI_NodeType type) {
  return native_node_api_->createNode(type);
}

bool NodeManager::SetAttribute(ArkUI_NodeHandle node,
                               ArkUI_NodeAttributeType type,
                               ArkUI_AttributeItem* item) {
  return native_node_api_->setAttribute(node, type, item) == 0;
}

const ArkUI_AttributeItem* NodeManager::GetAttribute(
    ArkUI_NodeHandle node, ArkUI_NodeAttributeType type) {
  return native_node_api_->getAttribute(node, type);
}

bool NodeManager::ResetAttribute(ArkUI_NodeHandle node,
                                 ArkUI_NodeAttributeType type) {
  return native_node_api_->resetAttribute(node, type) == 0;
}

void NodeManager::DisposeNode(ArkUI_NodeHandle node) {
  return native_node_api_->disposeNode(node);
}

bool NodeManager::InsertNode(ArkUI_NodeHandle parent, ArkUI_NodeHandle child,
                             int index) {
  return native_node_api_->insertChildAt(parent, child, index) == 0;
}

ArkUI_NodeHandle NodeManager::GetParent(ArkUI_NodeHandle node) {
  return native_node_api_->getParent(node);
}

bool NodeManager::InsertNodeAfter(ArkUI_NodeHandle parent,
                                  ArkUI_NodeHandle child,
                                  ArkUI_NodeHandle after) {
  return native_node_api_->insertChildAfter(parent, child, after) == 0;
}

bool NodeManager::RemoveNode(ArkUI_NodeHandle parent, ArkUI_NodeHandle child) {
  return native_node_api_->removeChild(parent, child) == 0;
}

void NodeManager::RequestLayout(ArkUI_NodeHandle node) {
  native_node_api_->markDirty(node, NODE_NEED_LAYOUT);
}

void NodeManager::Invalidate(ArkUI_NodeHandle node) {
  native_node_api_->markDirty(node, NODE_NEED_RENDER);
}

bool NodeManager::RegisterNodeEvent(ArkUI_NodeHandle node,
                                    ArkUI_NodeEventType type, uint32_t id,
                                    void* data) {
  return native_node_api_->registerNodeEvent(node, type, id, data) == 0;
}

bool NodeManager::RegisterNodeCustomEvent(ArkUI_NodeHandle node,
                                          ArkUI_NodeCustomEventType type,
                                          uint32_t id, void* data) {
  return native_node_api_->registerNodeCustomEvent(node, type, id, data) == 0;
}

void NodeManager::UnregisterNodeEvent(ArkUI_NodeHandle node,
                                      ArkUI_NodeEventType type) {
  native_node_api_->unregisterNodeEvent(node, type);
}

void NodeManager::UnregisterNodeCustomEvent(ArkUI_NodeHandle node,
                                            ArkUI_NodeCustomEventType type) {
  native_node_api_->unregisterNodeCustomEvent(node, type);
}

void NodeManager::SetMeasuredSize(ArkUI_NodeHandle node, int32_t width,
                                  int32_t height) {
  native_node_api_->setMeasuredSize(node, width, height);
}

void NodeManager::SetLayoutPosition(ArkUI_NodeHandle node, int32_t x,
                                    int32_t y) {
  native_node_api_->setLayoutPosition(node, x, y);
}

ArkUI_IntSize NodeManager::GetMeasuredSize(ArkUI_NodeHandle node) {
  return native_node_api_->getMeasuredSize(node);
}

int32_t NodeManager::MeasureNode(ArkUI_NodeHandle node,
                                 ArkUI_LayoutConstraint* constraint) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, NODE_MANAGER_MEASURE_NODE);
  return native_node_api_->measureNode(node, constraint);
}

int32_t NodeManager::LayoutNode(ArkUI_NodeHandle node, int32_t x, int32_t y) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, NODE_MANAGER_LAYOUT_NODE);
  return native_node_api_->layoutNode(node, x, y);
}

void NodeManager::AddNodeEventReceiver(
    ArkUI_NodeHandle node, void (*eventReceiver)(ArkUI_NodeEvent* event)) {
  native_node_api_->addNodeEventReceiver(node, eventReceiver);
}

void NodeManager::AddNodeCustomEventReceiver(
    ArkUI_NodeHandle node,
    void (*eventReceiver)(ArkUI_NodeCustomEvent* event)) {
  native_node_api_->addNodeCustomEventReceiver(node, eventReceiver);
}

void NodeManager::RemoveNodeEventReceiver(
    ArkUI_NodeHandle node, void (*eventReceiver)(ArkUI_NodeEvent* event)) {
  if (native_node_api_) {
    native_node_api_->removeNodeEventReceiver(node, eventReceiver);
  }
}

void NodeManager::RemoveNodeCustomEventReceiver(
    ArkUI_NodeHandle node,
    void (*eventReceiver)(ArkUI_NodeCustomEvent* event)) {
  if (native_node_api_) {
    native_node_api_->removeNodeCustomEventReceiver(node, eventReceiver);
  }
}

ArkUI_GestureRecognizer* NodeManager::CreateLongPressGesture(
    int32_t fingers_num, bool repeat_result, int32_t duration_num) {
  return native_gesture_api_->createLongPressGesture(fingers_num, repeat_result,
                                                     duration_num);
}

ArkUI_GestureRecognizer* NodeManager::createTapGestureWithDistanceThreshold(
    int32_t count_num, int32_t fingers_num, double distance_threshold) {
  return native_gesture_api_->createTapGestureWithDistanceThreshold(
      count_num, fingers_num, distance_threshold);
}

ArkUI_GestureRecognizer* NodeManager::CreatePanGesture(
    int32_t fingers_num, ArkUI_GestureDirectionMask directions,
    double distance_num) {
  return native_gesture_api_->createPanGesture(fingers_num, directions,
                                               distance_num);
}

void NodeManager::SetGestureEventTarget(
    ArkUI_GestureRecognizer* recognizer,
    ArkUI_GestureEventActionTypeMask action_type_mask, void* extra_params,
    void (*target_receiver)(ArkUI_GestureEvent* event, void* extra_params)) {
  native_gesture_api_->setGestureEventTarget(recognizer, action_type_mask,
                                             extra_params, target_receiver);
}

void NodeManager::SetGestureInterrupterToNode(
    ArkUI_NodeHandle node, ArkUI_GestureInterruptResult (*interrupter)(
                               ArkUI_GestureInterruptInfo* info)) {
  native_gesture_api_->setGestureInterrupterToNode(node, interrupter);
}

void NodeManager::AddGestureToNode(ArkUI_NodeHandle node,
                                   ArkUI_GestureRecognizer* recognizer,
                                   ArkUI_GesturePriority mode,
                                   ArkUI_GestureMask mask) {
  native_gesture_api_->addGestureToNode(node, recognizer, mode, mask);
}

void NodeManager::DisposeGesture(ArkUI_GestureRecognizer* recognizer) {
  native_gesture_api_->dispose(recognizer);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
