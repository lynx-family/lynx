// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

#include <memory>

#include "core/renderer/dom/element_container.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/starlight/types/layout_result.h"

namespace lynx {
namespace starlight {
class ComputedCSSStyle;
}
namespace tasm {

using starlight::LayoutResultForRendering;

// Combines layout results and rendering styles to generate display content
// via DisplayList. Owned by an element; lifetime must not exceed that element.
class Fragment : public ElementContainer {
 public:
  explicit Fragment(Element* element);

  ~Fragment() override = default;

  void InsertElementContainerAccordingToElement(Element* child,
                                                Element* ref) override;

  void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) override;
  void UpdatePaintingNode(
      bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override;

  void CreateLayerIfNeeded();
  void HandleAttributes(const fml::RefPtr<PropBundle>& painting_data) const;

  void UpdateLayout(LayoutResultForRendering layout_result_for_rendering);

  void SetBehavior(std::unique_ptr<FragmentBehavior> behavior);

  void Draw();

  void Draw(DisplayListBuilder& display_list_builder);

  void OnDraw(DisplayListBuilder& display_list_builder);

  void MarkNeedRedraw();

  void AddChildBefore(Fragment* child, Fragment* sibling);

 private:
  bool has_platform_renderer_;
  bool need_redraw_{false};

  base::MoveOnlyClosure<bool> should_create_layer_;

  // TODO(zhongyr): children management methods.
  base::InlineVector<Fragment*, kChildrenInlineVectorSize> children_;

  LayoutResultForRendering layout_result_for_rendering_;

  std::unique_ptr<FragmentBehavior> behavior_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
