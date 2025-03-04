// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_style_sheet_manager.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_fragment.h"

namespace lynx {
namespace tasm {
// app.ttss
static uint32_t sBasicCSSId = 0;

SharedCSSFragment* CSSStyleSheetManager::GetCSSStyleSheetForComponent(
    int32_t id) {
  // Actually this function can be fully replaced by
  // GetCSSStyleSheet(id). And component_fragments_ can be deleted.
  // No need to import self.
  return GetCSSStyleSheet(id);
}

SharedCSSFragment* CSSStyleSheetManager::GetCSSStyleSheetForPage(int32_t id) {
  if (enable_css_lazy_import_) {
    return GetCSSStyleSheet(id);
  } else {
    if (enable_new_import_rule_) {
      return GetCSSStyleSheet(id);
    }
    auto it = page_fragments_.find(id);
    if (it != page_fragments_.end() && it->second->is_baked()) {
      return it->second.get();
    }
    auto fragment = std::make_unique<SharedCSSFragment>(id, this);
    fragment->ImportOtherFragment(GetCSSStyleSheet(sBasicCSSId));
    if (id > 0) {
      fragment->ImportOtherFragment(GetCSSStyleSheet(id));
    }
    fragment->MarkBaked();
    auto ptr = fragment.get();
    page_fragments_[id] = std::move(fragment);
    return ptr;
  }
}

SharedCSSFragment* CSSStyleSheetManager::GetCSSStyleSheet(int32_t id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSStyleSheetManager::GetCSSStyleSheet");
  SharedCSSFragment* fragment = GetSharedCSSFragmentById(id);
  if (fragment == nullptr) {
    if (delegate_ && delegate_->DecodeCSSFragmentById(id)) {
      fragment = GetSharedCSSFragmentById(id);
    } else {
      return nullptr;
    }
  }
  if (fragment == nullptr || fragment->is_baked()) {
    return fragment;
  }
  FlatDependentCSS(fragment);
  return fragment;
}

void CSSStyleSheetManager::FlatDependentCSS(SharedCSSFragment* fragment) {
  const auto& dependents = fragment->dependent_ids();
  if (fragment->enable_css_selector()) {
    std::for_each(dependents.begin(), dependents.end(), [&](int32_t id) {
      fragment->ImportOtherFragment(GetCSSStyleSheet(id));
    });
  } else {
    // FIXME(linxs:) Retaining the logic below to avoid breaking changes,
    // although it is incorrect...
    std::for_each(dependents.rbegin(), dependents.rend(), [&](int32_t id) {
      fragment->ImportOtherFragment(GetCSSStyleSheet(id));
    });
  }
  fragment->MarkBaked();
}

void CSSStyleSheetManager::FlattenAllCSSFragment() {
  std::for_each(raw_fragments_->begin(), raw_fragments_->end(),
                [this](const auto& fragment) {
                  this->FlatDependentCSS(fragment.second.get());
                });
}

void CSSStyleSheetManager::CopyFrom(const CSSStyleSheetManager& other) {
  raw_fragments_ = other.raw_fragments_;
  enable_new_import_rule_ = other.enable_new_import_rule_;
}

const std::shared_ptr<CSSStyleSheetManager::CSSFragmentMap>&
CSSStyleSheetManager::GetCSSFragmentMap() const {
  return raw_fragments_;
}

}  // namespace tasm
}  // namespace lynx
