// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_
#define CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_

#include <memory>
#include <utility>

#include "base/include/geometry/point.h"
#include "base/include/vector.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

class Element;
class ElementManager;

struct FixedContainer {
  struct FixedContainer* parent{nullptr};
  struct FixedContainer* next{nullptr};
  struct FixedContainer* pre{nullptr};
};

class ElementContainer {
 public:
  explicit ElementContainer(Element* element);
  ~ElementContainer();

  Element* element() const { return element_; }
  ElementContainer* parent() const { return parent_; }
  const auto& children() const { return children_; }
  inline bool HasZChild() { return has_z_child_; }
  PaintingContext* painting_context();
  int id() const;

  void AddChild(ElementContainer* child, int index);
  void RemoveFromParent(bool is_move);
  void Destroy();
  void RemoveSelf(bool destroy);
  void InsertSelf();
  void UpdateLayout(float left, float top, bool transition_view = false);
  void UpdateLayoutWithoutChange();
  /**
   * Add element container to correct parent(if layout_only contained)
   * @param child the child to be added
   * @param ref the ref node ,which the child will be inserted before(currently
   * only for fiber)
   */
  void AttachChildToTargetContainer(Element* child, Element* ref = nullptr);
  void ReInsertChildForLayoutOnlyTransition(Element* child, int& index);
  void TransitionToNativeView(std::shared_ptr<PropBundle> prop_bundle);
  void StyleChanged();
  void UpdateZIndexList();
  ElementContainer* EnclosingStackingContextNode();
  bool IsStackingContextNode();

 private:
  void ZIndexChanged();
  void PositionFixedChanged();
  // Use RemoveFromParent/Destroy
  void RemoveChild(ElementContainer* child);
  // below helper functions to calculate the correct parent and UI index for
  // fiber element
  static std::pair<ElementContainer*, int> FindParentAndIndexForChildForFiber(
      Element* parent, Element* child, Element* ref);
  static int GetUIIndexForChildForFiber(Element* parent, Element* child);
  static int GetUIChildrenCountForFiber(Element* parent);
  static void MoveZChildrenRecursively(Element* element,
                                       ElementContainer* parent);

  ElementManager* element_manager();
  float last_left_{0};
  float last_top_{0};
  std::pair<ElementContainer*, int> FindParentForChild(Element* child);
  void MoveContainers(ElementContainer* old_parent,
                      ElementContainer* new_parent);
  int ZIndex() const;
  void SetNeedUpdate(bool update) { need_update_ = update; }
  void MarkDirty();
  bool IsSticky();
  Element* element_ = nullptr;
  ElementContainer* parent_ = nullptr;

  base::InlineVector<ElementContainer*, kChildrenInlineVectorSize> children_;
  // the children size does not contain layout only nodes
  int none_layout_only_children_size_{0};

  bool was_stacking_context_ = false;
  bool was_position_fixed_ = false;
  int old_index_ = 0;
  bool need_update_ = true;
  bool dirty_ = false;
  bool has_z_child_{false};
  // children with zIndex<0, negative zIndex child will be re-inserted to the
  // beginning after onPatchFinish
  base::InlineVector<ElementContainer*, 2> negative_z_children_;
  // indicate the ElementContainer has finished first layout
  bool is_layouted_{false};
  // true if the Element's props has changed during this patch
  bool props_changed_{true};
  std::unique_ptr<FixedContainer> fixed_node_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_
