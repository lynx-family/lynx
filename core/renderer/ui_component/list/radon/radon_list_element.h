// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_RADON_RADON_LIST_ELEMENT_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_RADON_RADON_LIST_ELEMENT_H_

#include <memory>

#include "core/renderer/dom/vdom/radon/radon_element.h"
#include "core/renderer/ui_component/list/list_container_delegate_internal.h"

namespace lynx {
namespace tasm {

// List element in radon diff.
class RadonListElement : public RadonElement {
 public:
  RadonListElement(const base::String& tag,
                   const fml::RefPtr<AttributeHolder>& node,
                   ElementManager* element_manager, uint32_t node_index,
                   bool enable_decoupled_list);

  void TickElement(fml::TimePoint& time) override;
  bool ResolveStyleValue(CSSPropertyID id,
                         const tasm::CSSValue& value) override;
  bool OnAttributeSet(const base::String& key,
                      const lepus::Value& value) override;
  void OnListElementUpdated(
      const std::shared_ptr<PipelineOptions>& options) override;
  void OnComponentFinished(
      Element* component,
      const std::shared_ptr<PipelineOptions>& option) override;
  void OnListItemLayoutUpdated(Element* component) override;
  void ScrollByListContainer(float content_offset_x, float content_offset_y,
                             float original_x, float original_y) override;
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override;
  void ScrollStopped() override;
  void SetEventHandler(const base::String& name,
                       EventHandler* handler) override;
  void ResetEventHandlers() override;
  bool DisableListPlatformImplementation() const override { return true; }
  void PropsUpdateFinish() override;

 private:
  bool enable_decoupled_list_{false};
  std::unique_ptr<ListContainerDelegateInternal>
      list_container_delegate_internal_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_RADON_RADON_LIST_ELEMENT_H_
