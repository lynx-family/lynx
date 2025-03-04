// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_STRING_TO_NUMBER_H_
#define CORE_RENDERER_CSS_NG_PARSER_STRING_TO_NUMBER_H_

#include "base/include/string/string_utils.h"
#include "core/renderer/css/ng/parser/number_parsing_options.h"

namespace lynx {
namespace css {

using UChar = base::UChar;
using LChar = base::LChar;
using UChar32 = base::UChar32;

enum class NumberParsingResult {
  kSuccess,
  kError,
  // For UInt functions, kOverflowMin never happens. Negative numbers are
  // treated as kError. This behavior matches to the HTML standard.
  // https://html.spec.whatwg.org/C/#rules-for-parsing-non-negative-integers
  kOverflowMin,
  kOverflowMax,
};

// string -> int.
int CharactersToInt(const LChar*, size_t, NumberParsingOptions, bool* ok);
int CharactersToInt(const UChar*, size_t, NumberParsingOptions, bool* ok);

// string -> unsigned.
unsigned HexCharactersToUInt(const LChar*, size_t, NumberParsingOptions,
                             bool* ok);
unsigned HexCharactersToUInt(const UChar*, size_t, NumberParsingOptions,
                             bool* ok);
uint64_t HexCharactersToUInt64(const UChar*, size_t, NumberParsingOptions,
                               bool* ok);
uint64_t HexCharactersToUInt64(const LChar*, size_t, NumberParsingOptions,
                               bool* ok);
unsigned CharactersToUInt(const LChar*, size_t, NumberParsingOptions, bool* ok);
unsigned CharactersToUInt(const UChar*, size_t, NumberParsingOptions, bool* ok);

// NumberParsingResult versions of CharactersToUInt. They can detect
// overflow. |NumberParsingResult*| should not be nullptr;
unsigned CharactersToUInt(const LChar*, size_t, NumberParsingOptions,
                          NumberParsingResult*);
unsigned CharactersToUInt(const UChar*, size_t, NumberParsingOptions,
                          NumberParsingResult*);

// string -> int64_t.
int64_t CharactersToInt64(const LChar*, size_t, NumberParsingOptions, bool* ok);
int64_t CharactersToInt64(const UChar*, size_t, NumberParsingOptions, bool* ok);

// string -> uint64_t.
uint64_t CharactersToUInt64(const LChar*, size_t, NumberParsingOptions,
                            bool* ok);
uint64_t CharactersToUInt64(const UChar*, size_t, NumberParsingOptions,
                            bool* ok);

// FIXME: Like the strict functions above, these give false for "ok" when there
// is trailing garbage.  Like the non-strict functions above, these return the
// value when there is trailing garbage.  It would be better if these were more
// consistent with the above functions instead.

// string -> double.
//
// These functions accepts:
//  - leading '+'
//  - numbers without leading zeros such as ".5"
//  - numbers ending with "." such as "3."
//  - scientific notation
//  - leading whitespace (base::IsASCIISpace, not base::IsHTMLSpace)
//  - no trailing whitespace
//  - no trailing garbage
//  - no numbers such as "NaN" "Infinity"
//
// A huge absolute number which a double can't represent is accepted, and
// +Infinity or -Infinity is returned.
//
// A small absolute numbers which a double can't represent is accepted, and
// 0 is returned
double CharactersToDouble(const LChar*, size_t, bool* ok);
double CharactersToDouble(const UChar*, size_t, bool* ok);

// |parsed_length| will have the length of characters which was parsed as a
// double number. It will be 0 if the input string isn't a number. It will be
// smaller than |length| if the input string contains trailing
// whiespace/garbage.
double CharactersToDouble(const LChar*, size_t length, size_t& parsed_length);
double CharactersToDouble(const UChar*, size_t length, size_t& parsed_length);

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_STRING_TO_NUMBER_H_
