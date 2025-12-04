// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_
#define CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_

#include <memory>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/css_property.h"

namespace lynx::style {
class StyleObject;
struct StyleObjectArrayDeleter;
using StyleObjectRef = fml::RefPtr<StyleObject>;

class SimpleStyleNode {
 public:
  SimpleStyleNode() = default;
  virtual ~SimpleStyleNode() = default;
  virtual void SetStyleObjects(
      std::unique_ptr<StyleObject*, StyleObjectArrayDeleter> style_object) = 0;
  virtual void UpdateSimpleStyles(const tasm::StyleMap& style_map) = 0;
  virtual void ResetSimpleStyle(tasm::CSSPropertyID id) = 0;
};
}  // namespace lynx::style
#endif  // CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_
