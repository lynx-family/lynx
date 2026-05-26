// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_STYLE_STYLE_RULE_H_
#define CORE_RENDERER_CSS_NG_STYLE_STYLE_RULE_H_

#include <limits.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {

namespace tasm {
class CSSParseToken;
}

namespace css {

class StyleRule : public fml::RefCountedThreadSafeStorage {
 public:
  StyleRule(std::unique_ptr<LynxCSSSelector[]> selector_array,
            fml::RefPtr<tasm::CSSParseToken> token)
      : selector_array_(std::move(selector_array)), token_(std::move(token)) {}

  StyleRule(std::unique_ptr<LynxCSSSelector[]> selector_array,
            fml::RefPtr<tasm::CSSParseToken> token, uint32_t position)
      : selector_array_(std::move(selector_array)),
        token_(std::move(token)),
        position_(position) {}

  void ReleaseSelf() const override { delete this; }

  unsigned IndexOfNextSelectorAfter(size_t index) const {
    const LynxCSSSelector& current = SelectorAt(index);
    const LynxCSSSelector* next = LynxCSSSelectorList::Next(current);
    if (!next) return UINT_MAX;
    return SelectorIndex(*next);
  }

  const LynxCSSSelector* FirstSelector() const { return selector_array_.get(); }
  const LynxCSSSelector& SelectorAt(size_t index) const {
    return selector_array_[index];
  }
  unsigned SelectorIndex(const LynxCSSSelector& selector) const {
    return static_cast<unsigned>(&selector - FirstSelector());
  }

  const auto& Token() { return token_; }

  uint32_t Position() const { return position_; }

 private:
  std::unique_ptr<LynxCSSSelector[]> selector_array_;
  fml::RefPtr<tasm::CSSParseToken> token_;
  uint32_t position_ = 0;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_STYLE_STYLE_RULE_H_
