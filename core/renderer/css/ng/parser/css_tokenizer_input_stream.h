// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_INPUT_STREAM_H_
#define CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_INPUT_STREAM_H_

#include <algorithm>
#include <string>
#include <vector>

#include "core/renderer/css/ng/css_ng_utils.h"

namespace lynx {
namespace css {

class CSSTokenizerInputStream {
 public:
  explicit CSSTokenizerInputStream(const std::u16string& input);
  CSSTokenizerInputStream(const CSSTokenizerInputStream&) = delete;
  CSSTokenizerInputStream& operator=(const CSSTokenizerInputStream&) = delete;

  // Gets the char in the stream replacing NUL characters with a unicode
  // replacement character. Will return (NUL) kEndOfFileMarker when at the
  // end of the stream.
  UChar NextInputChar() const {
    if (offset_ >= string_length_) return '\0';
    UChar result = string_[offset_];
    return result ? result : 0xFFFD;
  }

  // Gets the char at lookaheadOffset from the current stream position. Will
  // return NUL (kEndOfFileMarker) if the stream position is at the end.
  // NOTE: This may *also* return NUL if there's one in the input! Never
  // compare the return value to '\0'.
  UChar PeekWithoutReplacement(unsigned lookahead_offset) const {
    if ((offset_ + lookahead_offset) >= string_length_) return '\0';
    return string_[offset_ + lookahead_offset];
  }

  void Advance(unsigned offset = 1) { offset_ += offset; }
  void PushBack(UChar cc) {
    --offset_;
    // DCHECK(NextInputChar() == cc);
  }

  double GetDouble(unsigned start, unsigned end) const;

  template <bool characterPredicate(UChar)>
  unsigned SkipWhilePredicate(unsigned offset) {
    while ((offset_ + offset) < string_length_ &&
           characterPredicate(string_[offset_ + offset]))
      ++offset;
    return offset;
  }

  void AdvanceUntilNonWhitespace();

  size_t length() const { return string_length_; }
  size_t Offset() const { return std::min(offset_, string_length_); }

  std::u16string RangeAt(unsigned start, unsigned length) const {
    DCHECK(start + length <= string_length_);
    return string_.substr(start, length);
  }

 private:
  size_t offset_;
  const size_t string_length_;
  const std::u16string string_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_CSS_TOKENIZER_INPUT_STREAM_H_
