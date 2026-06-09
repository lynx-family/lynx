// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/supports_condition_parser.h"

#include <optional>
#include <utility>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/parser/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_stream.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"

namespace lynx {
namespace css {

// static
fml::RefPtr<const SupportsConditionNode> SupportsConditionParser::Parse(
    const std::string& text) {
  if (text.empty()) return nullptr;
  CSSTokenizer tokenizer(text);
  CSSParserTokenStream stream(tokenizer);
  stream.ConsumeWhitespace();
  if (stream.AtEnd()) return nullptr;
  SupportsConditionParser parser(stream);
  auto node = parser.ConsumeCondition();
  stream.ConsumeWhitespace();
  if (!stream.AtEnd()) return nullptr;
  return node;
}

// <supports-condition>  = not <supports-in-parens>
//                        | <supports-in-parens> [ and|or <supports-in-parens>
//                        ]*

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::ConsumeCondition() {
  stream_.ConsumeWhitespace();

  // `not` <supports-in-parens>
  if (PeekIdentIgnoringCase("not")) {
    stream_.Consume();
    stream_.ConsumeWhitespace();
    auto operand = ConsumeInParens();
    if (!operand) return nullptr;
    return fml::MakeRefCounted<SupportsUnaryNode>(
        SupportsConditionNode::Type::kNot, std::move(operand));
  }

  auto left = ConsumeInParens();
  if (!left) return nullptr;

  // and/or chain (cannot mix without parentheses)
  enum class Connective { kNone, kAnd, kOr };
  Connective connective = Connective::kNone;
  while (true) {
    stream_.ConsumeWhitespace();
    Connective next = Connective::kNone;
    if (PeekIdentIgnoringCase("and")) {
      next = Connective::kAnd;
    } else if (PeekIdentIgnoringCase("or")) {
      next = Connective::kOr;
    } else {
      break;
    }
    if (connective != Connective::kNone && next != connective) {
      return nullptr;  // mixing and/or is a syntax error
    }
    connective = next;
    stream_.Consume();
    stream_.ConsumeWhitespace();

    auto right = ConsumeInParens();
    if (!right) return nullptr;

    if (connective == Connective::kAnd) {
      left = fml::MakeRefCounted<SupportsBinaryNode>(
          SupportsConditionNode::Type::kAnd, std::move(left), std::move(right));
    } else {
      left = fml::MakeRefCounted<SupportsBinaryNode>(
          SupportsConditionNode::Type::kOr, std::move(left), std::move(right));
    }
  }
  return left;
}

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::ConsumeInParens() {
  stream_.ConsumeWhitespace();
  const CSSParserToken& token = stream_.Peek();

  // Function token → SupportsFunctionNode
  if (token.GetType() == kFunctionToken) {
    return ConsumeFunction();
  }

  // Must be `(` — nested condition, declaration, or general-enclosed.
  if (token.GetType() != kLeftParenthesisToken) return nullptr;

  CSSParserTokenStream::BlockGuard guard(stream_);
  stream_.ConsumeWhitespace();
  size_t content_start = stream_.LookAheadOffset();

  // If first inner token is `(`, function, or `not` → try as nested condition.
  const CSSParserToken& inner = stream_.Peek();
  if (inner.GetType() == kLeftParenthesisToken ||
      inner.GetType() == kFunctionToken ||
      (inner.GetType() == kIdentToken &&
       EqualIgnoringASCIICase(inner.Value(), u"not"))) {
    auto inner_cond = ConsumeCondition();
    if (inner_cond) {
      stream_.ConsumeWhitespace();
      if (stream_.AtEnd()) {
        return fml::MakeRefCounted<SupportsUnaryNode>(
            SupportsConditionNode::Type::kNested, std::move(inner_cond));
      }
    }
    // Failed or trailing tokens → general-enclosed.
    return MakeGeneralEnclosed(content_start);
  }

  // Try as declaration: ident `:` value
  if (inner.GetType() == kIdentToken) {
    auto decl = ConsumeDeclaration();
    if (decl) {
      stream_.ConsumeWhitespace();
      if (stream_.AtEnd()) return decl;
    }
    return MakeGeneralEnclosed(content_start);
  }

  // Anything else → general-enclosed.
  return MakeGeneralEnclosed(content_start);
}

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::ConsumeDeclaration() {
  stream_.ConsumeWhitespace();
  const CSSParserToken& prop_tok = stream_.Peek();
  if (prop_tok.GetType() != kIdentToken) return nullptr;
  std::string property;
  if (prop_tok.Value().size() >= 2 && prop_tok.Value()[0] == u'-' &&
      prop_tok.Value()[1] == u'-') {
    property = ustring_helper::to_string(prop_tok.Value());
  } else {
    property = ToLowerASCII(prop_tok.Value());
  }
  stream_.Consume();
  stream_.ConsumeWhitespace();

  if (stream_.Peek().GetType() != kColonToken) return nullptr;
  stream_.Consume();
  stream_.ConsumeWhitespace();

  stream_.Peek();
  size_t value_start = stream_.LookAheadOffset();
  bool important = false;
  size_t value_end = value_start;

  while (!stream_.AtEnd()) {
    const CSSParserToken& tok = stream_.Peek();

    if (tok.GetType() == kDelimiterToken && tok.Delimiter() == u'!') {
      size_t bang_offset = stream_.LookAheadOffset();
      stream_.Consume();
      stream_.ConsumeWhitespace();
      if (!stream_.AtEnd() && stream_.Peek().GetType() == kIdentToken &&
          EqualIgnoringASCIICase(stream_.Peek().Value(), u"important")) {
        stream_.Consume();
        stream_.ConsumeWhitespace();
        if (stream_.AtEnd()) {
          important = true;
          value_end = bang_offset;
          break;
        }
      }
      continue;
    }

    if (tok.GetType() == kSemicolonToken) {
      break;
    }

    if (tok.GetType() == kFunctionToken ||
        tok.GetType() == kLeftParenthesisToken ||
        tok.GetType() == kLeftBraceToken ||
        tok.GetType() == kLeftBracketToken) {
      stream_.UncheckedConsumeComponentValue();
    } else {
      stream_.Consume();
    }
  }

  if (!important) {
    value_end = stream_.LookAheadOffset();
  }

  if (value_end <= value_start) return nullptr;

  auto raw = stream_.StringRangeAt(value_start, value_end - value_start);
  std::string value = SimplifyWhiteSpace(ustring_helper::to_string(raw));
  if (value.empty()) return nullptr;

  return fml::MakeRefCounted<SupportsDeclNode>(std::move(property),
                                               std::move(value), important);
}

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::ConsumeFunction() {
  const CSSParserToken& fn_tok = stream_.Peek();
  std::string fn_name = ToLowerASCII(fn_tok.Value());

  CSSParserTokenStream::BlockGuard guard(stream_);
  stream_.ConsumeWhitespace();

  if (fn_name == "-x-engine-version") {
    return ConsumeEngineVersion();
  }

  // All other functions → generic SupportsFunctionNode.
  std::string raw_args = ConsumeRemainingAsString();
  return fml::MakeRefCounted<SupportsFunctionNode>(std::move(fn_name),
                                                   std::move(raw_args));
}

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::ConsumeEngineVersion() {
  // -x-engine-version(<begin> [, <end>])
  // <begin> must be a "major.minor" number literal.
  // <end> may be "major.minor" or `*` (meaning kMaxVersion).
  auto parse_segment = [](const std::u16string& str, size_t begin,
                          size_t end) -> std::optional<uint16_t> {
    if (begin >= end) return std::nullopt;
    uint32_t value = 0;
    for (size_t i = begin; i < end; ++i) {
      if (!base::IsASCIINumber(str[i])) return std::nullopt;
      value = value * 10 + (str[i] - u'0');
      if (value > SupportsEngineVersionNode::kVersionMax) return std::nullopt;
    }
    return static_cast<uint16_t>(value);
  };

  auto parse_version_text =
      [&](const std::u16string& str) -> std::optional<uint32_t> {
    auto dot = str.find(u'.');
    if (dot == 0 || dot == std::u16string::npos || dot + 1 >= str.size())
      return std::nullopt;
    auto major = parse_segment(str, 0, dot);
    auto minor = parse_segment(str, dot + 1, str.size());
    if (!major || !minor) return std::nullopt;
    return SupportsEngineVersionNode::PackVersion(*major, *minor);
  };

  auto consume_version_arg = [&](bool allow_star) -> std::optional<uint32_t> {
    stream_.ConsumeWhitespace();
    const CSSParserToken& tok = stream_.Peek();
    if (allow_star && tok.GetType() == kDelimiterToken &&
        tok.Delimiter() == u'*') {
      stream_.Consume();
      return SupportsEngineVersionNode::kMaxVersion;
    }
    if (tok.GetType() == kNumberToken) {
      // Read the original source text for this token to preserve precision.
      size_t start = stream_.LookAheadOffset();
      stream_.Consume();
      size_t end = stream_.Offset();
      auto raw = stream_.StringRangeAt(start, end - start);
      return parse_version_text(raw);
    }
    return std::nullopt;
  };

  std::optional<uint32_t> begin = consume_version_arg(false);
  if (!begin) return nullptr;

  stream_.ConsumeWhitespace();
  uint32_t end = SupportsEngineVersionNode::kMaxVersion;
  if (!stream_.AtEnd() && stream_.Peek().GetType() == kCommaToken) {
    stream_.Consume();
    auto parsed_end = consume_version_arg(true);
    if (!parsed_end) return nullptr;
    end = *parsed_end;
  }

  // Reject trailing tokens: only whitespace allowed before closing ')'.
  stream_.ConsumeWhitespace();
  if (!stream_.AtEnd()) return nullptr;

  return fml::MakeRefCounted<SupportsEngineVersionNode>(*begin, end);
}

// ---- helpers -------

bool SupportsConditionParser::PeekIdentIgnoringCase(
    const char* lowercase_ascii) {
  const CSSParserToken& tok = stream_.Peek();
  if (tok.GetType() != kIdentToken) return false;
  std::u16string expected(
      lowercase_ascii,
      lowercase_ascii + std::char_traits<char>::length(lowercase_ascii));
  return EqualIgnoringASCIICase(tok.Value(), expected);
}

std::string SupportsConditionParser::ConsumeRemainingAsString() {
  size_t start = stream_.LookAheadOffset();

  while (!stream_.AtEnd()) {
    const CSSParserToken& tok = stream_.Peek();
    if (tok.GetType() == kFunctionToken ||
        tok.GetType() == kLeftParenthesisToken ||
        tok.GetType() == kLeftBraceToken ||
        tok.GetType() == kLeftBracketToken) {
      stream_.UncheckedConsumeComponentValue();
    } else {
      stream_.Consume();
    }
  }

  size_t end = stream_.LookAheadOffset();
  if (end <= start) return {};

  auto raw = stream_.StringRangeAt(start, end - start);
  return SimplifyWhiteSpace(ustring_helper::to_string(raw));
}

fml::RefPtr<const SupportsConditionNode>
SupportsConditionParser::MakeGeneralEnclosed(size_t content_start) {
  while (!stream_.AtEnd()) {
    const CSSParserToken& tok = stream_.Peek();
    if (tok.GetType() == kFunctionToken ||
        tok.GetType() == kLeftParenthesisToken ||
        tok.GetType() == kLeftBraceToken ||
        tok.GetType() == kLeftBracketToken) {
      stream_.UncheckedConsumeComponentValue();
    } else {
      stream_.Consume();
    }
  }

  size_t end = stream_.LookAheadOffset();
  size_t len = (end > content_start) ? (end - content_start) : 0;
  std::string raw;
  if (len > 0) {
    auto s = stream_.StringRangeAt(content_start, len);
    raw = "(" + SimplifyWhiteSpace(ustring_helper::to_string(s)) + ")";
  } else {
    raw = "()";
  }
  return fml::MakeRefCounted<SupportsGeneralEnclosedNode>(std::move(raw));
}

}  // namespace css
}  // namespace lynx
