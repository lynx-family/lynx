// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_STYLE_NODE_H_
#define CORE_RENDERER_CSS_STYLE_NODE_H_

#include <string>

#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {
class CSSFragment;
}

namespace css {

class StyleNode {
 public:
  StyleNode() = default;
  virtual ~StyleNode() = default;

  virtual void OnStyleChange() = 0;

  virtual const base::String& tag() const = 0;

  virtual const base::String& idSelector() const = 0;

  virtual tasm::PseudoState GetPseudoState() const = 0;

  virtual bool HasPseudoState(tasm::PseudoState type) const = 0;

  virtual void OnPseudoStateChanged(tasm::PseudoState, tasm::PseudoState) = 0;

  virtual const tasm::ClassList& classes() const = 0;

  virtual StyleNode* SelectorMatchingParent() const = 0;

  virtual StyleNode* HolderParent() const = 0;

  virtual StyleNode* NextSibling() const = 0;

  virtual StyleNode* PreviousSibling() const = 0;

  virtual StyleNode* PseudoElementOwner() const = 0;

  virtual tasm::CSSFragment* ParentStyleSheet() const = 0;

  virtual bool GetRemoveCSSScopeEnabled() const = 0;
  virtual bool GetCascadePseudoEnabled() const = 0;
  virtual bool GetRemoveDescendantSelectorScope() const = 0;

  virtual bool IsComponent() const = 0;

  virtual bool ContainsIdSelector(const std::string& selector) const = 0;

  virtual bool ContainsClassSelector(const std::string& selector) const = 0;

  virtual bool ContainsTagSelector(const std::string& selector) const = 0;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_STYLE_NODE_H_
