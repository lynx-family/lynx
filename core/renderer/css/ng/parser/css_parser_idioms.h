
/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_IDIOMS_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_IDIOMS_H_

#include <string>

#include "base/include/string/string_utils.h"

namespace lynx {
namespace css {

class CSSTokenizerInputStream;

// Space characters as defined by the CSS specification.
// http://www.w3.org/TR/css3-syntax/#whitespace
inline bool IsCSSSpace(base::UChar c) {
  return c == ' ' || c == '\t' || c == '\n';
}

inline bool IsCSSNewLine(base::UChar cc) {
  // We check \r and \f here, since we have no preprocessing stage
  return (cc == '\r' || cc == '\n' || cc == '\f');
}

// https://drafts.csswg.org/css-syntax/#name-start-code-point
template <typename CharacterType>
bool IsNameStartCodePoint(CharacterType c) {
  return ((c | 0x20) >= 'a' && (c | 0x20) <= 'z') || c == '_' ||
         !base::IsASCII(c);
}

// https://drafts.csswg.org/css-syntax/#name-code-point
template <typename CharacterType>
bool IsNameCodePoint(CharacterType c) {
  return IsNameStartCodePoint(c) || base::IsASCIINumber(c) || c == '-';
}

// https://drafts.csswg.org/css-syntax/#check-if-two-code-points-are-a-valid-escape
inline bool TwoCharsAreValidEscape(base::UChar first, base::UChar second) {
  return first == '\\' && !IsCSSNewLine(second);
}

// Consumes a single whitespace, if the stream is currently looking at a
// whitespace. Note that \r\n counts as a single whitespace, as we don't do
// input preprocessing as a separate step.
//
// See https://drafts.csswg.org/css-syntax-3/#input-preprocessing
void ConsumeSingleWhitespaceIfNext(CSSTokenizerInputStream&);

// https://drafts.csswg.org/css-syntax/#consume-an-escaped-code-point
base::UChar32 ConsumeEscape(CSSTokenizerInputStream&);

// http://www.w3.org/TR/css3-syntax/#consume-a-name
std::u16string ConsumeName(CSSTokenizerInputStream&);

// https://drafts.csswg.org/css-syntax/#would-start-an-identifier
bool NextCharsAreIdentifier(base::UChar, const CSSTokenizerInputStream&);

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_PARSER_IDIOMS_H_
