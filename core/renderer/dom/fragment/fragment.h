// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_

#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace starlight {
class ComputedCSSStyle;
}
namespace tasm {

using starlight::LayoutResultForRendering;

// Combines layout results and rendering styles to generate display content
// via DisplayList. Owned by an element; lifetime must not exceed that element.
class Fragment : public fml::RefCountedThreadSafeStorage {
 public:
  Fragment(const starlight::ComputedCSSStyle& style, int sign,
           PaintingContext& painting_context, base::String tag);

  void CreateLayerIfNeeded();

  void UpdateLayout(LayoutResultForRendering layout_result_for_rendering);

 private:
  virtual ~Fragment() = default;

  const base::String& Tag() const { return tag_; };

  base::MoveOnlyClosure<bool> should_create_layer_;

  // TODO(zhongyr): children management methods.
  base::InlineVector<fml::RefPtr<Fragment>, kChildrenInlineVectorSize>
      children_;
  // Sign is used to identify the fragment in the tree. Same to the sign in
  // element.
  const int sign_;

  LayoutResultForRendering layout_result_for_rendering_;
  PaintingContext& painting_context_;

  // XXX(zhongyr): Do we need a refCounted style? Fragment's lifetime should not
  // exceed the element who owns it.
  const starlight::ComputedCSSStyle& style_;
  const base::String tag_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_FRAGMENT_H_
