// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_STYLE_SHARED_CSS_FRAGMENT_WRAPPER_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_STYLE_SHARED_CSS_FRAGMENT_WRAPPER_H_

#include <memory>
#include <utility>

#include "base/include/value/ref_counted_class.h"
#include "core/renderer/css/shared_css_fragment.h"

namespace lynx {
namespace tasm {

class SharedCSSFragmentWrapper : public lepus::RefCounted {
 public:
  explicit SharedCSSFragmentWrapper(std::unique_ptr<SharedCSSFragment> fragment)
      : fragment_(std::move(fragment)) {}
  std::unique_ptr<SharedCSSFragment> fragment_;

  bool HasFontFacesResolved() const { return has_font_faces_resolved_; }
  void MarkFontFacesResolved(bool resolved) {
    has_font_faces_resolved_ = resolved;
  }

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kCSSFragment;
  }

 private:
  bool has_font_faces_resolved_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_STYLE_SHARED_CSS_FRAGMENT_WRAPPER_H_
