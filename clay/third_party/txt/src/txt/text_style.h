/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_TEXT_STYLE_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_TEXT_STYLE_H_

#include <string>
#include <vector>

#include "clay/gfx/rendering_backend.h"
#include "font_features.h"
#include "font_style.h"
#include "font_weight.h"
#include "text_baseline.h"
#include "text_decoration.h"
#include "text_shadow.h"
#if defined(CLAY_ENABLE_TTTEXT)
#include <textra/layout_definition.h>
#endif  // ENABLE_TTTEXT

namespace txt {
enum WordBreak : uint8_t {
  kNormal = 0,
  kBreakAll = 1,
  kKeepAll = 2,
};
class TextStyle {
 public:
  clay::Color color = clay::Color::kBlack();
  int decoration = TextDecoration::kNone;
  // Does not make sense to draw a transparent object, so we use it as a default
  // value to indicate no decoration color was set.
  clay::Color decoration_color = clay::Color::kTransparent();
  TextDecorationStyle decoration_style = TextDecorationStyle::kSolid;
  // Thickness is applied as a multiplier to the default thickness of the font.
  double decoration_thickness_multiplier = 1.0;
  FontWeight font_weight = FontWeight::w400;
  FontStyle font_style = FontStyle::normal;
  TextBaseline text_baseline = TextBaseline::kAlphabetic;
  float text_baseline_shift = 0.0;
  bool half_leading = true;
  // An ordered list of fonts in order of priority. The first font is more
  // highly preferred than the last font.
  std::vector<std::string> font_families;
  double font_size = 14.0;
  double letter_spacing = 0.0;
  double word_spacing = 0.0;
  double height = 1.0;
  double line_spacing = 0.0;
  bool has_height_override = false;
  std::string locale;
  bool has_background = false;
  clay::GrPaint background;
  bool has_foreground = false;
  clay::GrPaint foreground;
  bool has_foreground_id = false;
  int foreground_id;
  // An ordered list of shadows where the first shadow will be drawn first (at
  // the bottom).
  std::vector<TextShadow> text_shadows;
  FontFeatures font_features;
  FontVariations font_variations;
  WordBreak word_break = kNormal;
#if defined(CLAY_ENABLE_TTTEXT)
  ttoffice::tttext::CharacterVerticalAlignment align_type =
      ttoffice::tttext::CharacterVerticalAlignment::kBaseLine;
  bool enable_text_bounds_ = false;
#endif  // CLAY_ENABLE_TTTEXT

  TextStyle();

  bool equals(const TextStyle& other) const;
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_TEXT_STYLE_H_
