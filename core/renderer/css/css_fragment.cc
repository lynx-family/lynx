// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_fragment.h"

namespace lynx {
namespace tasm {

const CSSKeyframesTokenMap& CSSFragment::GetKeyframesRuleMap() {
  return keyframes_;
}

const CSSFontFaceRuleMap& CSSFragment::GetFontFaceRuleMap() {
  return fontfaces_;
}

CSSKeyframesToken* CSSFragment::GetKeyframesRule(const std::string& key) {
  auto it = keyframes_.find(key);
  if (it != keyframes_.end()) {
    return it->second.get();
  }
  return nullptr;
}

const std::vector<std::shared_ptr<CSSFontFaceRule>>&
CSSFragment::GetFontFaceRule(const std::string& key) {
  auto it = fontfaces_.find(key);
  if (it != fontfaces_.end()) {
    return it->second;
  }
  return GetDefaultFontFaceList();
}

const std::vector<std::shared_ptr<CSSFontFaceRule>>&
CSSFragment::GetDefaultFontFaceList() {
  static base::NoDestructor<std::vector<std::shared_ptr<CSSFontFaceRule>>>
      fontfaces{};
  return *fontfaces;
}

}  // namespace tasm
}  // namespace lynx
