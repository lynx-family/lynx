// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_RANGE_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_RANGE_H_

#include <string>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token.h"

namespace lynx {
namespace css {

// A CSSParserTokenRange is an iterator over a subrange of a vector of
// CSSParserTokens. Accessing outside of the range will return an endless stream
// of EOF tokens. This class refers to half-open intervals [first, last).
class CSSParserTokenRange {
  static CSSParserToken g_static_eof_token;

 public:
  CSSParserTokenRange(const std::vector<CSSParserToken>& vector)
      : first_(vector.data()),
        last_(vector.data() + std::distance(vector.begin(), vector.end())) {}

  // This should be called on a range with tokens returned by that range.
  CSSParserTokenRange MakeSubRange(const CSSParserToken* first,
                                   const CSSParserToken* last) const;

  bool AtEnd() const { return first_ == last_; }
  const CSSParserToken* end() const { return last_; }

  const CSSParserToken& Peek(size_t offset = 0) const {
    if (first_ + offset >= last_) return g_static_eof_token;
    return *(first_ + offset);
  }

  const CSSParserToken& Consume() {
    if (first_ == last_) return g_static_eof_token;
    return *first_++;
  }

  const CSSParserToken& ConsumeIncludingWhitespace() {
    const CSSParserToken& result = Consume();
    ConsumeWhitespace();
    return result;
  }

  // The returned range doesn't include the brackets
  CSSParserTokenRange ConsumeBlock();

  void ConsumeComponentValue();

  void ConsumeWhitespace() {
    while (Peek().GetType() == kWhitespaceToken) ++first_;
  }

  std::string Serialize() const;

  const CSSParserToken* begin() const { return first_; }

 private:
  CSSParserTokenRange(const CSSParserToken* first, const CSSParserToken* last)
      : first_(first), last_(last) {}

  const CSSParserToken* first_;
  const CSSParserToken* last_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_TOKEN_RANGE_H_
