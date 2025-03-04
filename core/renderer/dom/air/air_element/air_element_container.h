// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_CONTAINER_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_CONTAINER_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/geometry/point.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace tasm {

class AirElement;
class ElementManager;

class AirElementContainer {
 public:
  explicit AirElementContainer(AirElement* air_element);
  ~AirElementContainer() = default;

  AirElementContainer(const AirElementContainer& node) = delete;
  AirElementContainer& operator=(const AirElementContainer& node) = delete;

  AirElement* air_element() const { return air_element_; }
  AirElementContainer* parent() const { return parent_; }
  const std::vector<AirElementContainer*>& children() const {
    return children_;
  }
  PaintingContext* painting_context();
  int id() const;

  void AddChild(AirElementContainer* child, int index);
  void RemoveFromParent(bool is_move);
  void Destroy();
  void RemoveSelf(bool destroy);
  void InsertSelf();
  void UpdateLayout(float left, float top, bool transition_view = false);
  void UpdateLayoutWithoutChange();
  void AttachChildToTargetContainer(AirElement* child);
  void SetPropsChanged(bool changed) { props_changed_ = changed; };

 private:
  // Use RemoveFromParent/Destroy
  void RemoveChild(AirElementContainer* child);
  ElementManager* element_manager();
  float last_left_{0};
  float last_top_{0};
  std::pair<AirElementContainer*, int> FindParentForChild(AirElement* child);
  AirElement* air_element_ = nullptr;
  AirElementContainer* parent_ = nullptr;

  std::vector<AirElementContainer*> children_;
  // indicate the AirElementContainer has finished first layout
  bool is_layouted_{false};
  // true if the AirElement's props has changed during this patch
  bool props_changed_{true};
  // the children size does not contain layout only nodes
  int none_layout_only_children_size_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_CONTAINER_H_
