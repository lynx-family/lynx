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

void CSSFragment::CollectIdChangedInvalidation(CSSFragment* style_sheet,
                                               css::InvalidationLists& lists,
                                               const std::string& old_id,
                                               const std::string& new_id) {
  // We know the style_sheet is not empty
  if (!old_id.empty()) style_sheet->CollectInvalidationSetsForId(lists, old_id);
  if (!new_id.empty()) style_sheet->CollectInvalidationSetsForId(lists, new_id);
}

void CSSFragment::CollectClassChangedInvalidation(
    CSSFragment* style_sheet, css::InvalidationLists& lists,
    const ClassList& old_classes, const ClassList& new_classes) {
  if (old_classes.empty()) {
    for (auto& class_name : new_classes) {
      style_sheet->CollectInvalidationSetsForClass(lists, class_name.str());
    }
  } else {
    base::InlineVector<bool, ClassList::kInlinedSize> remaining_class_bits(
        old_classes.size());
    for (auto& class_name : new_classes) {
      bool found = false;
      for (unsigned j = 0; j < old_classes.size(); ++j) {
        if (class_name == old_classes[j]) {
          // Mark each class that is still in the newClasses, so we can skip
          // doing a n^2 search below when looking for removals. We can't
          // break from this loop early since a class can appear more than
          // once.
          remaining_class_bits[j] = true;
          found = true;
        }
      }
      // Class was added.
      if (!found) {
        style_sheet->CollectInvalidationSetsForClass(lists, class_name.str());
      }
    }

    for (unsigned i = 0; i < old_classes.size(); ++i) {
      if (remaining_class_bits[i]) continue;
      // Class was removed.
      style_sheet->CollectInvalidationSetsForClass(lists, old_classes[i].str());
    }
  }
}

void CSSFragment::CollectPseudoChangedInvalidation(
    CSSFragment* style_sheet, css::InvalidationLists& lists, PseudoState prev,
    PseudoState curr) {
  if ((prev ^ curr) & kPseudoStateFocus) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoFocus);
  }
  if ((prev ^ curr) & kPseudoStateActive) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoActive);
  }
  if ((prev ^ curr) & kPseudoStateHover) {
    style_sheet->CollectInvalidationSetsForPseudoClass(
        lists, css::LynxCSSSelector::kPseudoHover);
  }
}

}  // namespace tasm
}  // namespace lynx
