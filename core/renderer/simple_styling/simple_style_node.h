// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_
#define CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"

namespace lynx::style {
class StyleObject;
class DynamicStyleObject;
struct StyleObjectArrayDeleter;
using StyleObjectRef = fml::RefPtr<StyleObject>;
using DynamicStyleObjectRef = fml::RefPtr<DynamicStyleObject>;

class SimpleStyleNode {
 public:
  SimpleStyleNode() = default;
  virtual ~SimpleStyleNode() = default;
  virtual void SetStyleObjects(
      std::unique_ptr<StyleObject*, StyleObjectArrayDeleter> style_object) = 0;
  // Apply a resolved delta and run the normal single tail.
  virtual void UpdateSimpleStyles(const tasm::StyleMap& style_map) = 0;
  // Commit and apply resolved static styles.
  virtual void UpdateSimpleStyles(tasm::StyleMap&& style_map) = 0;
  // Commit resolved static and dynamic styles, then apply the final result.
  virtual void UpdateStaticAndDynamicSimpleStyles(
      tasm::StyleMap&& style_map, tasm::StyleMap&& dynamic_style_map) = 0;
  // Commit resolved dynamic styles, then apply the final result.
  virtual void UpdateDynamicSimpleStyles(tasm::StyleMap&& style_map) = 0;
  // Restore to a specific resolved value without triggering the final tail.
  virtual void ResetSimpleStyle(tasm::CSSPropertyID id,
                                const tasm::CSSValue& value) = 0;
  // Restore to the default value without triggering the final tail.
  virtual void ResetSimpleStyle(tasm::CSSPropertyID id) = 0;
};
}  // namespace lynx::style
#endif  // CORE_RENDERER_SIMPLE_STYLING_SIMPLE_STYLE_NODE_H_
