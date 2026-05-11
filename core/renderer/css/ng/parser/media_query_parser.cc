// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/media_query_parser.h"

#include <cctype>
#include <sstream>
#include <utility>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/parser/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_stream.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"

namespace lynx {
namespace css {

namespace {

// Comparison operators made of one or two tokens. The tokenizer emits `<`,
// `>`, and `=` each as individual DelimiterTokens, so we must peek ahead.
bool ReadRelOp(CSSParserTokenStream& stream, MediaFeatureOperator& out) {
  const CSSParserToken& first = stream.Peek();
  if (first.GetType() != kDelimiterToken) return false;
  const UChar head = first.Delimiter();
  if (head != u'<' && head != u'>' && head != u'=') return false;

  stream.Consume();
  // Look at the very next token (no whitespace consumed) to detect `<=` /
  // `>=`. CSS Media Queries Level 4 explicitly bans whitespace between the
  // two delimiters in a relational operator.
  bool has_equals = false;
  if (stream.Peek().GetType() == kDelimiterToken &&
      stream.Peek().Delimiter() == u'=') {
    stream.Consume();
    has_equals = true;
  }

  switch (head) {
    case u'<':
      out = has_equals ? MediaFeatureOperator::kLe : MediaFeatureOperator::kLt;
      return true;
    case u'>':
      out = has_equals ? MediaFeatureOperator::kGe : MediaFeatureOperator::kGt;
      return true;
    case u'=':
      if (has_equals) return false;  // `==` is not a valid operator.
      out = MediaFeatureOperator::kEq;
      return true;
    default:
      return false;
  }
}

// Flip < to > (and <= to >=) when the feature appears on the right of a
// range expression. This normalizes `100px < width < 500px` into a pair of
// operators anchored on the feature name.
MediaFeatureOperator FlipRelOp(MediaFeatureOperator op) {
  switch (op) {
    case MediaFeatureOperator::kLt:
      return MediaFeatureOperator::kGt;
    case MediaFeatureOperator::kLe:
      return MediaFeatureOperator::kGe;
    case MediaFeatureOperator::kGt:
      return MediaFeatureOperator::kLt;
    case MediaFeatureOperator::kGe:
      return MediaFeatureOperator::kLe;
    default:
      return op;
  }
}

bool IsLtLeOp(MediaFeatureOperator op) {
  return op == MediaFeatureOperator::kLt || op == MediaFeatureOperator::kLe;
}

bool IsGtGeOp(MediaFeatureOperator op) {
  return op == MediaFeatureOperator::kGt || op == MediaFeatureOperator::kGe;
}

bool AreRangeOpsCompatible(MediaFeatureOperator left,
                           MediaFeatureOperator right) {
  return (IsLtLeOp(left) && IsLtLeOp(right)) ||
         (IsGtGeOp(left) && IsGtGeOp(right));
}

// Resolve a CSS dimension suffix (length or resolution) to a MediaFeatureUnit.
// Returns kUnknown if the suffix is not recognized.
MediaFeatureUnit ResolveUnit(const std::u16string& unit_u16) {
  if (EqualIgnoringASCIICase(unit_u16, u"px")) {
    return MediaFeatureUnit::kPixels;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"rem")) {
    return MediaFeatureUnit::kRem;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"em")) {
    return MediaFeatureUnit::kEm;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"vw")) {
    return MediaFeatureUnit::kViewportWidth;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"vh")) {
    return MediaFeatureUnit::kViewportHeight;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"vmin")) {
    return MediaFeatureUnit::kViewportMin;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"vmax")) {
    return MediaFeatureUnit::kViewportMax;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"dppx") ||
      EqualIgnoringASCIICase(unit_u16, u"x")) {
    return MediaFeatureUnit::kDppx;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"dpi")) {
    return MediaFeatureUnit::kDpi;
  }
  if (EqualIgnoringASCIICase(unit_u16, u"dpcm")) {
    return MediaFeatureUnit::kDpcm;
  }
  return MediaFeatureUnit::kUnknown;
}

}  // namespace

// ---- public entry points ---------------------------------------------------

