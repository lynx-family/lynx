// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_BASE_ELEMENT_CONTAINER_H_
#define CORE_RENDERER_DOM_BASE_ELEMENT_CONTAINER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

class Element;
class ElementManager;
class ElementContainer;
class Fragment;

class BaseElementContainer {
 public:
  enum DirtyState : uint8_t {
    kClean = 0,
    kNeedSortZChild = 1 << 0,
    kNeedSortFixedChild = 1 << 1,
    kNeedRedraw = 1 << 2,
    kNeedUpdateSubtreeProperty = 1 << 3
  };

  explicit BaseElementContainer(Element* element);
  virtual ~BaseElementContainer();

  virtual bool HasUIPrimitive() const = 0;

  void set_parent(BaseElementContainer* parent) { parent_ = parent; }
  BaseElementContainer* parent() const { return parent_; }

  Element* element() const;
  ElementManager* element_manager() const;
  PaintingContext* painting_context() const;
  int id() const;

  ElementContainer* CastToElementContainer();
  Fragment* CastToFragment();

  int32_t old_z_index() const { return old_z_index_; }
  void set_old_z_index(int32_t old_z_index) { old_z_index_ = old_z_index; }

  bool was_stacking_context() const { return was_stacking_context_; }
  void set_was_stacking_context(bool was_stacking_context) {
    was_stacking_context_ = was_stacking_context;
  }

  bool was_position_fixed() const { return was_position_fixed_; }
  void set_was_position_fixed(bool was_position_fixed) {
    was_position_fixed_ = was_position_fixed;
  }

  void MarkDirtyState(DirtyState state);

  // Marks this container as needing a full redraw, propagating to ancestors.
  void InvalidateForRedraw() {
    if (NeedRedraw()) {
      return;
    }
    MarkDirtyState(kNeedRedraw);
    if (parent()) {
      parent()->InvalidateForRedraw();
    }
  }

  // Marks this container as needing subtree property update (e.g., transform,
  // opacity). This is a lighter-weight invalidation than full redraw.
  void InvalidateForSubtreeProperty() {
    MarkDirtyState(kNeedUpdateSubtreeProperty);
  }

  /**
   * Add element container to correct parent(if layout_only contained)
   * @param child the child to be added
   * @param ref the ref node ,which the child will be inserted before(currently
   * only for fiber)
   */
  virtual void InsertElementContainerAccordingToElement(
      Element* child, Element* ref = nullptr) = 0;

  // Remove element container from correct parent(if layout_only contained)
  // @param child the child to be removed
  // @param destroy whether destroy the element container, only used for radon
  virtual void RemoveElementContainerAccordingToElement(Element* child,
                                                        bool destroy) = 0;
  virtual void Destroy() = 0;

  virtual void UpdateLayout(float left, float top,
                            bool transition_view = false) = 0;
  virtual void UpdateLayoutWithoutChange() = 0;

  virtual void TransitionToNativeView(fml::RefPtr<PropBundle> prop_bundle) = 0;
  virtual void StyleChanged() = 0;
  virtual void UpdateZIndexList() = 0;

  virtual void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) = 0;
  virtual void UpdatePaintingNode(
      bool tend_to_flatten, const fml::RefPtr<PropBundle>& painting_data) = 0;
  virtual void UpdatePlatformExtraBundle(PlatformExtraBundle* bundle);
  virtual bool CheckFlatten(base::MoveOnlyClosure<bool, bool> func);

  virtual void SetKeyframes(fml::RefPtr<PropBundle> bundle);
  virtual void SetFrameAppBundle(
      const std::shared_ptr<LynxTemplateBundle>& bundle);

  virtual void ListCellWillAppear(const std::string& item_key);
  virtual void ListCellDisappear(bool is_exist, const base::String& item_key);
  virtual void ListReusePaintingNode(int32_t child_id,
                                     const base::String& item_key);
  virtual void InsertListItemPaintingNode(int32_t child_id) = 0;
  virtual void RemoveListItemPaintingNode(int32_t child_id) = 0;
  virtual void UpdateContentOffsetForListContainer(float content_size,
                                                   float delta_x, float delta_y,
                                                   bool is_init_scroll_offset,
                                                   bool from_layout) = 0;

  virtual std::vector<float> ScrollBy(float width, float height);
  virtual std::vector<float> GetRectToLynxView();
  virtual void UpdateScrollInfo(float estimated_offset, bool smooth,
                                bool scrolling);
  virtual void Invoke(
      const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>&
          callback);

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
  void UpdateGlobalInsertionOrder();

 protected:
  virtual bool is_fragment() const { return false; }

  bool IsRootContainer() const;

  bool has_z_child() const { return has_z_child_; }
  void set_has_z_child(bool has_z_child) { has_z_child_ = has_z_child; }

  bool has_fixed_child() const { return has_fixed_child_; }
  void set_has_fixed_child(bool has_fixed_child) {
    has_fixed_child_ = has_fixed_child;
  }

  void ResetDirtyState(DirtyState state);
  bool NeedSortZChild() const {
    return dirty_state_ & DirtyState::kNeedSortZChild;
  }
  bool NeedSortFixedChild() const {
    return dirty_state_ & DirtyState::kNeedSortFixedChild;
  }
  bool NeedRedraw() const { return dirty_state_ & DirtyState::kNeedRedraw; }
  bool NeedUpdateSubtreeProperty() const {
    return dirty_state_ & DirtyState::kNeedUpdateSubtreeProperty;
  }

  void ClearPaintDirtyState() {
    dirty_state_ = static_cast<DirtyState>(
        dirty_state_ &
        (~(DirtyState::kNeedRedraw | DirtyState::kNeedUpdateSubtreeProperty)));
  }

  BaseElementContainer* EnclosingStackingContextNode();

  static Element const* FindCommonAncestor(Element const** left_mark,
                                           Element const** right_mark);
  static int CompareElementOrder(Element* left, Element* right);

  int32_t old_z_index_{0};
  bool was_stacking_context_{false};
  bool was_position_fixed_{false};
  bool is_root_{false};

  bool has_z_child_{false};
  bool has_fixed_child_{false};

  DirtyState dirty_state_{0};

  /**
   * A globally unique sequential identifier representing
   * the chronological position at which this element container  was
   * inserted into the parent element container relative to all other element
   * containers..
   */
  uint32_t global_insertion_order_{kInitialGlobalInsertionOrder};

 private:
  Element* element_{nullptr};
  ElementManager* manager_{nullptr};

  BaseElementContainer* parent_{nullptr};
  int32_t id_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_BASE_ELEMENT_CONTAINER_H_
