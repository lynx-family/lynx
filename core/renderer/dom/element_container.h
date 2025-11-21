// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_
#define CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/geometry/point.h"
#include "base/include/vector.h"
#include "core/renderer/dom/base_element_container.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

class ElementContainer : public BaseElementContainer {
 public:
  explicit ElementContainer(Element* element);
  virtual ~ElementContainer();

  ElementContainer* element_container_parent() {
    return static_cast<ElementContainer*>(parent());
  }

  const auto& children() const { return children_; }

  /**
   * Add element container to correct parent(if layout_only contained)
   * @param child the child to be added
   * @param ref the ref node ,which the child will be inserted before(currently
   * only for fiber)
   */
  void InsertElementContainerAccordingToElement(
      Element* child, Element* ref = nullptr) override;
  void RemoveElementContainerAccordingToElement(Element* child,
                                                bool destroy) override;
  void Destroy() override;

  void UpdateLayout(float left, float top,
                    bool transition_view = false) override;
  void UpdateLayoutWithoutChange() override;

  void TransitionToNativeView(fml::RefPtr<PropBundle> prop_bundle) override;
  void StyleChanged() override;
  void UpdateZIndexList() override;

  void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdatePaintingNode(
      bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;

 protected:
  void ReInsertChildForLayoutOnlyTransition(Element* child, int& index);

  bool IsStackingContextNode();

  void ZIndexChanged();
  void PositionFixedChanged();

  void AttachChildToTargetContainerRecursive(ElementContainer* parent,
                                             Element* child, int& index);

  virtual void AddChild(ElementContainer* child, int index);
  void RemoveSelf(bool destroy);
  void RemoveChild(ElementContainer* child);
  void InsertSelf();
  void RemoveFromParent(bool is_move);

  // below helper functions to calculate the correct parent and UI index for
  // fiber element
  static std::pair<ElementContainer*, int> FindParentAndIndexForChildForFiber(
      Element* parent, Element* child, Element* ref);
  static int GetUIIndexForChildForFiber(Element* parent, Element* child);
  static int GetUIChildrenCountForFiber(Element* parent);
  static void MoveZChildrenRecursively(Element* element,
                                       ElementContainer* parent);

  std::pair<ElementContainer*, int> FindParentForChild(Element* child);
  void MoveContainers(ElementContainer* old_parent,
                      ElementContainer* new_parent);
  int ZIndex() const;
  void SetNeedUpdate(bool update) { need_update_ = update; }

  bool IsSticky();

  // children with zIndex<0, negative zIndex child will be re-inserted to the
  // beginning after onPatchFinish
  base::Vector<ElementContainer*> negative_z_children_;
  base::Vector<ElementContainer*> children_;

  float last_left_{0};
  float last_top_{0};
  // the children size does not contain layout only nodes
  int32_t none_layout_only_children_size_{0};

  bool need_update_{true};

  // indicate the ElementContainer has finished first layout
  bool is_layouted_{false};
  // true if the Element's props has changed during this patch
  bool props_changed_{true};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_
