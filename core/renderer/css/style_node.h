// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_STYLE_NODE_H_
#define CORE_RENDERER_CSS_STYLE_NODE_H_

#include <string>
#include <vector>

#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace css {

class StyleNode {
 public:
  StyleNode() {}
  virtual ~StyleNode() {}

  virtual void OnStyleChange() = 0;

  virtual const base::String& tag() const = 0;

  virtual const base::String& idSelector() const = 0;

  virtual tasm::PseudoState GetPseudoState() const = 0;

  virtual bool HasPseudoState(tasm::PseudoState type) const = 0;

  virtual const tasm::ClassList& classes() const = 0;

  virtual css::StyleNode* SelectorMatchingParent() const = 0;

  virtual css::StyleNode* HolderParent() const = 0;

  virtual css::StyleNode* NextSibling() const = 0;

  virtual css::StyleNode* PreviousSibling() const = 0;

  virtual css::StyleNode* PseudoElementOwner() const = 0;

  virtual bool ContainsIdSelector(const std::string& selector) const = 0;

  virtual bool ContainsClassSelector(const std::string& selector) const = 0;

  virtual bool ContainsTagSelector(const std::string& selector) const = 0;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_STYLE_NODE_H_
