// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_NUMBER_PARSING_OPTIONS_H_
#define CORE_RENDERER_CSS_NG_PARSER_NUMBER_PARSING_OPTIONS_H_

#include <ostream>

#include "base/include/log/logging.h"

namespace lynx {
namespace css {

// Copyable and immutable object representing number parsing flags.
class NumberParsingOptions {
 public:
  static constexpr unsigned kNone = 0;
  static constexpr unsigned kAcceptTrailingGarbage = 1;
  static constexpr unsigned kAcceptLeadingPlus = 1 << 1;
  static constexpr unsigned kAcceptLeadingTrailingWhitespace = 1 << 2;
  static constexpr unsigned kAcceptMinusZeroForUnsigned = 1 << 3;

  // 'Strict' behavior for WTF::String.
  static constexpr unsigned kStrict =
      kAcceptLeadingPlus | kAcceptLeadingTrailingWhitespace;
  // Non-'Strict' behavior for WTF::String.
  static constexpr unsigned kLoose = kStrict | kAcceptTrailingGarbage;

  // This constructor allows implicit conversion from unsigned.
  NumberParsingOptions(unsigned options) : options_(options) {
    DCHECK(options < 1u << 4);
  }

  bool AcceptTrailingGarbage() const {
    return options_ & kAcceptTrailingGarbage;
  }
  bool AcceptLeadingPlus() const { return options_ & kAcceptLeadingPlus; }
  bool AcceptWhitespace() const {
    return options_ & kAcceptLeadingTrailingWhitespace;
  }
  bool AcceptMinusZeroForUnsigned() const {
    return options_ & kAcceptMinusZeroForUnsigned;
  }

 private:
  unsigned options_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_NUMBER_PARSING_OPTIONS_H_
