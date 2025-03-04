// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/css_parser_token_range.h"

#include <string>

namespace lynx {
namespace css {

CSSParserToken CSSParserTokenRange::g_static_eof_token =
    CSSParserToken(kEOFToken);

CSSParserTokenRange CSSParserTokenRange::MakeSubRange(
    const CSSParserToken* first, const CSSParserToken* last) const {
  if (first == &g_static_eof_token) first = last_;
  if (last == &g_static_eof_token) last = last_;
  DCHECK(first <= last);
  return CSSParserTokenRange(first, last);
}

CSSParserTokenRange CSSParserTokenRange::ConsumeBlock() {
  DCHECK_EQ(Peek().GetBlockType(), CSSParserToken::kBlockStart);
  const CSSParserToken* start = &Peek() + 1;
  unsigned nesting_level = 0;
  do {
    const CSSParserToken& token = Consume();
    if (token.GetBlockType() == CSSParserToken::kBlockStart)
      nesting_level++;
    else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      nesting_level--;
  } while (nesting_level && first_ < last_);

  if (nesting_level) return MakeSubRange(start, first_);  // Ended at EOF
  return MakeSubRange(start, first_ - 1);
}

void CSSParserTokenRange::ConsumeComponentValue() {
  // FIXME: This is going to do multiple passes over large sections of a
  // stylesheet. We should consider optimising this by precomputing where each
  // block ends.
  unsigned nesting_level = 0;
  do {
    const CSSParserToken& token = Consume();
    if (token.GetBlockType() == CSSParserToken::kBlockStart)
      nesting_level++;
    else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      nesting_level--;
  } while (nesting_level && first_ < last_);
}

std::string CSSParserTokenRange::Serialize() const {
  // We're supposed to insert comments between certain pairs of token types
  // as per spec, but since this is currently only used for @supports CSSOM
  // and CSS Paint API arguments we just get these cases wrong and avoid the
  // additional complexity.
  std::string builder;
  for (const CSSParserToken* it = first_; it != last_; ++it)
    it->Serialize(builder);
  return builder;
}

}  // namespace css
}  // namespace lynx
