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
  virtual ~ElementContainer();

  Element* element() const { return element_; }
  ElementContainer* parent() const { return parent_; }
  const auto& children() const { return children_; }
  inline bool HasZChild() { return has_z_child_; }
  PaintingContext* painting_context();
  int id() const;

  /**
   * Add element container to correct parent(if layout_only contained)
   * @param child the child to be added
   * @param ref the ref node ,which the child will be inserted before(currently
   * only for fiber)
   */
  virtual void InsertElementContainerAccordingToElement(Element* child,
                                                        Element* ref = nullptr);
  virtual void RemoveElementContainerAccordingToElement(Element* child,
                                                        bool destroy);
  virtual void Destroy();

  void UpdateLayout(float left, float top, bool transition_view = false);
  void UpdateLayoutWithoutChange();

  void TransitionToNativeView(fml::RefPtr<PropBundle> prop_bundle);
  void StyleChanged();
  void UpdateZIndexList();

  virtual void CreatePaintingNode(bool is_flatten,
                                  const fml::RefPtr<PropBundle>& painting_data);
  virtual void UpdatePaintingNode(bool tend_to_flatten,
                                  const fml::RefPtr<PropBundle>& painting_data);
  virtual void UpdatePlatformExtraBundle(PlatformExtraBundle* bundle);
  virtual bool CheckFlatten(base::MoveOnlyClosure<bool, bool> func);

  virtual void SetKeyframes(fml::RefPtr<PropBundle> bundle);
  virtual void SetFrameAppBundle(
      const std::shared_ptr<LynxTemplateBundle>& bundle);

  virtual void ListCellWillAppear(const std::string& item_key);
  virtual void ListCellDisappear(bool is_exist, const base::String& item_key);
  virtual void ListReusePaintingNode(int32_t child_id,
                                     const std::string& item_key);
  virtual void InsertListItemPaintingNode(int32_t child_id);
  virtual void RemoveListItemPaintingNode(int32_t child_id);

  virtual std::vector<float> ScrollBy(float width, float height);
  virtual std::vector<float> GetRectToLynxView();
  virtual void UpdateScrollInfo(float estimated_offset, bool smooth,
                                bool scrolling);
  virtual void Invoke(
      const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>&
          callback);
  virtual void UpdateContentOffsetForListContainer(float content_size,
                                                   float delta_x, float delta_y,
                                                   bool is_init_scroll_offset,
                                                   bool from_layout);

  virtual void SetGestureDetectorState(int32_t gesture_id, int32_t state);
  virtual void ConsumeGesture(int32_t gesture_id, const lepus::Value& params);

  virtual void OnNodeReady();
  virtual void OnNodeReload();
  virtual void UpdateLayoutPatching();
  virtual void UpdateNodeReadyPatching();
  virtual void Flush();
  virtual void FlushImmediately();

  virtual void OnFirstScreen();
  virtual void AppendOptionsForTiming(
      const std::shared_ptr<PipelineOptions>& options);
  virtual void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options);
  virtual void MarkLayoutUIOperationQueueFlushStartIfNeed();

 protected:
  void ReInsertChildForLayoutOnlyTransition(Element* child, int& index);

  ElementContainer* EnclosingStackingContextNode();

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

  ElementManager* element_manager();
  std::pair<ElementContainer*, int> FindParentForChild(Element* child);
  void MoveContainers(ElementContainer* old_parent,
                      ElementContainer* new_parent);
  int ZIndex() const;
  void SetNeedUpdate(bool update) { need_update_ = update; }
  void MarkDirty();
  bool IsSticky();

  // children with zIndex<0, negative zIndex child will be re-inserted to the
  // beginning after onPatchFinish
  base::Vector<ElementContainer*> negative_z_children_;
  base::Vector<ElementContainer*> children_;
  std::unique_ptr<FixedContainer> fixed_node_;
  Element* element_{nullptr};
  ElementManager* manager_{nullptr};

  ElementContainer* parent_{nullptr};
  float last_left_{0};
  float last_top_{0};
  // the children size does not contain layout only nodes
  int32_t none_layout_only_children_size_{0};
  int32_t old_index_{0};
  bool was_stacking_context_{false};
  bool was_position_fixed_{false};
  bool need_update_{true};
  bool dirty_{false};
  bool has_z_child_{false};
  // indicate the ElementContainer has finished first layout
  bool is_layouted_{false};
  // true if the Element's props has changed during this patch
  bool props_changed_{true};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_CONTAINER_H_
