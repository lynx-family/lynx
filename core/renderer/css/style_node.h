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
namespace css {

class StyleNode {
 public:
  StyleNode() = default;
  virtual ~StyleNode() = default;

  const base::String& tag() const { return tag_; };

  const base::String& idSelector() const { return id_selector_; }

  tasm::PseudoState GetPseudoState() const { return pseudo_state_; }

  bool HasPseudoState(tasm::PseudoState type) const {
    return pseudo_state_ & type;
  }

  const tasm::ClassList& classes() const { return classes_; }

  virtual StyleNode* SelectorMatchingParent() const = 0;

  virtual StyleNode* HolderParent() const = 0;

  virtual StyleNode* NextSibling() const = 0;

  virtual StyleNode* PreviousSibling() const = 0;

  virtual StyleNode* PseudoElementOwner() const = 0;

  bool ContainsIdSelector(const std::string& selector) const;
  bool ContainsClassSelector(const std::string& selector) const;
  bool ContainsTagSelector(const std::string& selector) const;

 protected:
  base::String tag_;

  // Should be unique in component
  base::String id_selector_;

  tasm::ClassList classes_;

  tasm::PseudoState pseudo_state_{tasm::kPseudoStateNone};
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_STYLE_NODE_H_
