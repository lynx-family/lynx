// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/css_parser_token.h"

#include <algorithm>
#include <limits>

namespace lynx {
namespace css {

// Just a helper used for Delimiter tokens.
CSSParserToken::CSSParserToken(CSSParserTokenType type, UChar c)
    : type_(type), block_type_(kNotBlock), delimiter_(c) {
  DCHECK_EQ(type_, static_cast<unsigned>(kDelimiterToken));
}

CSSParserToken::CSSParserToken(CSSParserTokenType type, double numeric_value,
                               NumericValueType numeric_value_type,
                               NumericSign sign)
    : type_(type),
      block_type_(kNotBlock),
      numeric_value_type_(numeric_value_type),
      numeric_sign_(sign) /*,
       unit_(static_cast<unsigned>(CSSPrimitiveValue::UnitType::kNumber))*/
{
  DCHECK_EQ(type, kNumberToken);
  numeric_value_ =
      std::clamp<double>(numeric_value, -std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max());
}

CSSParserToken::CSSParserToken(CSSParserTokenType type, UChar32 start,
                               UChar32 end)
    : type_(kUnicodeRangeToken), block_type_(kNotBlock) {
  DCHECK_EQ(type, kUnicodeRangeToken);
  unicode_range_.start = start;
  unicode_range_.end = end;
}

CSSParserToken::CSSParserToken(HashTokenType type, const std::u16string& value)
    : type_(kHashToken), block_type_(kNotBlock), hash_token_type_(type) {
  InitValueFromStringView(value);
}

void CSSParserToken::ConvertToDimensionWithUnit(const std::u16string& unit) {
  DCHECK_EQ(type_, static_cast<unsigned>(kNumberToken));
  type_ = kDimensionToken;
  InitValueFromStringView(unit);
  // unit_ = static_cast<unsigned>(CSSPrimitiveValue::StringToUnitType(unit));
}

void CSSParserToken::ConvertToPercentage() {
  DCHECK_EQ(type_, static_cast<unsigned>(kNumberToken));
  type_ = kPercentageToken;
  // unit_ = static_cast<unsigned>(CSSPrimitiveValue::UnitType::kPercentage);
}

UChar CSSParserToken::Delimiter() const {
  DCHECK_EQ(type_, static_cast<unsigned>(kDelimiterToken));
  return delimiter_;
}

NumericSign CSSParserToken::GetNumericSign() const {
  // This is valid for DimensionToken and PercentageToken, but only used
  // in <an+b> parsing on NumberTokens.
  DCHECK_EQ(type_, static_cast<unsigned>(kNumberToken));
  return static_cast<NumericSign>(numeric_sign_);
}

NumericValueType CSSParserToken::GetNumericValueType() const {
  DCHECK(type_ == kNumberToken || type_ == kPercentageToken ||
         type_ == kDimensionToken);
  return static_cast<NumericValueType>(numeric_value_type_);
}

double CSSParserToken::NumericValue() const {
  DCHECK(type_ == kNumberToken || type_ == kPercentageToken ||
         type_ == kDimensionToken);
  return numeric_value_;
}

// CSSPropertyID CSSParserToken::ParseAsUnresolvedCSSPropertyID(
//     const ExecutionContext* execution_context,
//     CSSParserMode mode) const {
//   DCHECK_EQ(type_, static_cast<unsigned>(kIdentToken));
//   return UnresolvedCSSPropertyID(execution_context, Value(), mode);
// }
//
// AtRuleDescriptorID CSSParserToken::ParseAsAtRuleDescriptorID() const {
//   DCHECK_EQ(type_, static_cast<unsigned>(kIdentToken));
//   return AsAtRuleDescriptorID(Value());
// }

// CSSValueID CSSParserToken::Id() const {
//   if (type_ != kIdentToken)
//     return CSSValueID::kInvalid;
//   if (id_ < 0)
//     id_ = static_cast<int>(CssValueKeywordID(Value()));
//   return static_cast<CSSValueID>(id_);
// }

// CSSValueID CSSParserToken::FunctionId() const {
//   if (type_ != kFunctionToken)
//     return CSSValueID::kInvalid;
//   if (id_ < 0)
//     id_ = static_cast<int>(CssValueKeywordID(Value()));
//   return static_cast<CSSValueID>(id_);
// }

// bool CSSParserToken::HasStringBacking() const {
//   CSSParserTokenType token_type = GetType();
//   return token_type == kIdentToken || token_type == kFunctionToken ||
//          token_type == kAtKeywordToken || token_type == kHashToken ||
//          token_type == kUrlToken || token_type == kDimensionToken ||
//          token_type == kStringToken;
// }

// CSSParserToken CSSParserToken::CopyWithUpdatedString(
//     const std::u16string& string) const {
//   CSSParserToken copy(*this);
//   copy.InitValueFromStringView(string);
//   return copy;
// }

bool CSSParserToken::ValueDataCharRawEqual(const CSSParserToken& other) const {
  if (value_length_ != other.value_length_) return false;

  return value_data_char_raw_ == other.value_data_char_raw_;
}

bool CSSParserToken::operator==(const CSSParserToken& other) const {
  if (type_ != other.type_) return false;
  switch (type_) {
    case kDelimiterToken:
      return Delimiter() == other.Delimiter();
    case kHashToken:
      if (hash_token_type_ != other.hash_token_type_) return false;
      [[fallthrough]];
    case kIdentToken:
    case kFunctionToken:
    case kStringToken:
    case kUrlToken:
      return ValueDataCharRawEqual(other);
    case kDimensionToken:
      if (!ValueDataCharRawEqual(other)) return false;
      [[fallthrough]];
    case kNumberToken:
    case kPercentageToken:
      return numeric_sign_ == other.numeric_sign_ &&
             numeric_value_ == other.numeric_value_ &&
             numeric_value_type_ == other.numeric_value_type_;
    case kUnicodeRangeToken:
      return unicode_range_.start == other.unicode_range_.start &&
             unicode_range_.end == other.unicode_range_.end;
    default:
      return true;
  }
}

void CSSParserToken::Serialize(std::string& builder) const {
  // This is currently only used for @supports CSSOM. To keep our implementation
  // simple we handle some of the edge cases incorrectly (see comments below).
}

}  // namespace css
}  // namespace lynx
