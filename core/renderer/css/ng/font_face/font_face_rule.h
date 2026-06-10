// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_FONT_FACE_FONT_FACE_RULE_H_
#define CORE_RENDERER_CSS_NG_FONT_FACE_FONT_FACE_RULE_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {

namespace lepus {
class Value;
}  // namespace lepus
using lepus_value = lepus::Value;

namespace css {

struct FontSource {
  bool is_local = false;
  std::string uri;
};

struct UnicodeRange {
  uint32_t from;
  uint32_t to;
};

struct VariationAxis {
  std::string tag;
  float value;
};

enum class FontFaceStyleKind : uint8_t {
  kNormal = 0,
  kItalic = 1,
  kOblique = 2,
};

struct FontFaceStyle {
  FontFaceStyleKind kind = FontFaceStyleKind::kNormal;
  int16_t oblique_angle_min = 0;
  int16_t oblique_angle_max = 0;
};

class FontFaceRule : public fml::RefCountedThreadSafeStorage {
 public:
  FontFaceRule(std::string family, std::vector<FontSource> sources,
               uint16_t weight_min, uint16_t weight_max, uint16_t stretch_min,
               uint16_t stretch_max, FontFaceStyle style,
               std::vector<VariationAxis> variation_settings,
               std::vector<UnicodeRange> unicode_range)
      : family_(std::move(family)),
        sources_(std::move(sources)),
        weight_min_(weight_min),
        weight_max_(weight_max),
        stretch_min_(stretch_min),
        stretch_max_(stretch_max),
        style_(style),
        variation_settings_(std::move(variation_settings)),
        unicode_range_(std::move(unicode_range)) {}

  ~FontFaceRule() override = default;
  void ReleaseSelf() const override { delete this; }

  const std::string& Family() const { return family_; }
  const std::vector<FontSource>& Sources() const { return sources_; }
  uint16_t WeightMin() const { return weight_min_; }
  uint16_t WeightMax() const { return weight_max_; }
  uint16_t StretchMin() const { return stretch_min_; }
  uint16_t StretchMax() const { return stretch_max_; }
  const FontFaceStyle& Style() const { return style_; }
  FontFaceStyleKind StyleKind() const { return style_.kind; }
  int16_t ObliqueAngleMin() const { return style_.oblique_angle_min; }
  int16_t ObliqueAngleMax() const { return style_.oblique_angle_max; }
  const std::vector<VariationAxis>& VariationSettings() const {
    return variation_settings_;
  }
  const std::vector<UnicodeRange>& GetUnicodeRange() const {
    return unicode_range_;
  }

  lepus_value ToLepus() const;
  static fml::RefPtr<const FontFaceRule> FromLepus(const lepus_value& value);

 private:
  std::string family_;
  std::vector<FontSource> sources_;
  uint16_t weight_min_ = 400;
  uint16_t weight_max_ = 400;
  uint16_t stretch_min_ = 100;
  uint16_t stretch_max_ = 100;
  FontFaceStyle style_;
  std::vector<VariationAxis> variation_settings_;
  std::vector<UnicodeRange> unicode_range_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_FONT_FACE_FONT_FACE_RULE_H_