fml::RefPtr<MediaQuerySet> MediaQueryParser::ParseMediaQuerySet(
    const std::string& text) {
  auto result = fml::MakeRefCounted<MediaQuerySet>();
  if (text.empty()) return result;

  CSSTokenizer tokenizer(text);
  CSSParserTokenStream stream(tokenizer);
  stream.ConsumeWhitespace();

  auto skip_to_comma = [&stream]() {
    while (!stream.AtEnd() && stream.Peek().GetType() != kCommaToken) {
      if (stream.Peek().GetBlockType() == CSSParserToken::kBlockStart) {
        CSSParserTokenStream::BlockGuard skip_guard(stream);
      } else {
        stream.Consume();
      }
    }
  };

  MediaQueryParser parser(stream);
  while (!stream.AtEnd()) {
    auto query = parser.ConsumeMediaQuery();
    stream.ConsumeWhitespace();
    if (query && (stream.AtEnd() || stream.Peek().GetType() == kCommaToken)) {
      result->Append(std::move(query));
    } else {
      skip_to_comma();
    }
    if (stream.Peek().GetType() == kCommaToken) {
      stream.Consume();
      stream.ConsumeWhitespace();
    }
  }
  return result;
}

fml::RefPtr<const MediaQueryExpNode> MediaQueryParser::ParseMediaCondition(
    const std::string& text) {
  if (text.empty()) return nullptr;
  CSSTokenizer tokenizer(text);
  CSSParserTokenStream stream(tokenizer);
  stream.ConsumeWhitespace();
  MediaQueryParser parser(stream);
  auto node = parser.ConsumeCondition(/*allow_or=*/true);
  stream.ConsumeWhitespace();
  if (!stream.AtEnd()) return nullptr;
  return node;
}

// ---- grammar ---------------------------------------------------------------

fml::RefPtr<MediaQuery> MediaQueryParser::ConsumeMediaQuery() {
  stream_.ConsumeWhitespace();

  MediaQueryRestrictor restrictor = MediaQueryRestrictor::kNone;
  std::string media_type;
  fml::RefPtr<const MediaQueryExpNode> condition;

  // Try `<media-condition>` first -- detected by the next token being `(` or
  // an identifier that is specifically `not` followed by `(`.
  const CSSParserToken& first = stream_.Peek();
  if (first.GetType() == kLeftParenthesisToken) {
    condition = ConsumeCondition(/*allow_or=*/true);
    if (!condition) return nullptr;
    stream_.ConsumeWhitespace();
    return fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                           std::string(), std::move(condition));
  }

  if (PeekIdentIgnoringCase("not") || PeekIdentIgnoringCase("only")) {
    if (PeekIdentIgnoringCase("not")) {
      // We need a 2-token lookahead. Consume the `not`, peek, then rewind by
      // letting ConsumeCondition re-create the `not` node from the stream if
      // needed. Simpler: branch eagerly based on the token following `not`.
      // To implement without extra lookahead, peek at the raw `(` case by
      // snapshotting: consume the ident, check next.
      stream_.Consume();
      stream_.ConsumeWhitespace();
      if (stream_.Peek().GetType() == kLeftParenthesisToken) {
        auto operand = ConsumeInParens();
        if (!operand) return nullptr;
        auto not_node =
            fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(operand));
        return fml::MakeRefCounted<MediaQuery>(
            MediaQueryRestrictor::kNone, std::string(), std::move(not_node));
      }
      // Otherwise the `not` was a modifier.
      restrictor = MediaQueryRestrictor::kNot;
    } else {
      stream_.Consume();  // "only"
      restrictor = MediaQueryRestrictor::kOnly;
    }
    stream_.ConsumeWhitespace();
  }

  // <media-type>
  const CSSParserToken& type_tok = stream_.Peek();
  if (type_tok.GetType() != kIdentToken) return nullptr;
  if (PeekIdentIgnoringCase("and") || PeekIdentIgnoringCase("or")) {
    return nullptr;
  }
  media_type = ToLowerASCII(type_tok.Value());
  stream_.Consume();
  stream_.ConsumeWhitespace();

  // Optional: "and <media-condition-without-or>"
  if (PeekIdentIgnoringCase("and")) {
    stream_.Consume();
    stream_.ConsumeWhitespace();
    condition = ConsumeCondition(/*allow_or=*/false);
    if (!condition) return nullptr;
  }

  return fml::MakeRefCounted<MediaQuery>(restrictor, std::move(media_type),
                                         std::move(condition));
}

