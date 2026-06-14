// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/font_face_parser.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/parser/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"

namespace lynx {
namespace css {
namespace {

std::string ToString(const std::u16string& value) {
  return ustring_helper::to_string(value);
}

std::string ToLowerASCII(std::string_view value) {
  return base::StringToLowerASCII(value);
}

std::string ToLowerASCII(const CSSParserToken& token) {
  return ToLowerASCII(ToString(token.Value()));
}

std::string TrimString(const std::string& value) {
  return base::TrimString(value);
}

bool IsIdent(const CSSParserToken& token, const char* lowercase_ascii) {
  return token.GetType() == kIdentToken &&
         EqualIgnoringASCIICase(token.Value(),
                                ustring_helper::from_string(lowercase_ascii));
}

bool IsValidVariationTag(const std::string& tag) {
  if (tag.size() != 4) return false;
  for (char c : tag) {
    if (c < 0x20 || c > 0x7E) return false;
  }
  return true;
}

template <typename Consumer, typename... Args>
bool ParseDescriptorValue(const std::string& text, Consumer consumer,
                          Args&&... args) {
  CSSTokenizer tokenizer(text);
  std::vector<CSSParserToken> tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range(tokens);
  range.ConsumeWhitespace();
  if (range.AtEnd() || !consumer(range, std::forward<Args>(args)...)) {
    return false;
  }
  return range.AtEnd();
}

bool ConsumeCommaIncludingWhitespace(CSSParserTokenRange& range) {
  if (range.Peek().GetType() != kCommaToken) return false;
  range.Consume();
  range.ConsumeWhitespace();
  return true;
}

bool ConsumeFontFamilyName(CSSParserTokenRange& range, std::string* family) {
  if (range.AtEnd()) return false;

  if (range.Peek().GetType() == kStringToken) {
    *family = ToString(range.ConsumeIncludingWhitespace().Value());
    return !family->empty();
  }

  std::vector<std::string> parts;
  while (!range.AtEnd()) {
    if (range.Peek().GetType() != kIdentToken) return false;
    parts.emplace_back(ToString(range.ConsumeIncludingWhitespace().Value()));
  }
  *family = base::Join(parts, " ");
  return !family->empty();
}

bool ConsumeUrlValue(CSSParserTokenRange& range, std::string* uri) {
  if (range.AtEnd() || range.Peek().GetType() != kStringToken) return false;
  *uri = ToString(range.ConsumeIncludingWhitespace().Value());
  return !uri->empty();
}

bool ConsumeFontSourceHints(CSSParserTokenRange& range) {
  while (range.Peek().GetType() == kFunctionToken) {
    const std::string name = ToLowerASCII(range.Peek());
    if (name != "format" && name != "tech") return false;
    range.ConsumeBlock();
    range.ConsumeWhitespace();
  }
  return true;
}

bool ConsumeFontSource(CSSParserTokenRange& range, FontSource* source) {
  if (range.AtEnd()) return false;

  if (range.Peek().GetType() == kUrlToken) {
    source->is_local = false;
    source->uri = ToString(range.ConsumeIncludingWhitespace().Value());
  } else if (range.Peek().GetType() == kFunctionToken) {
    const std::string name = ToLowerASCII(range.Peek());

    CSSParserTokenRange args = range.ConsumeBlock();
    args.ConsumeWhitespace();

    if (name == "local") {
      source->is_local = true;
      if (!ConsumeFontFamilyName(args, &source->uri) || !args.AtEnd()) {
        return false;
      }
    } else if (name == "url") {
      source->is_local = false;
      if (!ConsumeUrlValue(args, &source->uri) || !args.AtEnd()) return false;
    } else {
      return false;
    }
  } else {
    return false;
  }

  if (source->uri.empty()) return false;
  range.ConsumeWhitespace();
  return ConsumeFontSourceHints(range);
}

bool ConsumeFontSourceList(CSSParserTokenRange& range,
                           std::vector<FontSource>* sources) {
  std::vector<FontSource> parsed;
  do {
    FontSource source;
    if (!ConsumeFontSource(range, &source)) return false;
    parsed.push_back(std::move(source));
  } while (ConsumeCommaIncludingWhitespace(range));

  if (parsed.empty()) return false;
  *sources = std::move(parsed);
  return true;
}

bool ParseFontFamilyDescriptor(const std::string& text, std::string* family) {
  std::string parsed;
  if (!ParseDescriptorValue(text, ConsumeFontFamilyName, &parsed)) {
    return false;
  }
  *family = std::move(parsed);
  return true;
}

bool ParseSrcDescriptor(const std::string& text,
                        std::vector<FontSource>* sources) {
  std::vector<FontSource> parsed;
  if (!ParseDescriptorValue(text, ConsumeFontSourceList, &parsed)) {
    return false;
  }
  *sources = std::move(parsed);
  return true;
}

using NumericRangeEndpointParser = bool (*)(const CSSParserToken&, uint16_t*);

bool ConsumeNumericRange(CSSParserTokenRange& range, uint16_t* min,
                         uint16_t* max,
                         NumericRangeEndpointParser parse_endpoint) {
  uint16_t first = 0;
  if (!parse_endpoint(range.Peek(), &first)) return false;
  range.ConsumeIncludingWhitespace();

  if (range.AtEnd()) {
    *min = *max = first;
    return true;
  }

  uint16_t second = 0;
  if (!parse_endpoint(range.Peek(), &second)) return false;
  range.ConsumeIncludingWhitespace();

  if (first > second) {
    std::swap(first, second);
  }
  *min = first;
  *max = second;
  return true;
}

bool ParseIntegerWeightToken(const CSSParserToken& token, uint16_t* out) {
  if (token.GetType() != kNumberToken ||
      token.GetNumericValueType() != kIntegerValueType) {
    return false;
  }
  double value = token.NumericValue();
  if (!std::isfinite(value) || value < 1 || value > 1000) return false;
  *out = static_cast<uint16_t>(value);
  return true;
}

bool ParseWeightRangeEndpoint(const CSSParserToken& token, uint16_t* out) {
  if (IsIdent(token, "normal")) {
    *out = 400;
    return true;
  }
  if (IsIdent(token, "bold")) {
    *out = 700;
    return true;
  }
  return ParseIntegerWeightToken(token, out);
}

bool ConsumeWeightRange(CSSParserTokenRange& range, uint16_t* min,
                        uint16_t* max) {
  return ConsumeNumericRange(range, min, max, ParseWeightRangeEndpoint);
}

bool ParseWeightDescriptor(const std::string& text, uint16_t* min,
                           uint16_t* max) {
  uint16_t parsed_min = 0;
  uint16_t parsed_max = 0;
  if (!ParseDescriptorValue(text, ConsumeWeightRange, &parsed_min,
                            &parsed_max)) {
    return false;
  }
  *min = parsed_min;
  *max = parsed_max;
  return true;
}

bool ParseStretchKeywordToken(const CSSParserToken& token, uint16_t* value) {
  if (token.GetType() != kIdentToken) return false;
  const std::string keyword = ToLowerASCII(token);
  if (keyword == "ultra-condensed") {
    *value = 50;
  } else if (keyword == "extra-condensed") {
    *value = 63;
  } else if (keyword == "condensed") {
    *value = 75;
  } else if (keyword == "semi-condensed") {
    *value = 88;
  } else if (keyword == "normal") {
    *value = 100;
  } else if (keyword == "semi-expanded") {
    *value = 113;
  } else if (keyword == "expanded") {
    *value = 125;
  } else if (keyword == "extra-expanded") {
    *value = 150;
  } else if (keyword == "ultra-expanded") {
    *value = 200;
  } else {
    return false;
  }
  return true;
}

bool ParseStretchPercentageToken(const CSSParserToken& token, uint16_t* value) {
  if (token.GetType() != kPercentageToken) return false;
  double numeric = token.NumericValue();
  if (!std::isfinite(numeric) || numeric <= 0 || numeric > 1000) return false;
  double rounded = std::round(numeric);
  if (rounded < 1) return false;
  *value = static_cast<uint16_t>(rounded);
  return true;
}

bool ParseStretchRangeEndpoint(const CSSParserToken& token, uint16_t* value) {
  return ParseStretchKeywordToken(token, value) ||
         ParseStretchPercentageToken(token, value);
}

bool ConsumeStretchRange(CSSParserTokenRange& range, uint16_t* min,
                         uint16_t* max) {
  return ConsumeNumericRange(range, min, max, ParseStretchRangeEndpoint);
}

bool ParseStretchDescriptor(const std::string& text, uint16_t* min,
                            uint16_t* max) {
  uint16_t parsed_min = 0;
  uint16_t parsed_max = 0;
  if (!ParseDescriptorValue(text, ConsumeStretchRange, &parsed_min,
                            &parsed_max)) {
    return false;
  }
  *min = parsed_min;
  *max = parsed_max;
  return true;
}

// Oblique angles stored as degrees × 100 for fixed-point precision.
constexpr double kPi = 3.14159265358979323846;
constexpr int16_t kDefaultObliqueAngle = 1400;  // 14.00°
constexpr int16_t kMinObliqueAngle = -9000;     // -90.00°
constexpr int16_t kMaxObliqueAngle = 9000;      // 90.00°

bool ConsumeAngle(CSSParserTokenRange& range, int16_t* oblique_angle) {
  const CSSParserToken& token = range.Peek();
  if (token.GetType() != kDimensionToken ||
      !std::isfinite(token.NumericValue())) {
    return false;
  }

  const std::string unit = ToLowerASCII(token);
  double degrees = token.NumericValue();
  if (unit == "deg") {
    // Use numeric value directly.
  } else if (unit == "grad") {
    degrees *= 360.0 / 400.0;
  } else if (unit == "rad") {
    degrees *= 180.0 / kPi;
  } else if (unit == "turn") {
    degrees *= 360.0;
  } else {
    return false;
  }

  // Oblique angles stored as degrees × 100 for fixed-point precision.
  double scaled = std::round(degrees * 100.0);
  if (!std::isfinite(scaled) || scaled < kMinObliqueAngle ||
      scaled > kMaxObliqueAngle) {
    return false;
  }

  *oblique_angle = static_cast<int16_t>(scaled);
  range.ConsumeIncludingWhitespace();
  return true;
}

bool ConsumeFontStyleValue(CSSParserTokenRange& range, FontFaceStyle* style) {
  if (range.Peek().GetType() != kIdentToken) return false;

  if (IsIdent(range.Peek(), "normal")) {
    range.ConsumeIncludingWhitespace();
    *style = {FontFaceStyleKind::kNormal, 0, 0};
    return true;
  }
  if (IsIdent(range.Peek(), "italic")) {
    range.ConsumeIncludingWhitespace();
    *style = {FontFaceStyleKind::kItalic, 0, 0};
    return true;
  }
  if (!IsIdent(range.Peek(), "oblique")) return false;
  range.ConsumeIncludingWhitespace();

  FontFaceStyle parsed{FontFaceStyleKind::kOblique, kDefaultObliqueAngle,
                       kDefaultObliqueAngle};
  if (!range.AtEnd()) {
    int16_t first_angle = 0;
    if (!ConsumeAngle(range, &first_angle)) return false;
    parsed.oblique_angle_min = first_angle;
    parsed.oblique_angle_max = first_angle;

    if (!range.AtEnd()) {
      int16_t second_angle = 0;
      if (!ConsumeAngle(range, &second_angle)) return false;
      if (first_angle > second_angle) return false;
      parsed.oblique_angle_max = second_angle;
    }
  }
  *style = parsed;
  return true;
}

bool ParseStyleDescriptor(const std::string& text, FontFaceStyle* style) {
  FontFaceStyle parsed_style;
  if (!ParseDescriptorValue(text, ConsumeFontStyleValue, &parsed_style)) {
    return false;
  }
  *style = parsed_style;
  return true;
}

bool ConsumeVariationSettings(CSSParserTokenRange& range,
                              std::vector<VariationAxis>* settings) {
  std::vector<VariationAxis> parsed;
  if (IsIdent(range.Peek(), "normal")) {
    range.ConsumeIncludingWhitespace();
    *settings = std::move(parsed);
    return true;
  }

  do {
    if (range.AtEnd() || range.Peek().GetType() != kStringToken) {
      return false;
    }

    std::string tag = ToString(range.ConsumeIncludingWhitespace().Value());
    if (!IsValidVariationTag(tag)) return false;

    if (range.AtEnd() || range.Peek().GetType() != kNumberToken ||
        !std::isfinite(range.Peek().NumericValue())) {
      return false;
    }
    parsed.push_back({std::move(tag),
                      static_cast<float>(
                          range.ConsumeIncludingWhitespace().NumericValue())});
  } while (ConsumeCommaIncludingWhitespace(range));

  if (parsed.empty()) return false;
  *settings = std::move(parsed);
  return true;
}

bool ParseVariationSettingsDescriptor(const std::string& text,
                                      std::vector<VariationAxis>* settings) {
  std::vector<VariationAxis> parsed;
  if (!ParseDescriptorValue(text, ConsumeVariationSettings, &parsed)) {
    return false;
  }
  *settings = std::move(parsed);
  return true;
}

bool ConsumeUnicodeRangeList(CSSParserTokenRange& range,
                             std::vector<UnicodeRange>* ranges) {
  std::vector<UnicodeRange> parsed;
  do {
    if (range.AtEnd() || range.Peek().GetType() != kUnicodeRangeToken) {
      return false;
    }
    CSSParserToken token = range.ConsumeIncludingWhitespace();
    uint32_t from = static_cast<uint32_t>(token.UnicodeRangeStart());
    uint32_t to = static_cast<uint32_t>(token.UnicodeRangeEnd());
    if (from > to || to > 0x10FFFF) return false;
    parsed.push_back({from, to});
  } while (ConsumeCommaIncludingWhitespace(range));

  if (parsed.empty()) return false;
  *ranges = std::move(parsed);
  return true;
}

bool ParseUnicodeRangeDescriptor(const std::string& text,
                                 std::vector<UnicodeRange>* ranges) {
  std::vector<UnicodeRange> parsed;
  if (!ParseDescriptorValue(text, ConsumeUnicodeRangeList, &parsed)) {
    return false;
  }
  *ranges = std::move(parsed);
  return true;
}

std::unordered_map<std::string, std::string> NormalizeDescriptorMap(
    const std::unordered_map<std::string, std::string>& descriptors) {
  std::unordered_map<std::string, std::string> normalized;
  normalized.reserve(descriptors.size());
  for (const auto& descriptor : descriptors) {
    normalized[ToLowerASCII(TrimString(descriptor.first))] =
        TrimString(descriptor.second);
  }
  return normalized;
}

}  // namespace

// static
fml::RefPtr<const FontFaceRule> FontFaceParser::Parse(
    const std::unordered_map<std::string, std::string>& descriptors) {
  const auto normalized = NormalizeDescriptorMap(descriptors);

  auto family_it = normalized.find("font-family");
  auto src_it = normalized.find("src");
  if (family_it == normalized.end() || src_it == normalized.end()) {
    return nullptr;
  }

  std::string family;
  std::vector<FontSource> sources;
  if (!ParseFontFamilyDescriptor(family_it->second, &family)) {
    return nullptr;
  }
  if (!ParseSrcDescriptor(src_it->second, &sources)) return nullptr;

  uint16_t weight_min = 400;
  uint16_t weight_max = 400;
  if (auto it = normalized.find("font-weight"); it != normalized.end()) {
    ParseWeightDescriptor(it->second, &weight_min, &weight_max);
  }

  uint16_t stretch_min = 100;
  uint16_t stretch_max = 100;
  if (auto it = normalized.find("font-stretch"); it != normalized.end()) {
    ParseStretchDescriptor(it->second, &stretch_min, &stretch_max);
  }

  FontFaceStyle style{FontFaceStyleKind::kNormal, 0, 0};
  if (auto it = normalized.find("font-style"); it != normalized.end()) {
    ParseStyleDescriptor(it->second, &style);
  }

  std::vector<VariationAxis> variation_settings;
  if (auto it = normalized.find("font-variation-settings");
      it != normalized.end()) {
    ParseVariationSettingsDescriptor(it->second, &variation_settings);
  }

  std::vector<UnicodeRange> unicode_range;
  if (auto it = normalized.find("unicode-range"); it != normalized.end()) {
    ParseUnicodeRangeDescriptor(it->second, &unicode_range);
  }

  return fml::AdoptRef(new FontFaceRule(
      std::move(family), std::move(sources), weight_min, weight_max,
      stretch_min, stretch_max, style, std::move(variation_settings),
      std::move(unicode_range)));
}

}  // namespace css
}  // namespace lynx
