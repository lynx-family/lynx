// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/css_parser_idioms.h"

#include "core/renderer/css/ng/parser/css_tokenizer_input_stream.h"
#include "core/renderer/css/ng/parser/string_to_number.h"

namespace lynx {
namespace css {

void ConsumeSingleWhitespaceIfNext(CSSTokenizerInputStream& input) {
  // We check for \r\n and HTML spaces since we don't do preprocessing
  UChar next = input.PeekWithoutReplacement(0);
  if (next == '\r' && input.PeekWithoutReplacement(1) == '\n')
    input.Advance(2);
  else if (base::IsHTMLSpace(next))
    input.Advance();
}

// https://drafts.csswg.org/css-syntax/#consume-an-escaped-code-point
UChar32 ConsumeEscape(CSSTokenizerInputStream& input) {
  UChar cc = input.NextInputChar();
  input.Advance();
  DCHECK(!IsCSSNewLine(cc));
  if (base::IsASCIIHexNumber(cc)) {
    unsigned consumed_hex_digits = 1;
    std::u16string hex_chars;
    hex_chars.push_back(cc);
    while (consumed_hex_digits < 6 &&
           base::IsASCIIHexNumber(input.PeekWithoutReplacement(0))) {
      cc = input.NextInputChar();
      input.Advance();
      hex_chars.push_back(cc);
      consumed_hex_digits++;
    };
    ConsumeSingleWhitespaceIfNext(input);
    bool ok = false;
    UChar32 code_point =
        HexCharactersToUInt(hex_chars.c_str(), hex_chars.length(),
                            NumberParsingOptions::kStrict, &ok);
    DCHECK(ok);
    if (code_point == 0 || (0xD800 <= code_point && code_point <= 0xDFFF) ||
        code_point > 0x10FFFF)
      return kReplacementCharacter;
    return code_point;
  }

  if (cc == kEndOfFileMarker) return kReplacementCharacter;
  return cc;
}

// http://www.w3.org/TR/css3-syntax/#consume-a-name
std::u16string ConsumeName(CSSTokenizerInputStream& input) {
  std::u16string result;
  while (true) {
    UChar cc = input.NextInputChar();
    input.Advance();
    if (IsNameCodePoint(cc)) {
      result.push_back(cc);
      continue;
    }
    if (TwoCharsAreValidEscape(cc, input.PeekWithoutReplacement(0))) {
      result.push_back(ConsumeEscape(input));
      continue;
    }
    input.PushBack(cc);
    return result;
  }
}

// https://drafts.csswg.org/css-syntax/#would-start-an-identifier
bool NextCharsAreIdentifier(UChar first, const CSSTokenizerInputStream& input) {
  UChar second = input.PeekWithoutReplacement(0);
  if (IsNameStartCodePoint(first) || TwoCharsAreValidEscape(first, second))
    return true;

  if (first == '-') {
    return IsNameStartCodePoint(second) || second == '-' ||
           TwoCharsAreValidEscape(second, input.PeekWithoutReplacement(1));
  }

  return false;
}

}  // namespace css
}  // namespace lynx
