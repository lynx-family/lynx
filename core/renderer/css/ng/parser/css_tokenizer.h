// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_H_

#include <climits>
#include <string>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_tokenizer_input_stream.h"

namespace lynx {
namespace css {

class CSSTokenizerInputStream;

class CSSTokenizer {
 public:
  CSSTokenizer(const std::u16string&, size_t offset = 0);
  CSSTokenizer(const std::string&);
  CSSTokenizer(const CSSTokenizer&) = delete;
  CSSTokenizer& operator=(const CSSTokenizer&) = delete;

  std::vector<CSSParserToken> TokenizeToEOF();
  size_t TokenCount();

  size_t Offset() const { return input_.Offset(); }
  size_t PreviousOffset() const { return prev_offset_; }
  std::u16string StringRangeAt(size_t start, size_t length) const;

 private:
  CSSParserToken TokenizeSingle();
  CSSParserToken TokenizeSingleWithComments();

  CSSParserToken NextToken();

  UChar Consume();
  void Reconsume(UChar);

  CSSParserToken ConsumeNumericToken();
  CSSParserToken ConsumeIdentLikeToken();
  CSSParserToken ConsumeNumber();
  CSSParserToken ConsumeStringTokenUntil(UChar);
  CSSParserToken ConsumeUnicodeRange();
  CSSParserToken ConsumeUrlToken();

  void ConsumeBadUrlRemnants();
  void ConsumeSingleWhitespaceIfNext();
  void ConsumeUntilCommentEndFound();

  bool ConsumeIfNext(UChar);
  std::u16string ConsumeName();
  UChar32 ConsumeEscape();

  bool NextTwoCharsAreValidEscape();
  bool NextCharsAreNumber(UChar);
  bool NextCharsAreNumber();
  bool NextCharsAreIdentifier(UChar);
  bool NextCharsAreIdentifier();

  CSSParserToken BlockStart(CSSParserTokenType);
  CSSParserToken BlockStart(CSSParserTokenType block_type, CSSParserTokenType,
                            const std::u16string&);
  CSSParserToken BlockEnd(CSSParserTokenType, CSSParserTokenType start_type);

  CSSParserToken WhiteSpace(UChar);
  CSSParserToken LeftParenthesis(UChar);
  CSSParserToken RightParenthesis(UChar);
  CSSParserToken LeftBracket(UChar);
  CSSParserToken RightBracket(UChar);
  CSSParserToken LeftBrace(UChar);
  CSSParserToken RightBrace(UChar);
  CSSParserToken PlusOrFullStop(UChar);
  CSSParserToken Comma(UChar);
  CSSParserToken HyphenMinus(UChar);
  CSSParserToken Asterisk(UChar);
  CSSParserToken LessThan(UChar);
  CSSParserToken Solidus(UChar);
  CSSParserToken Colon(UChar);
  CSSParserToken SemiColon(UChar);
  CSSParserToken Hash(UChar);
  CSSParserToken CircumflexAccent(UChar);
  CSSParserToken DollarSign(UChar);
  CSSParserToken VerticalLine(UChar);
  CSSParserToken Tilde(UChar);
  CSSParserToken CommercialAt(UChar);
  CSSParserToken ReverseSolidus(UChar);
  CSSParserToken AsciiDigit(UChar);
  CSSParserToken LetterU(UChar);
  CSSParserToken NameStart(UChar);
  CSSParserToken StringStart(UChar);
  CSSParserToken EndOfFile(UChar);

  const std::u16string& RegisterString(const std::u16string&);

  using CodePoint = CSSParserToken (CSSTokenizer::*)(UChar);
  static const CodePoint kCodePoints[];

  CSSTokenizerInputStream input_;
  std::vector<CSSParserTokenType> block_stack_;

  // We only allocate strings when escapes are used.
  std::vector<std::u16string> string_pool_;

  friend class CSSParserTokenStream;

  size_t prev_offset_ = 0;
  size_t token_count_ = 0;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_H_