fml::RefPtr<const MediaQueryExpNode> MediaQueryParser::ConsumeCondition(
    bool allow_or) {
  stream_.ConsumeWhitespace();

  // `not` <media-in-parens>
  if (PeekIdentIgnoringCase("not")) {
    stream_.Consume();
    stream_.ConsumeWhitespace();
    auto operand = ConsumeInParens();
    if (!operand) return nullptr;
    return fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(operand));
  }

  auto left = ConsumeInParens();
  if (!left) return nullptr;

  // A media-condition is either all `and`s or all `or`s -- they cannot be
  // mixed at the same nesting level without parentheses.
  enum class Connective { kNone, kAnd, kOr };
  Connective connective = Connective::kNone;
  while (true) {
    stream_.ConsumeWhitespace();
    Connective next = Connective::kNone;
    if (PeekIdentIgnoringCase("and")) {
      next = Connective::kAnd;
    } else if (allow_or && PeekIdentIgnoringCase("or")) {
      next = Connective::kOr;
    } else {
      break;
    }
    if (connective != Connective::kNone && next != connective) {
      // Mixing and/or without parentheses is a syntax error.
      return nullptr;
    }
    connective = next;
    stream_.Consume();  // consume the connective ident
    stream_.ConsumeWhitespace();

    auto right = ConsumeInParens();
    if (!right) return nullptr;

    if (connective == Connective::kAnd) {
      left = fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                       std::move(right));
    } else {
      left = fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                      std::move(right));
    }
  }
  return left;
}

fml::RefPtr<const MediaQueryExpNode> MediaQueryParser::ConsumeInParens() {
  stream_.ConsumeWhitespace();
  const CSSParserToken& open = stream_.Peek();
  if (open.GetType() != kLeftParenthesisToken) return nullptr;

  // The token stream treats `(` as a block start; use BlockGuard so the
  // matching `)` is consumed automatically even if we fail mid-way.
  CSSParserTokenStream::BlockGuard guard(stream_);
  stream_.ConsumeWhitespace();

  // Try nested condition first: `( <media-condition> )`.
  const CSSParserToken& inner = stream_.Peek();
  if (inner.GetType() == kLeftParenthesisToken ||
      (inner.GetType() == kIdentToken &&
       EqualIgnoringASCIICase(inner.Value(), u"not"))) {
    auto inner_cond = ConsumeCondition(/*allow_or=*/true);
    if (!inner_cond) return nullptr;
    stream_.ConsumeWhitespace();
    if (!stream_.AtEnd()) return nullptr;  // extra junk inside `()`
    return fml::MakeRefCounted<MediaQueryNestedExpNode>(std::move(inner_cond));
  }

  // Otherwise it must be a <media-feature>.
  auto feature_node = ConsumeFeature();
  if (!feature_node) return nullptr;
  stream_.ConsumeWhitespace();
  if (!stream_.AtEnd()) return nullptr;
  return feature_node;
}

fml::RefPtr<const MediaQueryExpNode> MediaQueryParser::ConsumeFeature() {
  stream_.ConsumeWhitespace();
  const CSSParserToken& head = stream_.Peek();

  // Case A: plain form `(name)` or `(name: value)` / `(name op value)`.
  if (head.GetType() == kIdentToken) {
    std::string name;
    if (!ConsumeFeatureName(name)) return nullptr;
    MediaFeatureId id = ResolveMediaFeatureId(name);
    stream_.ConsumeWhitespace();

    // Boolean form: nothing else in the parens.
    if (stream_.AtEnd()) {
      return fml::MakeRefCounted<MediaQueryFeatureExpNode>(
          MediaFeature(id, std::move(name), MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean()));
    }

    // Plain form `(name: value)`.
    if (stream_.Peek().GetType() == kColonToken) {
      stream_.Consume();
      stream_.ConsumeWhitespace();
      MediaFeatureValue value;
      if (!ConsumeFeatureValue(value)) return nullptr;
      return fml::MakeRefCounted<MediaQueryFeatureExpNode>(MediaFeature(
          id, std::move(name), MediaFeatureOperator::kNone, std::move(value)));
    }

    // Range form `(name op value)` -- Level 4. The right-hand value in a
    // range comparison must be numeric (number / percentage / dimension /
    // ratio). Identifiers are reserved for the plain form `(name: ident)`.
    MediaFeatureOperator op = MediaFeatureOperator::kNone;
    if (!ReadRelOp(stream_, op)) return nullptr;
    stream_.ConsumeWhitespace();
    const CSSParserTokenType rhs_type = stream_.Peek().GetType();
    if (rhs_type != kNumberToken && rhs_type != kPercentageToken &&
        rhs_type != kDimensionToken) {
      return nullptr;
    }
    MediaFeatureValue value;
    if (!ConsumeFeatureValue(value)) return nullptr;
    return fml::MakeRefCounted<MediaQueryFeatureExpNode>(
        MediaFeature(id, std::move(name), op, std::move(value)));
  }

  // Case B: range with leading value `(value op name op value)` / `(value op
  // name)`. The leading value must be numeric (number / percentage /
  // dimension / ratio) -- identifiers are reserved for Case A feature names.
  const CSSParserTokenType head_type = head.GetType();
  if (head_type != kNumberToken && head_type != kPercentageToken &&
      head_type != kDimensionToken) {
    return nullptr;
  }
  MediaFeatureValue left_value;
  if (!ConsumeFeatureValue(left_value)) return nullptr;
  stream_.ConsumeWhitespace();

  MediaFeatureOperator left_op_raw = MediaFeatureOperator::kNone;
  if (!ReadRelOp(stream_, left_op_raw)) return nullptr;
  stream_.ConsumeWhitespace();

  std::string name;
  if (!ConsumeFeatureName(name)) return nullptr;
  MediaFeatureId id = ResolveMediaFeatureId(name);
  stream_.ConsumeWhitespace();

  MediaFeature feature(id, std::move(name), FlipRelOp(left_op_raw),
                       std::move(left_value));

  if (!stream_.AtEnd()) {
    MediaFeatureOperator right_op = MediaFeatureOperator::kNone;
    if (!ReadRelOp(stream_, right_op)) return nullptr;
    if (!AreRangeOpsCompatible(left_op_raw, right_op)) return nullptr;
    stream_.ConsumeWhitespace();
    const CSSParserTokenType rhs_type = stream_.Peek().GetType();
    if (rhs_type != kNumberToken && rhs_type != kPercentageToken &&
        rhs_type != kDimensionToken) {
      return nullptr;
    }
    MediaFeatureValue right_value;
    if (!ConsumeFeatureValue(right_value)) return nullptr;
    feature.SetRightBound(right_op, std::move(right_value));
  }

  return fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(feature));
}

