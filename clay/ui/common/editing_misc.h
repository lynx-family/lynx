// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_EDITING_MISC_H_
#define CLAY_UI_COMMON_EDITING_MISC_H_

// Miscellaneous definition or helpers for text editing.
namespace clay {

// When caret has more than 1 choice to locate, which side should it be.
// For example: Assume case that container box width is too short, so that
//   "abc 123" has to be soft wrapped, and imagine the whitespace is the soft
//   line break. Now caret offset is 3, which can be after 'c' or before '1'.
//   Thus, it depends on affinity to choice whether caret should at line
//   trailing or next line's leading.
enum class Affinity {
  kUpstream = 0,
  kDownstream,
};

enum class VerticalDirection {
  kUp = 0,
  kDown,
};

enum class HorizontalDirection {
  kLeft = 0,
  kRight,
};

// Returns true if |code_point| is a leading surrogate of a surrogate pair.
bool inline IsLeadingSurrogate(char32_t code_point) {
  return (code_point & 0xFFFFFC00) == 0xD800;
}
// Returns true if |code_point| is a trailing surrogate of a surrogate pair.
bool inline IsTrailingSurrogate(char32_t code_point) {
  return (code_point & 0xFFFFFC00) == 0xDC00;
}

}  // namespace clay

#endif  // CLAY_UI_COMMON_EDITING_MISC_H_
