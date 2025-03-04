// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_H_

#include <string>
#include <vector>

#include "core/renderer/css/ng/css_ng_utils.h"

namespace lynx {
namespace css {

enum CSSParserTokenType {
  kIdentToken = 0,
  kFunctionToken,
  kAtKeywordToken,
  kHashToken,
  kUrlToken,
  kBadUrlToken,
  kDelimiterToken,
  kNumberToken,
  kPercentageToken,
  kDimensionToken,
  kIncludeMatchToken,
  kDashMatchToken,
  kPrefixMatchToken,
  kSuffixMatchToken,
  kSubstringMatchToken,
  kColumnToken,
  kUnicodeRangeToken,
  kWhitespaceToken,
  kCDOToken,
  kCDCToken,
  kColonToken,
  kSemicolonToken,
  kCommaToken,
  kLeftParenthesisToken,
  kRightParenthesisToken,
  kLeftBracketToken,
  kRightBracketToken,
  kLeftBraceToken,
  kRightBraceToken,
  kStringToken,
  kBadStringToken,
  kEOFToken,
  kCommentToken,
};

enum NumericSign {
  kNoSign,
  kPlusSign,
  kMinusSign,
};

enum NumericValueType {
  kIntegerValueType,
  kNumberValueType,
};

enum HashTokenType {
  kHashTokenId,
  kHashTokenUnrestricted,
};

class CSSParserToken {
 public:
  enum BlockType {
    kNotBlock,
    kBlockStart,
    kBlockEnd,
  };

  CSSParserToken(CSSParserTokenType type, BlockType block_type = kNotBlock)
      : type_(type), block_type_(block_type) {}
  CSSParserToken(CSSParserTokenType type, const std::u16string& value,
                 BlockType block_type = kNotBlock)
      : type_(type), block_type_(block_type) {
    InitValueFromStringView(value);
    id_ = -1;
  }

  CSSParserToken(CSSParserTokenType, UChar);  // for DelimiterToken
  CSSParserToken(CSSParserTokenType, double, NumericValueType,
                 NumericSign);  // for NumberToken
  CSSParserToken(CSSParserTokenType, UChar32,
                 UChar32);  // for UnicodeRangeToken

  CSSParserToken(HashTokenType, const std::u16string&);

  bool operator==(const CSSParserToken& other) const;
  bool operator!=(const CSSParserToken& other) const {
    return !(*this == other);
  }

  // Converts NumberToken to DimensionToken.
  void ConvertToDimensionWithUnit(const std::u16string&);

  // Converts NumberToken to PercentageToken.
  void ConvertToPercentage();

  CSSParserTokenType GetType() const {
    return static_cast<CSSParserTokenType>(type_);
  }
  const std::u16string& Value() const { return value_data_char_raw_; }

  bool IsEOF() const { return type_ == static_cast<unsigned>(kEOFToken); }

  UChar Delimiter() const;
  NumericSign GetNumericSign() const;
  NumericValueType GetNumericValueType() const;
  double NumericValue() const;
  HashTokenType GetHashTokenType() const {
    DCHECK_EQ(type_, static_cast<unsigned>(kHashToken));
    return hash_token_type_;
  }
  BlockType GetBlockType() const { return static_cast<BlockType>(block_type_); }
  //  CSSPrimitiveValue::UnitType GetUnitType() const {
  //    return static_cast<CSSPrimitiveValue::UnitType>(unit_);
  //  }
  UChar32 UnicodeRangeStart() const {
    DCHECK_EQ(type_, static_cast<unsigned>(kUnicodeRangeToken));
    return unicode_range_.start;
  }
  UChar32 UnicodeRangeEnd() const {
    DCHECK_EQ(type_, static_cast<unsigned>(kUnicodeRangeToken));
    return unicode_range_.end;
  }
  // CSSValueID Id() const;
  // CSSValueID FunctionId() const;

  // bool HasStringBacking() const;

  void Serialize(std::string&) const;

  // CSSParserToken CopyWithUpdatedString(const std::u16string&) const;

  static CSSParserTokenType ClosingTokenType(CSSParserTokenType opening_type) {
    switch (opening_type) {
      case kFunctionToken:
      case kLeftParenthesisToken:
        return kRightParenthesisToken;
      case kLeftBracketToken:
        return kRightBracketToken;
      case kLeftBraceToken:
        return kRightBraceToken;
      default:
        // NOTREACHED();
        return kEOFToken;
    }
  }

 private:
  void InitValueFromStringView(const std::u16string& string) {
    value_length_ = string.length();
    value_data_char_raw_ = string;
  }
  bool ValueDataCharRawEqual(const CSSParserToken& other) const;

  unsigned type_ : 6;                // CSSParserTokenType
  unsigned block_type_ : 2;          // BlockType
  unsigned numeric_value_type_ : 1;  // NumericValueType
  unsigned numeric_sign_ : 2;        // NumericSign
  // unsigned unit_ : 7;                // CSSPrimitiveValue::UnitType
  // value_... is an unpacked std::u16string so that we can pack it
  // tightly with the rest of this object for a smaller object size.
  size_t value_length_;
  std::u16string value_data_char_raw_;  // Either LChar* or UChar*.

  union {
    UChar delimiter_;
    HashTokenType hash_token_type_;
    double numeric_value_;
    mutable int id_;

    struct {
      UChar32 start;
      UChar32 end;
    } unicode_range_;
  };
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_H_
