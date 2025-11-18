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

class BaseElementContainer {
 public:
  explicit BaseElementContainer(Element* element);
  virtual ~BaseElementContainer();

  Element* element() const { return element_; }
  ElementManager* element_manager() const { return manager_; }
  PaintingContext* painting_context() const {
    return element_manager()->painting_context();
  }

  int id() const { return element()->impl_id(); }

  /**
   * Add element container to correct parent(if layout_only contained)
   * @param child the child to be added
   * @param ref the ref node ,which the child will be inserted before(currently
   * only for fiber)
   */
  virtual void InsertElementContainerAccordingToElement(
      Element* child, Element* ref = nullptr) = 0;
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
  Element* element_{nullptr};
  ElementManager* manager_{nullptr};

  BaseElementContainer* parent_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_BASE_ELEMENT_CONTAINER_H_
