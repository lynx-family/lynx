// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_DARWIN_STRING_RANGE_SANITIZATION_H_
#define CLAY_FML_PLATFORM_DARWIN_STRING_RANGE_SANITIZATION_H_

#include <Foundation/Foundation.h>

namespace fml {

// NOLINTNEXTLINE
// Returns a range encompassing the grapheme cluster in which |index| is located.
// NOLINTNEXTLINE
// A nil |text| or an index greater than or equal to text.length will result in
// `NSRange(NSNotFound, 0)`.
NSRange RangeForCharacterAtIndex(NSString* text, NSUInteger index);

// Returns a range encompassing the grapheme clusters falling in |range|.
//
// This method will not alter the length of the input range, but will ensure
// that the range's location is not in the middle of a multi-byte unicode
// sequence.
//
// An invalid range will result in `NSRange(NSNotFound, 0)`.
NSRange RangeForCharactersInRange(NSString* text, NSRange range);

}  // namespace fml

#endif  // CLAY_FML_PLATFORM_DARWIN_STRING_RANGE_SANITIZATION_H_
