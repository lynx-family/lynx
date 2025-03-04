// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/css_tokenizer_input_stream.h"

#include "core/renderer/css/ng/parser/string_to_number.h"

namespace lynx {
namespace css {

CSSTokenizerInputStream::CSSTokenizerInputStream(const std::u16string& input)
    : offset_(0), string_length_(input.length()), string_(input) {}

void CSSTokenizerInputStream::AdvanceUntilNonWhitespace() {
  // Using HTML space here rather than CSS space since we don't do preprocessing
  while (offset_ < string_length_ && base::IsHTMLSpace(string_[offset_]))
    ++offset_;
}

double CSSTokenizerInputStream::GetDouble(unsigned start, unsigned end) const {
  DCHECK(start <= end && ((offset_ + end) <= string_length_));
  bool is_result_ok = false;
  double result = 0.0;
  if (start < end) {
    result = CharactersToDouble(string_.data() + offset_ + start, end - start,
                                &is_result_ok);
  }
  // FIXME: It looks like callers ensure we have a valid number
  return is_result_ok ? result : 0.0;
}

}  // namespace css
}  // namespace lynx
