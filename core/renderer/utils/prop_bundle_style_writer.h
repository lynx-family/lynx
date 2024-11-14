// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_H_
#define CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_H_

#include "core/public/prop_bundle.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_property_id.h"
#include "core/style/properties/border.h"

namespace lynx {
namespace tasm {
class PropBundleStyleWriter {
  using WriterFunc = void (*)(PropBundle*, CSSPropertyID,
                              starlight::ComputedCSSStyle*);

 public:
  static void PushStyleToBundle(PropBundle* bundle, CSSPropertyID id,
                                starlight::ComputedCSSStyle* style);

 private:
  static constexpr CSSPropertyID kPlatformIDs[] = {
#define DECLARE_STYLE_ID(name) tasm::CSSPropertyID::kPropertyID##name,
      FOREACH_PLATFORM_PROPERTY(DECLARE_STYLE_ID)};
#undef DECLARE_STYLE_ID

  static void DefaultWriterFunc(PropBundle* bundle, CSSPropertyID id,
                                starlight::ComputedCSSStyle* style);

#pragma region border longhand
  static void SetBorderColorBySide(PropBundle* bundle, CSSPropertyID id,
                                   starlight::ComputedCSSStyle* style,
                                   style::BorderSide side);
  template <style::BorderSide Side>
  static void SetBorderColor(PropBundle* bundle, CSSPropertyID id,
                             starlight::ComputedCSSStyle* style) {
    SetBorderColorBySide(bundle, id, style, Side);
  }
#pragma endregion  // border longhand

  static constexpr std::array<WriterFunc, kPropertyEnd> kWriter = [] {
    std::array<WriterFunc, kPropertyEnd> writer = {nullptr};
    for (CSSPropertyID id : kPlatformIDs) {
      writer[id] = &DefaultWriterFunc;
    }
    writer[kPropertyIDBorderTopColor] =
        &SetBorderColor<style::BorderSide::kTop>;
    writer[kPropertyIDBorderRightColor] =
        &SetBorderColor<style::BorderSide::kRight>;
    writer[kPropertyIDBorderBottomColor] =
        &SetBorderColor<style::BorderSide::kBottom>;
    writer[kPropertyIDBorderLeftColor] =
        &SetBorderColor<style::BorderSide::kLeft>;
    return writer;
  }();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_H_