// ---- small helpers ---------------------------------------------------------

bool MediaQueryParser::ConsumeIdentIgnoringCase(const char* lowercase_ascii) {
  if (!PeekIdentIgnoringCase(lowercase_ascii)) return false;
  stream_.Consume();
  return true;
}

bool MediaQueryParser::PeekIdentIgnoringCase(const char* lowercase_ascii) {
  const CSSParserToken& tok = stream_.Peek();
  if (tok.GetType() != kIdentToken) return false;
  std::u16string expected(
      lowercase_ascii,
      lowercase_ascii + std::char_traits<char>::length(lowercase_ascii));
  return EqualIgnoringASCIICase(tok.Value(), expected);
}

bool MediaQueryParser::ConsumeFeatureOperator(MediaFeatureOperator& out) {
  return ReadRelOp(stream_, out);
}

bool MediaQueryParser::ConsumeFeatureName(std::string& out) {
  const CSSParserToken& tok = stream_.Peek();
  if (tok.GetType() != kIdentToken) return false;
  out = ToLowerASCII(tok.Value());
  stream_.Consume();
  return true;
}

bool MediaQueryParser::ConsumeFeatureValue(MediaFeatureValue& out) {
  const CSSParserToken& tok = stream_.Peek();
  switch (tok.GetType()) {
    case kNumberToken: {
      const double num = tok.NumericValue();
      stream_.Consume();
      stream_.ConsumeWhitespace();
      // Ratio support: `<number> / <number>`.
      if (stream_.Peek().GetType() == kDelimiterToken &&
          stream_.Peek().Delimiter() == u'/') {
        stream_.Consume();
        stream_.ConsumeWhitespace();
        const CSSParserToken& denom = stream_.Peek();
        if (denom.GetType() != kNumberToken) return false;
        const double d = denom.NumericValue();
        if (d < 0) return false;
        stream_.Consume();
        out = MediaFeatureValue::Ratio(num, d);
      } else {
        out = MediaFeatureValue::Number(num);
      }
      return true;
    }
    case kPercentageToken: {
      const double num = tok.NumericValue();
      stream_.Consume();
      out = MediaFeatureValue::Dimension(num, MediaFeatureUnit::kPercent);
      return true;
    }
    case kDimensionToken: {
      const double num = tok.NumericValue();
      const std::u16string unit = tok.Value();
      stream_.Consume();

      MediaFeatureUnit resolved = ResolveUnit(unit);
      if (resolved != MediaFeatureUnit::kUnknown) {
        out = MediaFeatureValue::Dimension(num, resolved);
        return true;
      }
      // Unknown unit: preserve the original suffix (plus numeric prefix) so
      // diagnostics can surface what the page actually wrote.
      std::string unit_utf8 = ustring_helper::to_string(unit);
      std::ostringstream oss;
      oss << num << unit_utf8;
      std::string text = oss.str();
      out = MediaFeatureValue::Invalid(std::move(text));
      return true;
    }
    case kIdentToken: {
      out = MediaFeatureValue::Ident(ToLowerASCII(tok.Value()));
      stream_.Consume();
      return true;
    }
    default:
      return false;
  }
}

}  // namespace css
}  // namespace lynx
