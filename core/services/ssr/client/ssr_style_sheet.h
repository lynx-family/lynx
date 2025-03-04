// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_CLIENT_SSR_STYLE_SHEET_H_
#define CORE_SERVICES_SSR_CLIENT_SSR_STYLE_SHEET_H_

#include <string>
#include <utility>

#include "core/renderer/css/css_fragment_decorator.h"

namespace lynx {
namespace tasm {

class SsrStyleSheet : public CSSFragmentDecorator {
 public:
  SsrStyleSheet() : CSSFragmentDecorator(nullptr) {}
  ~SsrStyleSheet() {}

  void SetKeyFrames(CSSKeyframesTokenMap&& keyframes) {
    keyframes_ = std::move(keyframes);
  }

  void SetPseudoMap(CSSParserTokenMap&& css_parse_token) {
    ssr_pseudo_map_ = std::move(css_parse_token);
  }

  CSSParseToken* GetPseudoStyle(const std::string& key) override {
    auto it = ssr_pseudo_map_.find(key);
    if (it != ssr_pseudo_map_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

 private:
  CSSParserTokenMap ssr_pseudo_map_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_SERVICES_SSR_CLIENT_SSR_STYLE_SHEET_H_
