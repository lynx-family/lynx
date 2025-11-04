// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

#include <memory>

#include "core/renderer/dom/element_container.h"
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

  void CreatePaintingNode(
      bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) override;

  void CreateLayerIfNeeded() const;
  void HandleAttributes(const fml::RefPtr<PropBundle>& painting_data) const;

  void UpdateLayout(LayoutResultForRendering layout_result_for_rendering);

  void SetBehavior(std::unique_ptr<FragmentBehavior> behavior);

 private:
  const base::String& Tag() const { return tag_; };

  base::MoveOnlyClosure<bool> should_create_layer_;

  // TODO(zhongyr): children management methods.
  base::InlineVector<Fragment*, kChildrenInlineVectorSize> children_;
  // Sign is used to identify the fragment in the tree. Same to the sign in
  // element.
  const int sign_;

  LayoutResultForRendering layout_result_for_rendering_;

  // XXX(zhongyr): Do we need a refCounted style? Fragment's lifetime should not
  // exceed the element who owns it.
  // TODO(songshourui.null): remove maybe_unused later.
  [[maybe_unused]] const starlight::ComputedCSSStyle* style_;
  const base::String tag_;
  std::unique_ptr<FragmentBehavior> behavior_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
